#ifndef _DRIVER_AUDIO_H_
#define _DRIVER_AUDIO_H_

#include "driver_dma_fft.h"
#include "config.h"
#include "driver_ringbuff.h"
#include "app_env.h"

//#define ADC_FIRST_DELAY         //to control whether discard the first adc data to avoid openning noise
#define PAMUTE_GPIO_PIN         9
#define MUTE_LOW_THD            2
#define MUTE_HIGH_THD           7

typedef struct _aud_mute_cfg_s
{
    uint8 mute_pin;
    uint8 mute_high_flag;
    uint8 mute_status;
    uint8 backup_mute_status;
    uint8 shade_flag;
    uint8 mute_outside;
    uint8 auto_mute_flag; //自动(快速)静音标识
    uint32 mute_mask;
}aud_mute_cfg_t;
#define AUDIO_BUFF_LEN     4096  // 4096//3072//2560	/**< 2.5k */

#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
//#define LINEIN_SBC_ENCODE_NODE_NUM    (8)
#define LINEIN_SBC_ENCODE_NODE_NUM    (5)
//#define PCM_BUFF_LEN    (512*6)	/**< 1k */
//#define PCM_BUFF_LEN    (512*7)	/**< 1k */
#define PCM_BUFF_LEN    (512*(LINEIN_SBC_ENCODE_NODE_NUM + 1))	/**< 1k */
#else
#define PCM_BUFF_LEN    1536	/**< 1k */
#endif
#define SDADC_VOLUME_MAX        124 //95    // all 48db, 0.5db per step

typedef struct
{
    SAMPLE_ALIGN uint8 data_buff[PCM_BUFF_LEN];
    driver_ringbuff_t   aud_rb;
    int   empty_count;
    int   channels;
#ifdef BEKEN_DEBUG
    int aud_empty_count;
#endif
#if CONFIG_ADC_DMA
    struct dma_struct adc_dma_conf;  // DMA for ADC sampling.
#endif
}PCM_CTRL_BLK;


#if(CONFIG_AUD_FADE_IN_OUT == 1)
typedef enum
{
    AUD_FADE_NONE       = 0,
    AUD_FADE_IN         = 1,
    AUD_FADE_OUT        = 2,
    AUD_FADE_FINISHED   = 4
}t_aud_fade_state;
__INLINE__ void  set_aud_fade_in_out_state(t_aud_fade_state state);
__INLINE__ t_aud_fade_state get_aud_fade_in_out_state(void);
void aud_fade_in_out_process(void);
#endif


#define AUDIO_VOLUME_MAX        16

#define AUDIO_DIV_8K            0x06590000
#define AUDIO_DIV_8K_SLOW       (0x06590000+2048)
#define AUDIO_DIV_8K_FAST       (0x06590000-2048)
#define AUDIO_DIV_8K_1PPM      212


#define AUDIO_DIV_16K           0x06590000
#define AUDIO_DIV_16K_SLOW      (0x06590000+2048)
#define AUDIO_DIV_16K_FAST      (0x06590000-2048)
#define AUDIO_DIV_16K_1PPM     106

#define AUDIO_DIV_441K          0x049B2369
#define AUDIO_DIV_441K_SLOW     0x049B2970
#define AUDIO_DIV_441K_FAST     0x049B1D5C
#define AUDIO_DIV_441K_1PPM     77

#define AUDIO_DIV_48K           0x043B5555
#define AUDIO_DIV_48K_SLOW      0x043B5AE0
#define AUDIO_DIV_48K_FAST      0x043B4FC8
#define AUDIO_DIV_48K_1PPM     70

#define AUDIO_CLK_DIV_441K   2
#define AUDIO_CLK_DIV_48K     3
#define AUDIO_CLK_DIV_8K       0
#define AUDIO_CLK_DIV_16K     1

#ifdef CONFIG_TWS
#define AUDIO_SYNC_INTVAL 		3000

#define AUDIO_DIV_COVER 		1
#define AUDIO_DIV_441K_Dot ((175*AUDIO_SYNC_INTVAL/10000)*AUDIO_DIV_COVER)

#define AUDIO_DIV_48K_Dot ((142*AUDIO_SYNC_INTVAL/10000)*AUDIO_DIV_COVER)

#define AUDIO_DIV_INIT ((AUDIO_DIV_441K_SLOW-AUDIO_DIV_441K)/AUDIO_DIV_441K_Dot)
#define AUDIO_DIV_MAX (AUDIO_DIV_INIT*3)

#endif

typedef enum
{
    AUDIO_DAC_NORM = 0,
    AUDIO_DAC_SLOW = 0x01,
    AUDIO_DAC_SLOWER = 0x02,
    AUDIO_DAC_FAST = 0x04,
    AUDIO_DAC_FASTER = 0x08,
}t_dac_clk_state;

void aud_set_dac_frac_coef(uint32 coef);
uint8_t get_audio_dac_clk_state(void);
void RAM_CODE audio_dac_clk_process(void);
void aud_set_dac_opened(uint8 enable);
void aud_set_adc_opened(uint8 enable);
void aud_set_adda_en_response(void);


#if(CONFIG_ANA_DAC_CLOSE_IN_IDLE == 1)
void BK3000_dig_dac_close(void);
void aud_ana_dac_close_in_idle(void);
#endif
void aud_PAmute_oper( uint8 enable );
void aud_mcu2dsp_cmd_timeout(void);
void aud_wait_for_dsp_resp(void);
void aud_mic_open(uint8 enable);
void aud_mic_volume_set(uint8 volume);
void aud_adc_dig_volume_set( uint8 volume );
void aud_aux_ana_gain_set( uint8 gain );
void aud_aux_dig_gain_set( uint8 gain );
void aud_volume_table_init(void);
uint32 aud_get_buffer_size(void);
void aud_dsp_mode_mp3_set(void);
uint32 aud_get_buffer_used_size(void);
int32 aud_initial(uint32 freq, uint32 channels, uint32 bits_per_sample);
int32 aud_adc_initial(uint32 freq, uint32 channels, uint32 bits_per_sample);
void aud_open(void);
void aud_close(void);
void aud_volume_set(int8 volume);
void aud_volume_mute(uint8 enable);
void aud_mute_init( void );
void aud_mute_update( int16 samplel, int16 sampler );
void aud_mic_mute(uint8 enable);
void aud_clr_PAmute(void);


void aud_discard_adc_clear_flag(void);
void aud_discard_adc_data(uint8* buff,uint16 size);
void aud_read_buffer(uint8 *buff, uint32 size);
void aud_fill_buffer(uint8 *buff,uint16 size);

void BK3000_Ana_Line_enable( uint8 enable);
uint8 DRAM_CODE aud_discard_sco_data(void);


#if (CONFIG_APP_MP3PLAYER == 1)
void RAM_CODE wav_fill_buffer( uint8 *buff, uint16 size );
#endif
void RAM_CODE audio_dac_clk_process(void);
#ifdef CONFIG_TWS
void dac_init_clk(void);
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE

typedef struct _driver_lineindata_s
{
    driver_ringbuff_t   data_rb;
}DRIVER_LINEINDATA_T;
DRIVER_LINEINDATA_T linein_data_blk;

void  aud_linein_fill_buffer( uint8 *buff, uint16 size );
driver_ringbuff_t *get_line_ringbuf(void);
void line_in_fill_aud_buf(uint8 *buff, uint16 size );

#endif
void set_dac_clk(uint32_t clk_val);
inline uint32_t get_dac_clk(void);
#endif
#if(CONFIG_APP_AEC == 1)
void pcm_read_buffer(uint8 *buff,uint16 size);
void pcm_fill_buffer(uint8 *buff,uint16 size);
#endif
uint32 pcm_get_buffer_size(void);
uint32 pcm_get_data_size(void);
#if CONFIG_ADC_DMA
void adc_dma_initial(void);
void adc_dma_start(void);
void adc_dma_stop(void);
void adc_dma_reset(void);
int adc_init(uint32 freq, uint32 channels, uint32 bits_per_sample);
int DRAM_CODE adc_dma_get_data(uint8 *buf, uint32 buf_len);
uint32 adc_isr_get_data(uint8* buf, uint32 buf_len);
#endif
#endif

