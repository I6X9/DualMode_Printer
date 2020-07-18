/**
 **************************************************************************************
 * @file    audio_sync.c
 * @brief   Audio synchronization
 *
 * @author  Aixing.Li
 * @version V2.1.0
 *
 * &copy; 2017-2019 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#include "app_sbc.h"
#include "audio_sync.h"
#include "driver_audio.h"
#include "app_tws.h"

#ifdef CONFIG_TWS

#define AUDIO_SYNC_DBG_ENABLE           (0)
#if AUDIO_SYNC_DBG_ENABLE
extern int32 os_printf(const char *fmt, ...) ;
#define AUDIO_SYNC_DBG(fmt, ...)        os_printf(fmt, ##__VA_ARGS__)
#else
#define AUDIO_SYNC_DBG(fmt, ...)
#endif

#define SAMPLES_PER_SBC_FRAME           (128)
#define BT_CLK_TICKS_PER_SECOND         (3200)

#define AUDIO_SYNC_FLAG_W4_DAC_OPEN     (0x00000001)
#define AUDIO_SYNC_FLAG_P4_DAC_OPEN     (0x00000002)

#define AUDIO_SYNC_DAC_ADJUST_BASE_PPM  (100)
#define AUDIO_SYNC_MASTER_LEVEL         (SBC_MEM_POOL_MAX_NODE / 2)
#define AUDIO_SYNC_MASTER_THRESHOLD     (SBC_MEM_POOL_MAX_NODE * 3 / 20) //30%
#define AUDIO_SYNC_SLAVE_THRESHOLD      (3)
#define AUDIO_SYNC_SLAVE_ADD_DEL_THD    (10)
#define AUDIO_SYNC_RATIO_FRA_BITS       (4)

extern inline t_clock get_btclk_as_piconet_slave(void);
extern inline t_clock get_btclk_as_piconet_master(void);

int32_t audio_sync_process_by_buffer(uint32_t tp);
int32_t audio_sync_process_by_btclk(uint32_t tp);

static uint8_t  AUDIO_SYNC_RATIO_Q4[] = { 0, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 19, 20, 22, 24, 25, 27, 29, 31, 33, 36, 38, 40, 43, 45, 48, 51, 54, 57, 60, 63, 67, 71, 75, 79, 83, 87, 92, 96, 101, 107, 112, 118, 124, 130, 136, 143, 150, 157, 165, 173, 181, 190, 199, 209, 219, 229, 240 };
static uint32_t audio_sync_flag;
static uint32_t audio_sync_dac_start_play_time;
static int32_t  (*audio_sync_process_for_primary)(uint32_t) = audio_sync_process_by_btclk;

static inline uint32_t audio_sync_get_bt_clk(void)
{
    return get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY ? get_btclk_as_piconet_master() : get_btclk_as_piconet_slave();
}

static inline uint32_t audio_sync_get_dac_sample_rate(void)
{
    switch((BK3000_AUD_AUDIO_CONF >> 6) & 0x3)
    {
    case 0:
        return 8000;
    case 1:
        return 16000;
    case 2:
        return 44100;
    case 3:
        return 48000;
    default:
        return 0;
    }
}

/**
 * @brief  Get residue sbc nodes
 * @return the residue sbc nodes
 */
static inline uint32_t audio_sync_get_residue_sbc_nodes(void)
{
    extern app_sbc_t app_sbc;
    return app_sbc.sbc_ecout;
}

/**
 * @brief  Get residue PCM samples in the fifo
 * @return the residue PCM samples in the fifo
 */
static inline uint32_t audio_sync_get_residue_pcm_samples(void)
{
    return aud_get_buffer_used_size() / 4;
}

/**
 * @brief  Get residue time (summation of residue sbc frame nodes and residue PCM samples), unit in BT clock
 * @return The residue playing time since the received stream packet
 */
static inline uint32_t audio_sync_get_residue_time(void)
{
    uint32_t r;

    r = audio_sync_get_residue_sbc_nodes() * SAMPLES_PER_SBC_FRAME + audio_sync_get_residue_pcm_samples();

    return (r * BT_CLK_TICKS_PER_SECOND) / audio_sync_get_dac_sample_rate();
}

/**
 * @brief  Adjust DAC sample rate
 * @param  ppm DAC sample rate adjust parameter, unit in ppm (Parts Per Million)
 * @return NULL
 */
static inline void audio_sync_adjust_dac_sample_rate(int32_t ppm)
{
    //AUDIO_SYNC_DBG("PPM: %d\n", ppm);

    int32_t dac_frac_coef = audio_sync_get_dac_sample_rate() == 44100 ? 0x049B2368 : 0x043B5554;

    dac_frac_coef -= (int32_t)((int64_t)dac_frac_coef * ppm / 1000000);

    BK3000_AUD_DAC_FRACCOEF = dac_frac_coef;
}

static inline void audio_sync_set_flag(uint32_t flag)
{
    audio_sync_flag |= flag;
}

static inline void audio_sync_unset_flag(uint32_t flag)
{
    audio_sync_flag &= ~flag;
}

void audio_sync_init(void)
{
    BK3000_AUD_AUDIO_CONF &= ~(1 << 2);
    //aud_dac_dma_enable(0);
    audio_sync_flag = AUDIO_SYNC_FLAG_W4_DAC_OPEN;
    audio_sync_dac_start_play_time = 0;

    audio_sync_process_for_primary = bt_flag1_is_set(APP_FLAG_LINEIN) ? audio_sync_process_by_buffer : audio_sync_process_by_btclk;
}

void audio_sync_prepare_for_dac_open(uint32_t t)
{
    if((audio_sync_flag & AUDIO_SYNC_FLAG_W4_DAC_OPEN) && !(audio_sync_flag & AUDIO_SYNC_FLAG_P4_DAC_OPEN))
    {
        audio_sync_dac_start_play_time = t;
        audio_sync_flag |= AUDIO_SYNC_FLAG_P4_DAC_OPEN;
    }
}

void audio_sync_process_for_dac_open(void)
{
    if(audio_sync_flag & AUDIO_SYNC_FLAG_P4_DAC_OPEN)
    {
        uint32_t tp = audio_sync_get_bt_clk();

        if( tp >= audio_sync_dac_start_play_time)
        {
            //BK3000_GPIO_20_CONFIG = 2;
            BK3000_AUD_AUDIO_CONF |= (1 << 2);
            //aud_dac_dma_enable(1);
            //AUDIO_SYNC_DBG("[%d, %d]\n", audio_sync_get_residue_sbc_nodes(), audio_sync_get_residue_pcm_samples());
            audio_sync_flag &= ~(AUDIO_SYNC_FLAG_P4_DAC_OPEN | AUDIO_SYNC_FLAG_W4_DAC_OPEN);
            //BK3000_GPIO_20_CONFIG = 0;
        }
    }
}

uint32_t audio_sync_calc_stream_packet_play_time(void)
{
    if(audio_sync_flag & AUDIO_SYNC_FLAG_W4_DAC_OPEN)
    {
        if(audio_sync_flag & AUDIO_SYNC_FLAG_P4_DAC_OPEN)
        {
            return audio_sync_dac_start_play_time + audio_sync_get_residue_time();
        }
        else
        {
            return audio_sync_get_bt_clk() + audio_sync_get_residue_time() + (AUDIO_SYNC_MASTER_LEVEL * SAMPLES_PER_SBC_FRAME * BT_CLK_TICKS_PER_SECOND) / audio_sync_get_dac_sample_rate();
        }
    }
    else
    {
        return audio_sync_get_bt_clk() + audio_sync_get_residue_time();
    }
}

int32_t audio_sync_process_by_buffer(uint32_t tp)
{
    int32_t  dif, dir;
    uint32_t nodes = audio_sync_get_residue_sbc_nodes();

    (void)tp;

    AUDIO_SYNC_DBG("%d\n", nodes - AUDIO_SYNC_MASTER_LEVEL);

    if(nodes > AUDIO_SYNC_MASTER_LEVEL)
    {
        dir = 1;
        dif = nodes - AUDIO_SYNC_MASTER_LEVEL;
    }
    else
    {
        dir = -1;
        dif = AUDIO_SYNC_MASTER_LEVEL - nodes;
    }

    dif = ((dif << AUDIO_SYNC_RATIO_FRA_BITS) + AUDIO_SYNC_MASTER_THRESHOLD / 2) / AUDIO_SYNC_MASTER_THRESHOLD;

    if(dif >= sizeof(AUDIO_SYNC_RATIO_Q4)) dif = sizeof(AUDIO_SYNC_RATIO_Q4) - 1;

    dif = AUDIO_SYNC_RATIO_Q4[dif] / 4;
    dif = AUDIO_SYNC_DAC_ADJUST_BASE_PPM * dif >> AUDIO_SYNC_RATIO_FRA_BITS;

    audio_sync_adjust_dac_sample_rate(dir * dif);

    return 0;
}

int32_t audio_sync_process_by_btclk(uint32_t tp)
{
    int32_t  dif = 0, dir = 0;
    uint32_t tck = (audio_sync_flag & AUDIO_SYNC_FLAG_W4_DAC_OPEN) ? audio_sync_dac_start_play_time : audio_sync_get_bt_clk();
    uint32_t tpr = tck + audio_sync_get_residue_pcm_samples() * BT_CLK_TICKS_PER_SECOND / audio_sync_get_dac_sample_rate();

    AUDIO_SYNC_DBG("%d\n", tpr - tp);

    if(tpr > tp)
    {
        dir = 1;
        dif = tpr - tp;
    }
    else
    {
        dir = -1;
        dif = tp - tpr;
    }

    if(dif > AUDIO_SYNC_SLAVE_ADD_DEL_THD) return -dir;

    dif = ((dif << AUDIO_SYNC_RATIO_FRA_BITS) + AUDIO_SYNC_SLAVE_THRESHOLD / 2) / AUDIO_SYNC_SLAVE_THRESHOLD;

    if(dif >= sizeof(AUDIO_SYNC_RATIO_Q4)) dif = sizeof(AUDIO_SYNC_RATIO_Q4) - 1;

    dif = AUDIO_SYNC_RATIO_Q4[dif];
    dif = AUDIO_SYNC_DAC_ADJUST_BASE_PPM * dif >> AUDIO_SYNC_RATIO_FRA_BITS;

    audio_sync_adjust_dac_sample_rate(dir * dif);

    return 0;
}

int32_t audio_sync_process(uint32_t tp)
{
    if(bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
    {
        if(get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)
        {
            return audio_sync_process_for_primary(tp);
        }
        else
        {
            return audio_sync_process_by_btclk(tp);
        }
    }
    else
    {
        return 0;//audio_sync_process_by_buffer(tp);
    }
}

void audio_sync_show_info(void)
{
    AUDIO_SYNC_DBG("¡¾%d¡¿: %d, %d\n", audio_sync_get_bt_clk(), audio_sync_get_residue_sbc_nodes(), audio_sync_get_residue_pcm_samples());
}

#endif
