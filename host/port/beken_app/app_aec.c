#include <string.h>
#include "aec.h"
#include "app_beken_includes.h"
#include "driver_beken_includes.h"

#if (CONFIG_APP_AEC  == 1)
#define AEC_MULTIPLEX_SBC_MEM   (0)

#define ECHO_RX_BUFFER_SIZE     (360 * 2)
#define ECHO_TX_BUFFER_SIZE     (256 * 2)

static uint8_t  aec_init_flag = 0;
static uint8_t* aec_rx_buf    = NULL;
static uint8_t* aec_tx_buf    = NULL;

static driver_ringbuff_t aec_rx_rb;
static driver_ringbuff_t aec_tx_rb;

static AECContext* aec = NULL;

CONST unsigned short HFP_VOLUME_Q15[17] =
{
    0,
    184,
    260,
    368,
    519,
    734,
    1036,
    1464,
    2068,
    2920,
    4125,
    5827,
    8231,
    11627,
    16423,
    23198,
    32767
};

static uint8 *get_aec_ram_buff(void)
{
    extern uint32 _sbcmem_begin;
    return (uint8 *)((uint32)&_sbcmem_begin + 4096); 
}

void app_aec_init(int sample_rate)
{
    int32_t  ec_depth;
    uint32_t aec_mem_size  = 0;
    app_env_handle_t env_h = app_env_get_handle();

    os_printf("app_aec_init(%d)\r\n", sample_rate);

#if (AEC_MULTIPLEX_SBC_MEM == 1)
    aec_rx_buf    = get_aec_ram_buff();
    aec_mem_size += ECHO_RX_BUFFER_SIZE;
    aec_tx_buf    = get_aec_ram_buff() + aec_mem_size;
    aec_mem_size += ECHO_TX_BUFFER_SIZE;
#else
    aec_rx_buf    = (uint8_t*)jmalloc(ECHO_RX_BUFFER_SIZE, M_ZERO); 
    aec_tx_buf    = (uint8_t*)jmalloc(ECHO_TX_BUFFER_SIZE, M_ZERO);
    aec_mem_size += ECHO_RX_BUFFER_SIZE;
    aec_mem_size += ECHO_TX_BUFFER_SIZE;
#endif

    rb_init(&aec_rx_rb, (uint8 *)aec_rx_buf, 0, ECHO_RX_BUFFER_SIZE);
    rb_init(&aec_tx_rb, (uint8 *)aec_tx_buf, 0, ECHO_TX_BUFFER_SIZE);

#if (AEC_MULTIPLEX_SBC_MEM == 1)
    aec = (AECContext*)(get_aec_ram_buff() + aec_mem_size);
    aec_mem_size += aec_size();
#else
    aec = (AECContext*)jmalloc(aec_size(), M_ZERO);
    aec_mem_size += aec_size();
#endif

    aec_init(aec);

    ec_depth = env_h->env_cfg.env_aec_cfg.aec_fft_shift;
    ec_depth = ec_depth < 0 ? (8 + ec_depth) : (ec_depth << 3);
    aec_ctrl(aec, AEC_CTRL_CMD_SET_EC_DEPTH, ec_depth);
    aec_ctrl(aec, AEC_CTRL_CMD_SET_DECAY_TIME, 112);

    aec_init_flag = 1;
}

void app_aec_uninit(void)
{
    aec_init_flag = 0;
    
    if(NULL != aec_rx_buf)
    {
    #if (AEC_MULTIPLEX_SBC_MEM == 0)
        jfree(aec_rx_buf); 
    #endif
        aec_rx_buf = NULL;
    }
    
    if(NULL != aec_tx_buf)
    {
    #if (AEC_MULTIPLEX_SBC_MEM == 0)
        jfree(aec_tx_buf); 
    #endif
        aec_tx_buf = NULL;
    }
    
    if(NULL != aec)
    {
    #if (AEC_MULTIPLEX_SBC_MEM == 0)
        jfree(aec);
    #endif        
        aec = NULL;
    }
}

int app_aec_get_rx_size(void)
{
    return ECHO_RX_BUFFER_SIZE - rb_get_buffer_size(&aec_rx_rb);
}

int app_aec_get_tx_size(void)
{
    return ECHO_TX_BUFFER_SIZE - rb_get_buffer_size(&aec_tx_rb);
}

void DRAM_CODE app_aec_fill_rxbuff(uint8 *buff, uint8 fid, uint32 len)
{
    if(app_wave_playing()) return;

    if(aec_init_flag) rb_fill_buffer(&aec_rx_rb, buff, len, AEC_RX_FILL);

    //Output limitation
    #if 0
    {
        #define  MAX_PCM_VALUE   4096
        int32_t  i;
        int16_t* pcm = (int16_t*)buff;
        for(i = 0; i < len / 2; i++)
        {
            int16_t t = *pcm;
            if(t > MAX_PCM_VALUE)
            {
                t = MAX_PCM_VALUE;
            }
            else if(t < -MAX_PCM_VALUE)
            {
                t = -MAX_PCM_VALUE;
            }
            *pcm++ = t;
        }
    }
    #endif

    aud_fill_buffer((uint8 *)buff, len);
}

void DRAM_CODE app_aec_fill_txbuff(uint8 *buff, uint32 len)
{
    if(aec_init_flag) rb_fill_buffer(&aec_tx_rb, buff, len, AEC_TX_FILL);
}

void RAM_CODE app_aec_swi()
{
    if(aec_init_flag)
    {
    #if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
        BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
    #endif

        int rx_size = app_aec_get_rx_size() / 2;
        int tx_size = pcm_get_data_size() / 4;

        if((rx_size >= AEC_FRAME_SAMPLES) && (tx_size >= AEC_FRAME_SAMPLES))
        {
            int16_t* rin;
            int16_t* sin;
            int16_t* out;
            uint32_t flags;

            aec_ctrl(aec, AEC_CTRL_CMD_GET_TEMP_BUF, (uint32_t)&rin);
            aec_ctrl(aec, AEC_CTRL_CMD_GET_FLAGS,    (uint32_t)&flags);

            sin = rin + AEC_FRAME_SAMPLES;
            out = sin + AEC_FRAME_SAMPLES;

            rb_get_buffer_with_length(&aec_rx_rb,(uint8*)rin, AEC_FRAME_SAMPLES * 2);
            aud_read_buffer((uint8*)sin, AEC_FRAME_SAMPLES * 2);

            if(flags & 0x80)
            {
                int i;
                char* prin = (char*)rin;
                char* psin = (char*)sin;

                for(i = 0; i < AEC_FRAME_SAMPLES; i++)
                {
                    while(!UART_TX_WRITE_READY);
                    UART_WRITE_BYTE(*prin++);
                    while(!UART_TX_WRITE_READY);
                    UART_WRITE_BYTE(*prin++);
                    while(!UART_TX_WRITE_READY);
                    UART_WRITE_BYTE(*psin++);
                    while(!UART_TX_WRITE_READY);
                    UART_WRITE_BYTE(*psin++);
                }

                memcpy(out, sin, AEC_FRAME_SAMPLES * 2);
            }
            else
            {
                int32_t mic_delay = (int32_t)aud_get_buffer_used_size() / 4 - (int32_t)rb_get_used_size(&aec_rx_rb) / 2 + (int32_t)pcm_get_data_size() / 4;
                if(mic_delay < 0)
                {
                    mic_delay = 0;
                }
                else if(mic_delay > AEC_MAX_MIC_DELAY)
                {
                    mic_delay = AEC_MAX_MIC_DELAY;
                }
                aec_ctrl(aec, AEC_CTRL_CMD_SET_MIC_DELAY, mic_delay);
                aec_proc(aec, rin, sin, out);
            }

            pcm_fill_buffer((uint8*)out, AEC_FRAME_SAMPLES * 2);
        }
    }
}

void app_set_aec_para(uint8 *para)
{
    os_printf("app_set_aec_para(%08X, %d, %02X, %d, %d, %d, %d)\r\n", aec, (int8_t)para[0], para[1], para[2], para[3], para[4], para[5]);

    if(aec)
    {
        #if 1
        int8_t ec_depth = (int8_t)para[0];
        para[0] = ec_depth < 0 ? (8 + ec_depth) : (ec_depth << 3);
        para[1] = 0x0A;
        para[2] = 0x04;
        para[3] = 0x20;
        para[4] = 0x7F;
        para[5] = 0x0A;
        #endif

        aec_ctrl(aec, AEC_CTRL_CMD_SET_PARAMS, (uint32_t)para);
    }
}

void aec_fill_buffer(uint8_t *buf,uint32_t len)
{
    app_aec_fill_rxbuff(buf,0,len);
}

void aec_read_buffer(uint8_t *buf,uint32_t len)
{
    if (aec_init_flag)
    {
        pcm_read_buffer(buf,len);   // from pcm ringbuff
    }
    else
    {
        aud_read_buffer(buf,len);    // from DSP
    }
}
#endif

