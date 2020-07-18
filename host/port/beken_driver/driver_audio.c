#include "hw_leds.h"
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "coder.h"
#include "bkmp3_resample.h"
#include <string.h>
#include "tws_hfp.h"
#define DAC_CLK_ADJUST_STEP     8192
uint32 s_aud_dac_fraccoef[2][3] = 
{
    {AUDIO_DIV_441K_SLOW,AUDIO_DIV_441K,AUDIO_DIV_441K_FAST},
    {AUDIO_DIV_48K_SLOW,AUDIO_DIV_48K,AUDIO_DIV_48K_FAST}    
};
/* These MACRO define are same as work mode of DSP */
#define AUDIO_IDLE_MODE          0x00
#define AUDIO_HFP_MODE           0x01
#define AUDIO_A2DP_MODE          0x02
#define AUDIO_AUX_MODE           0x04

static t_dac_clk_state s_aud_dac_state = AUDIO_DAC_NORM;
static t_dac_clk_state s_current_aud_dac_state = AUDIO_DAC_NORM;

PCM_CTRL_BLK    pcm_ctrl_blk;
#if 0//def CONFIG_DRIVER_DAC
static void dac_volume_set( int8 volume );
static void audio_dac_digital_gain_set(uint32 new_gain);
static void audio_dac_analog_gain_set(uint32_t gain, uint32_t dc_offset_l, uint32_t dc_offset_r);
#endif

typedef struct _AudioExchangeInfo
{
    uint32_t adc_fifo_fill_size;
    uint32_t adc_fifo_free_size;
    uint32_t dac_fifo_fill_size;
    uint32_t dac_fifo_free_size;
}AudioExchangeInfo;
/* This info exchange between MCU and DSP, Locate at share memory,address:0x00E00000 */
extern uint32 _shdata_ram_begin;
volatile AudioExchangeInfo* audio_exchange_info   = (AudioExchangeInfo*)((uint32_t) &_shdata_ram_begin);

#if (CONFIG_DEBUG_PCM_TO_UART == 1)
extern void uart_send_ppp(unsigned char *buff, unsigned char fid,unsigned short len);
#endif

#if(CONFIG_AUD_FADE_IN_OUT == 1)
static t_aud_fade_state s_aud_fade_status = AUD_FADE_NONE;
int16_t s_fade_scale = 0;
__INLINE__ void  set_aud_fade_in_out_state(t_aud_fade_state state)
{
    if((AUD_FADE_SCALE_MIN == s_fade_scale) && (AUD_FADE_IN == state))
    {
        s_fade_scale = AUD_FADE_SCALE_MIN;
    }
    else if((AUD_FADE_SCALE_MIN == s_fade_scale) && (AUD_FADE_OUT == state))
    {
        s_fade_scale = AUD_FADE_SCALE_MAX;
    }
    else if(AUD_FADE_FINISHED == state)
    {
        s_fade_scale = AUD_FADE_SCALE_MIN;
    }
    else if(AUD_FADE_NONE == state)
    {
        s_fade_scale = AUD_FADE_SCALE_MIN;
    }

    s_aud_fade_status = state;  
}
__INLINE__ t_aud_fade_state get_aud_fade_in_out_state(void)
{
    return s_aud_fade_status;
}
void aud_fade_status_debug(void)
{
    os_printf("---------Fade in/out status------------\r\n");
    os_printf("| Fade in/out state:%d\r\n",s_aud_fade_status);
    os_printf("| Fade in/out step :%d\r\n",s_fade_scale);
    os_printf("---------Fade in/out end---------------\r\n");
}
void aud_fade_in_out_process(void)
{
    t_aud_fade_state status;
    status = get_aud_fade_in_out_state();
    if(status == AUD_FADE_IN)
	{
	#if TWS_A2DP_AVOID_POP_ENABLE
		if (bt_flag1_is_set(APP_FLAG_MUSIC_PLAY))
		{
			if (tws_a2dp_get_avoid_pop_sound_cnt() == 0)
				s_fade_scale += AUD_FADE_STEP;            
		}
		else
		{
			s_fade_scale += AUD_FADE_STEP;
		}
	#else
		s_fade_scale += AUD_FADE_STEP;
	#endif

		if(s_fade_scale >= AUD_FADE_SCALE_MAX)
		{
			set_aud_fade_in_out_state(AUD_FADE_FINISHED);
		}
	}
    else if(AUD_FADE_OUT == status)
    {
        s_fade_scale -= AUD_FADE_STEP;
        if(s_fade_scale <= AUD_FADE_SCALE_MIN)
        {
            set_aud_fade_in_out_state(AUD_FADE_FINISHED);
        }
    }
    else
    {
        set_aud_fade_in_out_state(AUD_FADE_NONE);
    }
}
#endif

static __inline int16 f_sat(int32 din)
{
    if (din>32767)
        return 32767;
    if (din<-32768)
        return -32768;
    else
        return(din);
}
uint8 DRAM_CODE aud_discard_sco_data(void)
{
    if(bt_flag1_is_set( APP_FLAG_WAVE_PLAYING))
    {
        return 1;
    }
#if 1/*(CONFIG_APP_TOOLKIT_5 == 1)*/
    else if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_DISABLE_IOS_INCOMING_RING)
        &&bt_flag2_is_set(APP_FLAG2_HFP_INCOMING)
        &&( !bt_flag1_is_set( APP_FLAG_CALL_ESTABLISHED))
        )
    {
        return 1;
    }
#endif
    else
        return 0;
}

#if(CONFIG_ANA_DAC_CLOSE_IN_IDLE == 1)
static uint8_t s_bt_aud_idle_stat = 0;
volatile static uint8_t s_aud_ADDA_open = 0;
#define AUDIO_DAC_ENALBE_BIT     (1 << 0)
#define AUDIO_ADC_ENALBE_BIT     (1 << 1)
#define AUDIO_ADDA_DSP_RSP_TIMEOUT_BIT  (1 << 6)
#define AUDIO_ADDA_EN_RSP_BIT    (1 << 7)
void aud_ana_dac_close_in_idle(void)
{
    t_mailbox_ctrl mbx_ctrl;
    if(!bt_flag1_is_set(APP_AUDIO_WORKING_FLAG_SET) && !(get_system_idle_cnt()))
    {
        if(s_bt_aud_idle_stat == 0) 
        {
            os_printf("aud_close_in_idle()\r\n");
            s_bt_aud_idle_stat = 1;
            set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_BT_IDLE,0,0,0);
            write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,0);
        }
    }    
}
#endif
void aud_open(void)
{
    t_mailbox_ctrl mbx_ctrl;
    //os_printf("aud_open()\r\n");
#if(CONFIG_ANA_DAC_CLOSE_IN_IDLE == 1)
    s_bt_aud_idle_stat = 0;
#endif
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_DAC_ENABLE | MAILBOX_CMD_NEED_RSP_FLAG | MAILBOX_CMD_FAST_RSP_FLAG, ADDA_ENABLE,0,0);
    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,1);
    aud_wait_for_dsp_resp();
    /* if dsp response timeout,AUDIO_ADDA_DSP_RSP_TIMEOUT_BIT will be set,MCU recursive call this funciotn */
    if((s_aud_ADDA_open & AUDIO_ADDA_DSP_RSP_TIMEOUT_BIT))
    {
        s_aud_ADDA_open &= ~AUDIO_ADDA_DSP_RSP_TIMEOUT_BIT;
        aud_open();  /* Recursive call */
    }

#ifdef CONFIG_TWS
    if (!bt_flag1_is_set(APP_FLAG_WAVE_PLAYING|APP_FLAG_CALL_ESTABLISHED|APP_FLAG_HFP_CALLSETUP|APP_FLAG_SCO_CONNECTION) 
        && bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
    {
        audio_sync_init();
    }
#endif
}

void aud_dac_dma_enable(uint8 enable)
{
    t_mailbox_ctrl mbx_ctrl;
    set_mailbox_param(&mbx_ctrl, MAILBOX_CMD_AUDIO_DAC_DMA_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, enable,0,0);
    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,1);
}

void aud_set_dac_opened(uint8 enable)
{
    //os_printf("set_dac_opened(%d)\r\n",enable);
    s_aud_ADDA_open &= ~AUDIO_DAC_ENALBE_BIT;
    if(enable)
        s_aud_ADDA_open |= AUDIO_DAC_ENALBE_BIT;        
}
void aud_set_adc_opened(uint8 enable)
{
    //os_printf("set_adc_opened(%d)\r\n",enable);
    s_aud_ADDA_open &= ~AUDIO_ADC_ENALBE_BIT;        
    if(enable)
        s_aud_ADDA_open |= AUDIO_ADC_ENALBE_BIT;        
}
void aud_set_adda_en_response(void)
{
    s_aud_ADDA_open |=  AUDIO_ADDA_EN_RSP_BIT;           
}
/* 
*  When MCU sets cmd to dsp with MAILBOX_CMD_NEED_RSP_FLAG,
*  MCU must wait for response of DSP;
*/
void aud_mcu2dsp_cmd_timeout(void)
{
    s_aud_ADDA_open |= AUDIO_ADDA_EN_RSP_BIT;   
    s_aud_ADDA_open |= AUDIO_ADDA_DSP_RSP_TIMEOUT_BIT;
}
void aud_wait_for_dsp_resp(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    /* Start task of timeout,Once expired,Set the AUDIO_ADDA_EN_RSP_BIT and AUDIO_ADDA_DSP_RSP_TIMEOUT_BIT */
    jtask_schedule(sys_hdl->app_audio_task, 500, (jthread_func)aud_mcu2dsp_cmd_timeout, (void *)NULL);
    /* Wait for the response of dsp ,or timeout callback */
    while(!(s_aud_ADDA_open & AUDIO_ADDA_EN_RSP_BIT));
    s_aud_ADDA_open &= ~AUDIO_ADDA_EN_RSP_BIT;
    jtask_stop(sys_hdl->app_audio_task);
}
void aud_close(void)
{
    t_mailbox_ctrl mbx_ctrl;
    os_printf("aud_close()\r\n");
#ifdef CONFIG_TWS
    tws_clear_a2dp_buffer(); // It must be excuted after [set_flag_sbc_buffer_play(0)].
#endif
    set_flag_sbc_buffer_play(0);

    //s_aud_ADDA_open &= ~(AUDIO_DAC_ENALBE_BIT);
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_DAC_ENABLE | MAILBOX_CMD_NEED_RSP_FLAG,ADDA_DISABLE,0,0);
    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,1);
#if(CONFIG_ANA_DAC_CLOSE_IN_IDLE == 1)
    set_system_idle_cnt(200); // 200 * 10ms = 2s
#endif
    aud_wait_for_dsp_resp();
    /* if dsp response timeout,AUDIO_ADDA_DSP_RSP_TIMEOUT_BIT will be set,MCU recursive call this funciotn */
    if((s_aud_ADDA_open & AUDIO_ADDA_DSP_RSP_TIMEOUT_BIT))
    {
        s_aud_ADDA_open &= ~AUDIO_ADDA_DSP_RSP_TIMEOUT_BIT;
        aud_close(); /* Recursive call */
    }
}
void aud_mic_open(uint8 enable)
{
    t_mailbox_ctrl mbx_ctrl;
    os_printf("mic_open(%d)\r\n",enable);
    //s_aud_ADDA_open &= ~(AUDIO_ADC_ENALBE_BIT);
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_ADC_ENABLE | MAILBOX_CMD_NEED_RSP_FLAG,enable,0,0);
    write_mailbox_ctrl(MCU2DSP_ADC_CTRL,&mbx_ctrl,1);
    aud_wait_for_dsp_resp();
    /* if dsp response timeout,AUDIO_ADDA_DSP_RSP_TIMEOUT_BIT will be set,MCU recursive call this funciotn */
	if((s_aud_ADDA_open & AUDIO_ADDA_DSP_RSP_TIMEOUT_BIT))
    {
        s_aud_ADDA_open &= ~AUDIO_ADDA_DSP_RSP_TIMEOUT_BIT;
        aud_mic_open(enable); /* Recursive call */
    }

}
void aud_mic_volume_set( uint8 volume )
{
    t_mailbox_ctrl mbx_ctrl;
    //os_printf("mic_vol:%d\r\n",volume);
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_ADC_ANALOG_VOLUME_SET,(uint32)volume,0,0);
    write_mailbox_ctrl(MCU2DSP_ADC_CTRL,&mbx_ctrl,1);     
}

void aud_adc_dig_volume_set( uint8 volume )
{
    t_mailbox_ctrl mbx_ctrl;
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_ADC_DIGITAL_VOLUME_SET,(uint32)volume,0,0);
    write_mailbox_ctrl(MCU2DSP_ADC_CTRL,&mbx_ctrl,1); 
}

void aud_aux_ana_gain_set( uint8 gain )
{
    t_mailbox_ctrl mbx_ctrl;
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_AUX_ANA_GAIN_SET,(uint32)gain,0,0);
    write_mailbox_ctrl(MCU2DSP_ADC_CTRL,&mbx_ctrl,1); 
}

void aud_aux_dig_gain_set( uint8 gain )
{
    t_mailbox_ctrl mbx_ctrl;
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_AUX_DIGI_GAIN_SET,(uint32)gain,0,0);
    write_mailbox_ctrl(MCU2DSP_ADC_CTRL,&mbx_ctrl,1); 
}


void aud_volume_set(int8 volume)
{
#if 1
    t_mailbox_ctrl mbx_ctrl;
    //os_printf("vol:%d\r\n",volume);
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_DAC_VOLUME_SET | MAILBOX_CMD_FAST_RSP_FLAG,(uint32)volume,0,0);
    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,1);
#else
#ifdef CONFIG_DRIVER_DAC
    dac_volume_set(volume);
#endif
#endif
}

int32 aud_initial(uint32 freq, uint32 channels, uint32 bits_per_sample)
{
    t_mailbox_ctrl mbx_ctrl;
    //os_printf("aud ini:%d,%d,%d\r\n",freq,channels,bits_per_sample);
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_DAC_INIT,freq, channels,bits_per_sample);
    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,1);
    return 0;
}
int32 aud_adc_initial(uint32 freq, uint32 channels, uint32 bits_per_sample)
{
#if 0
    t_mailbox_ctrl mbx_ctrl;
    //os_printf("adc ini:%d,%d,%d\r\n",freq,channels,bits_per_sample);
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_ADC_INIT,freq, channels,bits_per_sample);
    write_mailbox_ctrl(MCU2DSP_ADC_CTRL,&mbx_ctrl,1);
#endif
#if(CONFIG_APP_AEC == 1)
    rb_init( &pcm_ctrl_blk.aud_rb,(uint8 *)pcm_ctrl_blk.data_buff,0,PCM_BUFF_LEN);
    pcm_ctrl_blk.channels = channels;
    pcm_fill_buffer((uint8 *)pcm_ctrl_blk.data_buff, (PCM_BUFF_LEN>>1));
#endif
    return 0;
}
/*
    Copy the volume talbe from the env cfg to the dsp;
*/
void aud_volume_table_init(void)
{
    t_mailbox_ctrl mbx_ctrl;
    //aud_volume_t *vol_tbl_ptr;
    aud_volume_t vol_tbl_ptr[AUDIO_VOLUME_MAX+1];
    app_env_handle_t env_h = app_env_get_handle();

    // Warning:env_h->env_cfg.feature.hfp_vol is not 2/4 alignment;
    //vol_tbl_ptr = (aud_volume_t *)&env_h->env_cfg.feature.hfp_vol;
    memcpy((uint8 *)vol_tbl_ptr,(uint8 *)&env_h->env_cfg.feature.hfp_vol,(AUDIO_VOLUME_MAX+1)*sizeof(aud_volume_t));
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_DAC_VOL_TBL | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)vol_tbl_ptr, sizeof(aud_volume_t), AUDIO_HFP_MODE);
    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,1);

    // Warning:env_h->env_cfg.feature.a2dp_vol is not 2/4 alignment;
    //vol_tbl_ptr = (aud_volume_t *)&env_h->env_cfg.feature.a2dp_vol;
    memcpy((uint8 *)vol_tbl_ptr,(uint8 *)&env_h->env_cfg.feature.a2dp_vol,(AUDIO_VOLUME_MAX+1)*sizeof(aud_volume_t));
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_DAC_VOL_TBL | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)vol_tbl_ptr, sizeof(aud_volume_t), AUDIO_A2DP_MODE);
    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,1);

    // Warning:env_h->env_cfg.feature.linein_vol is not 2/4 alignment;
    //vol_tbl_ptr = (aud_volume_t *)&env_h->env_cfg.feature.linein_vol;
    memcpy((uint8 *)vol_tbl_ptr,(uint8 *)&env_h->env_cfg.feature.linein_vol,(AUDIO_VOLUME_MAX+1)*sizeof(aud_volume_t));
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_DAC_VOL_TBL | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)vol_tbl_ptr, sizeof(aud_volume_t), AUDIO_AUX_MODE);
    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,1);
}

void aud_dac_mic_update_init(void)
{
    app_env_handle_t  env_h = app_env_get_handle();

    audio_exchange_info->adc_fifo_fill_size = 0; //bit0:dac differ   bit1:L+R   bit2:mic single
    if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_DAC_DIFFER)
        audio_exchange_info->adc_fifo_fill_size |= (1 << 0);
    if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_L_is_LplusR)
        audio_exchange_info->adc_fifo_fill_size |= (1 << 1);
    if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_MIC_SINGLE_ENABLE))
        audio_exchange_info->adc_fifo_fill_size |= (1 << 2);
    os_printf("adc_fifo_fill_size:0x%x\r\n",audio_exchange_info->adc_fifo_fill_size);
}

#if (CONFIG_AUDIO_USED_MCU == 0)
void BK3000_Ana_Line_enable( uint8 enable)
{
    t_mailbox_ctrl mbx_ctrl;
    app_env_handle_t  env_h = app_env_get_handle();

    os_printf("Line_enable(%d)\r\n",enable);
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
    if (bt_flag1_is_set(APP_FLAG_LINEIN)
        && bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE)
        && (get_tws_prim_sec()==TWS_PRIM_SEC_PRIMARY))
    {
    
        sdadc_enable(enable);	
        write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,1);
        wait_mailbox_ready(MCU2DSP_DAC_CTRL);
    }
    else
#endif
    {
        aud_aux_ana_gain_set(env_h->env_cfg.feature.vol_linein_ana);
        aud_aux_dig_gain_set(env_h->env_cfg.feature.vol_linein_dig&0x3f);
        set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_AUX_ENABLE,enable,0,0);
        write_mailbox_ctrl(MCU2DSP_ADC_CTRL,&mbx_ctrl,1);
        Delay(4000);
        set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_BT_IDLE,0,0,0);
        write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,1);
    }
    return;
}
#endif

void aud_dsp_mode_mp3_set(void) // transfer status to DSP
{
    t_mailbox_ctrl mbx_ctrl;
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_TF_USB,0,0,0);
    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,1);
    return;
}

uint32 pcm_get_buffer_size(void)
{
    return((s_aud_ADDA_open & (AUDIO_ADC_ENALBE_BIT))?audio_exchange_info->adc_fifo_free_size:0);
}
uint32 pcm_get_data_size(void)
{
    return((s_aud_ADDA_open & (AUDIO_ADC_ENALBE_BIT))?audio_exchange_info->adc_fifo_fill_size:0);
}    
uint32 aud_get_buffer_size(void)
{
#if 1
    return((s_aud_ADDA_open & (AUDIO_DAC_ENALBE_BIT))?audio_exchange_info->dac_fifo_free_size:0);
#else
    t_mailbox_ctrl mbx_ctrl;
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_DAC_PCM_CHECK, 0, 0, 0);
    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,0);
    return READ_MAILBOX_REG_SEG(MAILBOX_BASEADDR + MCU2DSP_DAC_CTRL*MAILBOX_OFFSET + MAILBOX_P1_SEG);
#endif
}
uint32 aud_get_buffer_used_size(void)
{
#if 0
	return((s_aud_ADDA_open & (AUDIO_DAC_ENALBE_BIT))?audio_exchange_info->dac_fifo_fill_size:0);
#else
    if(s_aud_ADDA_open & AUDIO_DAC_ENALBE_BIT)
    {
	    t_mailbox_ctrl mbx_ctrl;
	    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_DAC_PCM_AVARIABLE | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0);
	    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,0);
	    wait_mailbox_ready(MCU2DSP_DAC_CTRL);
	    return READ_MAILBOX_REG_SEG(MAILBOX_BASEADDR + MCU2DSP_DAC_CTRL*MAILBOX_OFFSET + MAILBOX_P1_SEG);
    }
    else
    {
        return 0;
    }
#endif
}

static uint8 adc_first_discard_flag = 0;
static uint8 adc_first_discard_cnt = 0;

#define ADC_FIRST_DISCARD_THRESHOLD     120     

void aud_discard_adc_clear_flag(void)
{
    adc_first_discard_flag  = 0;  
    adc_first_discard_cnt = 0;
}
void aud_discard_adc_data(uint8* buff,uint16 size)
{
    if(adc_first_discard_flag == 0) //we discard the adc data about the first 160 * 7.5ms for the opening noise
    {
        adc_first_discard_cnt++;
        memset(buff, 0, size);
        if(adc_first_discard_cnt >= ADC_FIRST_DISCARD_THRESHOLD)
        {
            adc_first_discard_flag = 1;
        }
    }
}
void aud_read_buffer(uint8 *buff, uint32 size)
{
    t_mailbox_ctrl mbx_ctrl;
    //memset(buff, 0, size);
    if(!(s_aud_ADDA_open & AUDIO_ADC_ENALBE_BIT)) 
        return;
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_ADC_PCM_READ | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)buff, size, 0);
    write_mailbox_ctrl(MCU2DSP_ADC_CTRL,&mbx_ctrl,1);
    aud_discard_adc_data(buff,size);
}

void aud_fill_buffer(uint8 *buff,uint16 size)
{
    t_mailbox_ctrl mbx_ctrl;
    if(!(s_aud_ADDA_open & AUDIO_DAC_ENALBE_BIT)) 
        return;
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_DAC_PCM_WRITE | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)buff, size, 0);
    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,1);
}
#if(CONFIG_APP_AEC == 1)
void pcm_read_buffer(uint8 *buff,uint16 size)
{
    uint16 len;
    len = rb_get_buffer_pcm(&(pcm_ctrl_blk.aud_rb),buff,size);
    if(len < size)
    {
        //os_printf("===adc empty!!!\r\n");
        memset(buff+len,0,size-len);
    }
}
void pcm_fill_buffer(uint8 *buff,uint16 size)
{   
    rb_fill_buffer(&(pcm_ctrl_blk.aud_rb),buff,size,ADC_FILL);
}
#endif
void aud_set_dac_frac_coef(uint32 coef)
{
    t_mailbox_ctrl mbx_ctrl;
    set_mailbox_param(&mbx_ctrl,MAILBOX_CMD_AUDIO_DAC_FRAC_COEF,coef, 0,0);
    write_mailbox_ctrl(MCU2DSP_DAC_CTRL,&mbx_ctrl,0);
}


uint8_t get_audio_dac_clk_state(void)
{
    return s_aud_dac_state;
}
/* dac clk coef modified for track mobile clk;

sbc_node_stat = result of sbc node monitor;

sbc_freq means sample frequency;
0 : 44100;
1 : 48000;

sbc_clk means:
0 : clk is slow;
1 : clk is normal;
2 : clk is fast;
*/
void RAM_CODE audio_dac_clk_process(void)
{
    uint8 sbc_node_stat = sbc_node_buff_monitor();
    uint8 sbc_freq  = (sbc_node_stat >> 2)&0x01;
    uint8 sbc_clk   = (sbc_node_stat & 0x03);
    uint32 *p_dac_clk_coef = s_aud_dac_fraccoef[sbc_freq];
    switch(s_aud_dac_state)
    {
        case AUDIO_DAC_NORM:
                if(s_current_aud_dac_state != AUDIO_DAC_NORM)
                {
                    BK3000_AUD_DAC_FRACCOEF = p_dac_clk_coef[1];  
                    //aud_set_dac_frac_coef(p_dac_clk_coef[1]);
                }
                s_current_aud_dac_state = AUDIO_DAC_NORM;
                if(sbc_clk == 0) 
                {
                    s_aud_dac_state =  AUDIO_DAC_SLOW;         // Local CLK is slow
                }
                else if(sbc_clk == 2)
                {
                    s_aud_dac_state =  AUDIO_DAC_FAST;         // Local CLK is fast   
                }
                break;
        case AUDIO_DAC_SLOW:
                if(s_current_aud_dac_state != AUDIO_DAC_SLOW)
                {
                    BK3000_AUD_DAC_FRACCOEF = p_dac_clk_coef[2];  //AUDIO_DIV_441K_SLOW;  
                    //aud_set_dac_frac_coef(p_dac_clk_coef[2]); 
                }
                s_current_aud_dac_state = AUDIO_DAC_SLOW;
                if(sbc_clk == 0) 
                {
                    s_aud_dac_state =  AUDIO_DAC_SLOWER;    // Local CLK is very slow
                }
                else if(sbc_clk == 2)
                {
                    s_aud_dac_state =  AUDIO_DAC_NORM;        // Local CLk between norm and slow    
                }
                
                break;
        case AUDIO_DAC_SLOWER:
                if(s_current_aud_dac_state != AUDIO_DAC_SLOWER)
                {
                    BK3000_AUD_DAC_FRACCOEF = p_dac_clk_coef[2] - DAC_CLK_ADJUST_STEP; 
                    //aud_set_dac_frac_coef(p_dac_clk_coef[2] - DAC_CLK_ADJUST_STEP);  
                }
                s_current_aud_dac_state = AUDIO_DAC_SLOWER;
                if(sbc_clk == 0) 
                {
                    s_aud_dac_state =  AUDIO_DAC_SLOWER;    // Local clk is very slow,but not follow the mobile yet;   
                }
                else if(sbc_clk == 2)
                {
                    s_aud_dac_state =  AUDIO_DAC_SLOW;        // Local clk between  veryslow and slow            
                }
                break;
        case AUDIO_DAC_FAST:
                if(s_current_aud_dac_state != AUDIO_DAC_FAST)
                {
                    BK3000_AUD_DAC_FRACCOEF = p_dac_clk_coef[0];  
                    //aud_set_dac_frac_coef(p_dac_clk_coef[0]);  
                }
                s_current_aud_dac_state = AUDIO_DAC_FAST;
                if(sbc_clk == 0) 
                {
                    s_aud_dac_state =  AUDIO_DAC_NORM;        // Local clk between fast and norm
                }
                else if(sbc_clk == 2)
                {
                    s_aud_dac_state =  AUDIO_DAC_FASTER;    // Local clk is very fast
                }
                break;
        case AUDIO_DAC_FASTER:
                if(s_current_aud_dac_state != AUDIO_DAC_FASTER)
                {
                    BK3000_AUD_DAC_FRACCOEF = p_dac_clk_coef[0] + DAC_CLK_ADJUST_STEP; 
                    //aud_set_dac_frac_coef(p_dac_clk_coef[0] + DAC_CLK_ADJUST_STEP);   
                }
                s_current_aud_dac_state = AUDIO_DAC_FASTER;
                if(sbc_clk == 0) 
                {
                    s_aud_dac_state =  AUDIO_DAC_FAST;        // Local clk between veryfast and fast
                }
                else if(sbc_clk == 2)
                {
                    s_aud_dac_state =  AUDIO_DAC_FASTER;    // Local clk is very fast,but not follow the mobile yet; 
                }

                break;
        default:
                break;
    }    
}

void aud_volume_mute(uint8 enable)
{
    return;
}
void aud_mic_mute(uint8 enable)
{
    return;
}

#if 1
#if (CONFIG_APP_MP3PLAYER == 1)
extern wavInfo wav_info;
extern short  *aulawsmpl;
extern short  *alawtbl;
extern short  *ulawtbl;

void RAM_CODE wav_fill_buffer( uint8 *buff, uint16 size )
{
    int32 tmp;
    int16 i;
    int16 fmtTag;
    int16 bits;
    int16 nSmpls;
    uint8 *buff_st = NULL;
    uint8 *tmp_buf=NULL;

    fmtTag = wav_info.fmtTag;
    bits = wav_info.bits;
    switch(bits)
    {
        case 8:
            nSmpls = size;
            if(fmtTag == 0x06)//a-law
            {
                for(i=0;i<nSmpls;i++)
                {
                    aulawsmpl[i] = alawtbl[buff[i]^0x80];
                    tmp = aulawsmpl[i] << 3;
                    aulawsmpl[i]= f_sat(tmp);
                }
            }
            else if(fmtTag == 0x07) //u-law
            {
                for(i=0;i<nSmpls;i++)
                {
                    aulawsmpl[i] = ulawtbl[buff[i]];
                    tmp = aulawsmpl[i] << 2;
                    aulawsmpl[i]= f_sat(tmp);
                    //aulawsmpl[i] <<= 2;
                }

            }
            else//linear
            {
                for(i=0;i<nSmpls;i++)
                {
                    //tmp = (buff[i]-127)<<8;
                    //aulawsmpl[i]= f_sat(tmp);
                    aulawsmpl[i] = ((short)(buff[i]-128))<<8;
                }
            }
            break;
        case 16:
            nSmpls = size>>1;
            for(i=0;i<nSmpls;i++)
            {
                aulawsmpl[i] = (buff[2*i])|(buff[2*i+1]<<8);
            }
            break;
        case 24://Not Support!!!!!
            nSmpls = size / 3;
            for(i=0;i<nSmpls;i++)
            {
                aulawsmpl[i] = (buff[3*i+1])|(buff[3*i+2]<<8);
            }
            break;
        case 32://Not Support!!!!!
            nSmpls = size>>2;
            for(i=0;i<nSmpls;i++)
            {
                aulawsmpl[i] = (buff[4*i+2])|(buff[4*i+3]<<8);
            }
            break;
        default:
            nSmpls = size>>1;
            for(i=0;i<nSmpls;i++)
            {
                aulawsmpl[i] = (buff[2*i])|(buff[2*i+1]<<8);
            }
            break;
    }

    if(wav_info.ch == 1)
    {
        uint16 i = 0;
        nSmpls = nSmpls << 1;
        buff_st = jmalloc( (nSmpls << 1),0);
        uint8 *src = (uint8 *)&aulawsmpl[0];

        if(buff_st == NULL)
        {
            os_printf("wav jmalloc fail!!!\r\n");
            return;
        }
        while(i < nSmpls)
        {
            *(buff_st + 2*i) = *(src + i);
            *(buff_st + 2*i + 1) = *(src + i + 1);
            *(buff_st + 2*i + 2) = *(src + i);
            *(buff_st + 2*i + 3) = *(src + i + 1);
            i += 2;
        }
    }

    if(buff_st == NULL)
        tmp_buf = (uint8 *)&aulawsmpl[0];
    else
        tmp_buf = buff_st;

    //rb_fill_buffer( &audio_ctrl_blk.aud_rb, tmp_buf, nSmpls<<1, 4 );
    aud_fill_buffer(tmp_buf, nSmpls<<1);
    if(buff_st)
        jfree(buff_st);

}

#endif

#define  PA_MUTE_STATUS 1
#define  PA_UNMUTE_STATUS 0
extern uint8 mutePa;
static aud_mute_cfg_t aud_mute_cfg;
uint8 pa_delay_unmute=0;
uint8 pa_delay_mute=0;
void aud_mute_init( void )
{
    app_env_handle_t env_h = app_env_get_handle();

    if( env_h->env_cfg.used == 0x01 )
    {
        aud_mute_cfg.mute_pin = env_h->env_cfg.system_para.pamute_pin;
        aud_mute_cfg.mute_high_flag
            = ((env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_PAMUTE_HIGH) >> 6 );
        aud_mute_cfg.mute_status = 2;
        aud_mute_cfg.mute_outside =
            ((env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_PA_ENABLE) >> 5 );
        aud_mute_cfg.shade_flag =
            ((env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_SHADE_OUT) >> 10 );
        aud_mute_cfg.auto_mute_flag =
            (!!(env_h->env_cfg.feature.feature_flag & APP_ENV_FEATURE_FLAG_FAST_MUTE));
    }
    else
    {
        aud_mute_cfg.mute_pin       = PAMUTE_GPIO_PIN;
        aud_mute_cfg.mute_high_flag = 0;
        aud_mute_cfg.mute_status    = 2;
        aud_mute_cfg.mute_outside   = 0;
        aud_mute_cfg.shade_flag     = 0;
        aud_mute_cfg.auto_mute_flag = 0;
    }

    if( aud_mute_cfg.mute_outside )
    {
        gpio_config(aud_mute_cfg.mute_pin, 1);
        if (aud_mute_cfg.mute_high_flag)
            gpio_output(aud_mute_cfg.mute_pin, 1);
        else
            gpio_output(aud_mute_cfg.mute_pin, 0);
    }
}

static void pa_mute(void)
{
    if(aud_mute_cfg.mute_outside == 1)
    {
        if(aud_mute_cfg.mute_high_flag)  // bit7 = 1: high effect
            gpio_output(aud_mute_cfg.mute_pin, 1);
        else
            gpio_output(aud_mute_cfg.mute_pin, 0);

        aud_mute_cfg.mute_status = PA_MUTE_STATUS;
        os_printf("pa mute:%d\r\n",aud_mute_cfg.mute_pin);		
    }
}
static void pa_unmute(void)
{
    if(aud_mute_cfg.mute_outside == 1)
    {
        if( aud_mute_cfg.mute_high_flag )  // bit7 = 1: high effect
            gpio_output(aud_mute_cfg.mute_pin, 0);
        else
            gpio_output(aud_mute_cfg.mute_pin, 1);

        aud_mute_cfg.mute_status = PA_UNMUTE_STATUS;
        os_printf("pa unmute:%d\r\n",aud_mute_cfg.mute_pin);
    }
}

/*  1 -- direct PA mute  0 -- delay PA unmute */
void aud_PAmute_oper(uint8 enable)
{
    app_env_handle_t  env_h = app_env_get_handle();
    if(1 == enable )
    {
        pa_delay_unmute = env_h->env_cfg.feature.pa_unmute_delay_time;
        if(pa_delay_mute)
        {
          //  os_printf("****pa_delay_mute=%d\r\n",pa_delay_mute);	
            pa_delay_mute--;
            return;
        }
        if(aud_mute_cfg.mute_status != PA_MUTE_STATUS)
            pa_mute();
    }
    else if(0 == enable)
    {
        pa_delay_mute  = env_h->env_cfg.feature.pa_mute_delay_time;
        if(pa_delay_unmute)
        {
          //  os_printf("****pa_delay_unmute=%d\r\n",pa_delay_unmute);	
            pa_delay_unmute--;
            return;
        }
        if(bt_flag1_is_set(APP_FLAG_POWERDOWN))
            return;
        if((aud_mute_cfg.mute_status != PA_UNMUTE_STATUS))
         pa_unmute();
    }
    return;
}
extern volatile uint8 linein_audio_flag;
void aud_clr_PAmute(void)
{
    uint32 mode;
    mode = get_app_mode();
    if(mutePa)
    {
        aud_PAmute_oper(1);
    }
    else if(app_wave_playing())
    {
        aud_PAmute_oper(0);
    }
    else if(SYS_WM_BT_MODE == mode)
    {
	#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
    	if (app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_AUX_MODE_PAUSE_MUTE)
    		&&app_bt_slave_linein_get())
    	{
    		if (linein_get_play_status()&&player_vol_bt&&bt_flag1_is_set(APP_FLAG_MUSIC_PLAY))
				aud_PAmute_oper(0);
			else
				aud_PAmute_oper(1);
    	}
		else
	#endif		
		{ 
	        if( bt_flag2_is_set(APP_FLAG2_HFP_INCOMING)
	            ||((bt_flag1_is_set(APP_FLAG_HFP_OUTGOING|APP_FLAG_SCO_CONNECTION|APP_FLAG_CALL_ESTABLISHED|APP_FLAG_HFP_CALLSETUP)))
	            ||(a2dp_has_music()&&(player_vol_bt>0)&&(!bt_flag2_is_set(APP_FLAG2_MUTE_FUNC_MUTE))))
	        {
	            aud_PAmute_oper(0);
	        }
	        else
	        {
	            aud_PAmute_oper(1);
	        }
		}
    }
#if (CONFIG_APP_MP3PLAYER == 1)
    else if((SYS_WM_SD_MUSIC_MODE == mode)
        #ifdef CONFIG_APP_USB 
            ||(SYS_WM_UDISK_MUSIC_MODE == mode)
        #endif
        )
    {
        if(player_get_play_status()&&player_vol_bt)
            aud_PAmute_oper(0);
    #ifdef CONFIG_BLUETOOTH_COEXIST
        else if( bt_flag2_is_set(APP_FLAG2_HFP_INCOMING)
            ||((bt_flag1_is_set(APP_FLAG_HFP_OUTGOING|APP_FLAG_SCO_CONNECTION|APP_FLAG_CALL_ESTABLISHED|APP_FLAG_HFP_CALLSETUP))))
        {
            aud_PAmute_oper(0);
        }
    #endif
        else
            aud_PAmute_oper(1);
    }
    else if(SYS_WM_FM_MODE == mode)
    {

    }
#endif
    else if(SYS_WM_LINEIN_MODE == mode)
    {
        if(linein_get_play_status()&&player_vol_bt&&linein_audio_flag)
            aud_PAmute_oper(0);
    #ifdef CONFIG_BLUETOOTH_COEXIST
        else if( bt_flag2_is_set(APP_FLAG2_HFP_INCOMING)
            ||((bt_flag1_is_set(APP_FLAG_HFP_OUTGOING|APP_FLAG_SCO_CONNECTION|APP_FLAG_CALL_ESTABLISHED|APP_FLAG_HFP_CALLSETUP))))
        {
            aud_PAmute_oper(0);
        }
    #endif
        else
            aud_PAmute_oper(1);
    }


}

#ifdef CONFIG_TWS
void dac_init_clk(void)
{
    if(((BK3000_AUD_AUDIO_CONF >> 6) & 0x3) == 2) //44.1K
        set_dac_clk(AUDIO_DIV_441K);
    else if(((BK3000_AUD_AUDIO_CONF >> 6) & 0x3) == 3) //44.1K//48K
        set_dac_clk(AUDIO_DIV_48K);
    else
    {
    //os_printf("%s, 0x%x error! \n", __func__, BK3000_AUD_AUDIO_CONF);
    }
}

static uint32_t s_last_dac_val = 0;
void RAM_CODE set_dac_clk(uint32_t clk_val)
{
#if 0//def CONFIG_TWS
    if(tws_get_call_exist() == TRUE)
    {
        os_printf("CALL:0x%x\n",clk_val);
        return;
    }
#endif

    if(clk_val != s_last_dac_val)
    {
        BK3000_AUD_DAC_FRACCOEF = clk_val;
        s_last_dac_val = clk_val;
        //os_printf("\n\n.%s.%d-%d \n\n", __func__, get_tws_prim_sec(), clk_val);
        //os_printf(".sd%d;%d\n", clk_val, get_sbc_mem_pool_node_left());
    }    
}

inline uint32_t get_dac_clk(void)
{
    return BK3000_AUD_DAC_FRACCOEF;
}
#endif

#if 0//def CONFIG_DRIVER_DAC
static void dac_volume_set( int8 volume )
{
    env_aud_dc_offset_data_t* dc_offset = app_get_env_dc_offset_cali();
    aud_volume_t *vol_tbl_ptr;
    int16 dig_gain = 0;
    //uint32 dac_dig_reg = BK3000_AUD_DAC_CONF0;
    app_env_handle_t env_h = app_env_get_handle();
    app_handle_t app_h = app_get_sys_handler();

    if(bt_flag1_is_set(APP_FLAG_SCO_CONNECTION | APP_FLAG_WAVE_PLAYING))
    {
        vol_tbl_ptr = (aud_volume_t *)&env_h->env_cfg.feature.hfp_vol;
        os_printf("vol:hfp||wave=%d\r\n",volume);
    }
    else if (bt_flag1_is_set(APP_FLAG_LINEIN)
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
        && (!bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
#endif
        )
    {
        vol_tbl_ptr = (aud_volume_t *)&env_h->env_cfg.feature.linein_vol;
        os_printf("vol:line=%d\r\n",volume);
    }
#if (CONFIG_APP_MP3PLAYER == 1)
    else if( (SYS_WM_SD_MUSIC_MODE == app_h->sys_work_mode)
    #ifdef CONFIG_APP_USB 
        ||(SYS_WM_UDISK_MUSIC_MODE == app_h->sys_work_mode)
    #endif
        )
    {
        vol_tbl_ptr = (aud_volume_t *)&env_h->env_cfg.feature.a2dp_vol;
        os_printf("vol:sd/usb=%d\r\n",volume);
    }
#endif
    else // SYS_WM_BT_MODE
    {
        vol_tbl_ptr = (aud_volume_t *)&env_h->env_cfg.feature.a2dp_vol;
        os_printf("vol:a2dp=%d\r\n",volume);
    }

    uint32_t ana_gain = vol_tbl_ptr[volume & 0x1F].ana_gain & 0x1f;
    dig_gain += vol_tbl_ptr[volume & 0x1F].dig_gain & 0x3f;
	
	os_printf("dig_gain=0x%x,  ana_gain=0x%x\r\n",vol_tbl_ptr[volume & 0x1F].dig_gain,vol_tbl_ptr[volume & 0x1F].ana_gain);
#if 1//def CONFIG_APP_EQUANLIZER
    if(dig_gain<0) 
        dig_gain = 0;
#endif
	bt_flag2_operate(APP_FLAG2_SW_MUTE, !volume);

    if( volume > 0 )
    {
        aud_PAmute_oper(0);

        if( volume > AUDIO_VOLUME_MAX )
            volume = AUDIO_VOLUME_MAX;
        /***
        dac_dig_reg &= ~0x00FC0000;
        dac_dig_reg |= (((dig_gain) << 18)|(1 << 16));
        BK3000_AUD_DAC_CONF0 = dac_dig_reg;
        **/
        
        audio_dac_digital_gain_set(dig_gain);
        audio_dac_analog_gain_set(ana_gain, dc_offset->dac_l_dc_offset[ana_gain], dc_offset->dac_r_dc_offset[ana_gain]);
    }
    else  // volume = 0;
    {
        aud_PAmute_oper(1);

        if( volume <= AUDIO_VOLUME_MIN )
            volume = AUDIO_VOLUME_MIN;
		audio_dac_analog_gain_set(ana_gain, dc_offset->dac_l_dc_offset[ana_gain], dc_offset->dac_r_dc_offset[ana_gain]);
        if( aud_mute_cfg.mute_outside )
        {
            audio_dac_digital_gain_set(dig_gain);
        }
		else
		{
            audio_dac_digital_gain_set(dig_gain);
		}
    }
}
static void audio_dac_digital_gain_set(uint32 new_gain)
{
    uint32 dac_dig_reg;
    int8 step;
    uint32 old_gain = ((BK3000_AUD_DAC_CONF0 >> 18) & 0x03f);
    if(old_gain == new_gain)
        return;
    else
    {
        step = 2 *(new_gain > old_gain) - 1;
        dac_dig_reg = BK3000_AUD_DAC_CONF0;
        while(old_gain != new_gain)
        {
            old_gain += step;
            dac_dig_reg &= ~0x00FC0000;
            dac_dig_reg |= (((old_gain) << 18));//|(1 << 16)
            BK3000_AUD_DAC_CONF0 = dac_dig_reg;
            os_delay_ms(1);
        }
    }
}
static void audio_dac_analog_gain_set(uint32_t gain, uint32_t dc_offset_l, uint32_t dc_offset_r)
{
	uint32_t reg;

	os_printf("audio_dac_analog_gain_set:%d, %d, %d\r\n", gain, dc_offset_l, dc_offset_r);
	reg = (BK3000_A5_CONFIG & ~(0xFFFF << 16)) | (((dc_offset_l & 0xFF) << 16) | ((dc_offset_r & 0xFF) << 24));
	BK3000_A5_CONFIG = reg;

	reg = (BK3000_A6_CONFIG & ~(0x3F << 1)) | ((gain& 0x3F) << 1);
	BK3000_A6_CONFIG = reg;
}
#endif

#if (CONFIG_AUDIO_USED_MCU == 1)
static void adc_isr( void )
{
    #define AD_FIFO_LEN    (16 + 2)
    uint32 sts = BK3000_AUD_FIFO_STATUS;
    int32 pcmval = 0;
    //uint8 pcmbuf[16];
    int16 pcmbuf[AD_FIFO_LEN];
    int16 pcm_tmp = 0;
    //uint32 pcm_data32[AD_FIFO_LEN];
    int16 i = 0,size = 0;
    app_env_handle_t  env_h = app_env_get_handle();
    uint8 flag_L_is_LplusR = !!(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_L_is_LplusR);
    if(bt_flag1_is_set( APP_FLAG_LINEIN ))
    {
    #if 0//(CONFIG_CPU_CLK_OPTIMIZATION == 1)
        BK3000_set_clock(1, 0); // xtal 26M 
    #endif
    #if CONFIG_ADC_DMA
    	if (bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
        	return;
    #endif	
        if(sts  & bit_ADCL_INTR_FLAG)
        {
            for (i = 0; (!(sts & (bit_ADCL_FIFO_EMPTY|bit_DACR_FIFO_FULL|bit_DACL_FIFO_FULL))) && (i < AD_FIFO_LEN); i++)
            //for(i = 0; i <8; i++)
            //while(!(sts & (bit_ADCL_FIFO_EMPTY|bit_DACR_FIFO_FULL|bit_DACL_FIFO_FULL)))
            {
                pcmval = BK3000_AUD_ADC_PORT;
                if(bt_flag2_is_set(APP_FLAG2_SW_MUTE))
                {
                    pcmval = 0;
                    BK3000_AUD_DAC_PORT = 0;
                }
                else
                {
                #if 1//(CONFIG_APP_TOOLKIT_5 == 1)
                    if(flag_L_is_LplusR)
                    {
                        pcmbuf[0] = (int16) ((pcmval & 0xffff0000) >> 16);
                        pcmbuf[1] = (int16) (pcmval & 0x0000ffff);
                        pcmval = (pcmbuf[0] + pcmbuf[1]) >> 1;
                        pcmbuf[0] = f_sat(pcmval);
                        pcmbuf[1] = f_sat(pcmval * (-1));
                        pcmval = (((uint16)pcmbuf[0] << 16) | (uint16)pcmbuf[1]);
                        BK3000_AUD_DAC_PORT = pcmval;
                    }
                    else
                    {
                        //if((pcmval & 0x0ffff) == 0x8000) // for: (-32768) * (-1) = 32767
                        //    pcm_tmp = (pcmval ^ 0x0000ffff);
                        //else
                        //    pcm_tmp = ((pcmval ^ 0x0000ffff) + 1);
                        BK3000_AUD_DAC_PORT = (pcmval & 0xffff0000 ) | ((uint16) pcmval & 0xffff) ;
						//pcm_data32[i] = (pcmval & 0xffff0000 ) | ((uint16) pcm_tmp & 0xffff) ;
                    }
                #elif(CONFIG_EXT_PA_DIFF_EN == 1)
                    pcmbuf[0] = (int16) ((pcmval & 0xffff0000) >> 16);
                    pcmbuf[1] = (int16) (pcmval & 0x0000ffff);
                    pcmval = (pcmbuf[0] - pcmbuf[1]) >> 1;
                    pcmbuf[0] = f_sat(pcmval);
                    pcmbuf[1] = f_sat(pcmval * (-1));
                    pcmval = (((uint16)pcmbuf[0] << 16) | (uint16)pcmbuf[1]);
                    BK3000_AUD_DAC_PORT = pcmval;
                #else
                    if((pcmval & 0x0ffff) == 0x8000) // for: (-32768) * (-1) = 32767
                        pcm_tmp = (pcmval ^ 0x0000ffff);
                    else
                        pcm_tmp = ((pcmval ^ 0x0000ffff) + 1);
                    BK3000_AUD_DAC_PORT = (pcmval & 0xffff0000 ) | ((uint16) pcm_tmp & 0xffff) ;
                #endif
                }
                //sts = BK3000_AUD_FIFO_STATUS;
            }
            //size = i;
        }
    }
    
}

void aud_isr(int mode_sel )
{
#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#endif
    if( mode_sel == 0 )
    {       
        adc_isr();
    }
}
extern u_int32 XVR_analog_reg_save[];
//volatile uint32 BK3000_AUD_AUDIO_CONF_BAK = 0;
void BK3000_Ana_Line_enable( uint8 enable )
{
	app_env_handle_t  env_h = app_env_get_handle();
	uint16 i = 0;
	uint32_t reg = 0;
	os_printf("BK3000_Ana_Line_enable(%d)\r\n", enable);

	if(enable)
	{
		for(i = 0; i < 16; i++)
			AUD_WRITE_DACL(0, 0);
	#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE	
		aud_initial(44100, 2, 16);
	#else
		aud_initial(48000, 2, 16); // include open dac: BK3000_Ana_Dac_enable(1)
	#endif	
		BK3000_AUD_ADC_CONF0 &= ~(0x3f<<18); 
		BK3000_AUD_ADC_CONF0  = ((env_h->env_cfg.feature.vol_linein_dig&0x3f)<<18) | 0x00003F72;//ADC gain bit[23]-bit[18]
		BK3000_AUD_ADC_CONF0 |= (1<<sft_AUD_ADC_HPF1_BYPASS);
		BK3000_AUD_ADC_CONF1  = 0x811C3F72;
		BK3000_AUD_ADC_CONF2  = 0xC11A7EE3;
		
		// 1. enable digital ADC
		//BK3000_PMU_PERI_PWDS	&= ~bit_PMU_AUD_PWD;
		BK3000_AUD_AUDIO_CONF |= (1 << sft_AUD_ADC_ENABLE) | (1 << sft_AUD_LINE_ENABLE);
		// 2. enable analog ADC;
		BK3000_A6_CONFIG &= ~(0x03 << 20); //Power down of Left/Right channel ADC
		BK3000_A6_CONFIG |= (1<<24);  // line_en
		BK3000_A6_CONFIG &= ~(3<<22); //line_gain<23:22>
		BK3000_A6_CONFIG |= ((env_h->env_cfg.feature.vol_linein_ana)<<22);	//	line_gain<23:22>= 0/1/2/3  ????0/2/4/6dB
		
		// 1. enable digital DAC
		BK3000_AUD_AUDIO_CONF |= (1 << sft_AUD_DAC_ENABLE);
		//while(!(BK3000_AUD_FIFO_STATUS & (bit_DACL_FIFO_EMPTY | bit_DACR_FIFO_EMPTY)));
		//for(i = 0; i < 16; i++) 
		//	BK3000_AUD_DAC_PORT = 0x00400040;//0x00010001; //Ïû³ý¾²ÒôÊ±´æÔÚµÄ¹¾ààÉù
		//while(!(BK3000_AUD_FIFO_STATUS & (bit_DACL_FIFO_EMPTY | bit_DACR_FIFO_EMPTY)));
		//for(i = 0; i < 16; i++) 
		//	BK3000_AUD_DAC_PORT = 0x00000000;

		// 2. enable analog DAC
		BK3000_A7_CONFIG |= (1 << 6) | (3 << 23) | (3 << 29);
		os_delay_ms(1);
		BK3000_A5_CONFIG &= ~(1 << 9);
		os_delay_ms(50);

		// 3. auto calibration
		//Skip

		// 4. ramp
		reg = BK3000_A7_CONFIG;
		for(i = 0; i <= 0x3F; i += 1)
		{
			reg &= ~(0x3F << 15);
			reg |= (i << 15);
			BK3000_A7_CONFIG = reg;
			os_delay_ms(1);
		}

		os_delay_ms(10);
		BK3000_A7_CONFIG |= (1 << 25);
		os_delay_ms(400);
		BK3000_A7_CONFIG |= (1 << 21);
		os_delay_ms(10);
		BK3000_A7_CONFIG &= ~(1 << 22);
		os_delay_ms(10);
		BK3000_A7_CONFIG &= ~(1 << 26);
		os_delay_ms(10);
        #ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
            if (bt_flag1_is_set(APP_FLAG_LINEIN)&& !bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
        #endif
            {
                BK3000_AUD_FIFO_CONF |= (1 << sft_AUD_ADC_INT_EN);
            }
	}
	else
	{
		// 1. disable digital ADC
		BK3000_AUD_AUDIO_CONF &= ~(1 << sft_AUD_ADC_ENABLE);
		BK3000_AUD_FIFO_CONF &= ~(1 << sft_AUD_ADC_INT_EN);
		//BK3000_PMU_PERI_PWDS	|= (!!(BK3000_AUD_AUDIO_CONF & 0xF)) << 11;
		// 2. disable analog ADC;
		BK3000_A6_CONFIG |= (0x03 << 20);

		BK3000_A7_CONFIG  |= (1 << 22);
		os_delay_ms(100);
		BK3000_A7_CONFIG  |= (1 << 26);
		os_delay_ms(50);
		BK3000_A7_CONFIG &= ~(1 << 25);
		os_delay_ms(1);
		BK3000_A7_CONFIG &= ~(1 << 21);
		os_delay_ms(1);

	#if 0
		reg = BK3000_A7_CONFIG;
		for(i = ((reg >> 15) & 0x3F); i >= 0; i -= 1)
		{
			reg &= ~(0x3F << 15);
			reg |= (i << 15);
			BK3000_A7_CONFIG = reg;
			os_delay_ms(1);
		}
	#endif

		// 1. disable analog DAC
		BK3000_A7_CONFIG &= ~(1 << 23);
		os_delay_ms(1);
		BK3000_A7_CONFIG &= ~(1 << 24);
		os_delay_ms(1);
		BK3000_A7_CONFIG &= ~((1 << 29) | (1 << 30));
		os_delay_ms(1);
		BK3000_A7_CONFIG &= ~(1 << 6);
		BK3000_A5_CONFIG |= (1 << 9);
		os_delay_ms(1);

		BK3000_A7_CONFIG &= ~(0x3F << 15);

		// 2. disable digital DAC
		BK3000_AUD_AUDIO_CONF &= ~(1 << sft_AUD_DAC_ENABLE);

		XVR_analog_reg_save[4] &= ~((1 << 24) | (1 << 26));
		BK3000_XVR_REG_0x04 	 = XVR_analog_reg_save[4];

	}
}
#endif

#endif

#if CONFIG_ADC_DMA
void adc_dma_initial(void)
{
    //os_printf("%s\n", __func__);

    pcm_ctrl_blk.adc_dma_conf.request_select  = 5                 ;  // 0(Software) 1(Uart_rx) 2(Uart_tx) 3(Pcm_rx) 4(Pcm_tx) 5(Aud_rx) 6(Aud_tx)
    pcm_ctrl_blk.adc_dma_conf.transfer_mode   = 2                 ;  // 0(Single) 1(Block) 2(SingleRepeat) 3(BlockRepeat)
    pcm_ctrl_blk.adc_dma_conf.dst_addr_incr   = 3                 ;  // 0/1(No change) 2(Decrease) 3(Increase)
    pcm_ctrl_blk.adc_dma_conf.src_addr_incr   = 0                 ;  // 0/1(No change) 2(Decrease) 3(Increase)
    pcm_ctrl_blk.adc_dma_conf.dst_data_type   = 2                 ;  // 0(8 bits) 1(16 bits) 2(32 bits)
    pcm_ctrl_blk.adc_dma_conf.src_data_type   = 2                 ;  // 0(8 bits) 1(16 bits) 2(32 bits)
    pcm_ctrl_blk.adc_dma_conf.trigger_type    = 1                 ;  // 0(Posedge)  1(High Level)
    pcm_ctrl_blk.adc_dma_conf.interrupt_en    = 0                 ;  // 0(Disable)  1(Enable)
    pcm_ctrl_blk.adc_dma_conf.soft_request    = 0                 ;  // 0(Hardware) 1(Software)
    pcm_ctrl_blk.adc_dma_conf.transfer_size   = 0                 ;  // DMA Transfer Length
    
    pcm_ctrl_blk.adc_dma_conf.src_address     = (uint32) &(BK3000_AUD_ADC_PORT);  // DMA Source Address
    pcm_ctrl_blk.adc_dma_conf.dst_address     = (uint32) (&(pcm_ctrl_blk.data_buff[0]) + PCM_BUFF_LEN-4); // DMA Destination Address
    pcm_ctrl_blk.adc_dma_conf.src_addr_top    = 0                 ;  // DMA Source Top Boundary
    pcm_ctrl_blk.adc_dma_conf.src_addr_bottom = 0                 ;  // DMA Source Bottom Boundary
    pcm_ctrl_blk.adc_dma_conf.dst_addr_top    = (uint32) &(pcm_ctrl_blk.data_buff[0]);  // DMA Destination Top Boundary
    pcm_ctrl_blk.adc_dma_conf.dst_addr_bottom = (uint32) (&(pcm_ctrl_blk.data_buff[0]) + PCM_BUFF_LEN-4);  // DMA Destination Bottom Boundary
    pcm_ctrl_blk.adc_dma_conf.dma_enable      = 0                 ;

    //adc_dma_reset();
}

void adc_dma_start(void)
{
    //os_printf("adc_dma_start()\n");
    
    pcm_ctrl_blk.adc_dma_conf.dma_enable = 1;
    start_dma_transfer(DMA_CHN_4, &pcm_ctrl_blk.adc_dma_conf);
}

void adc_dma_stop(void)
{
    //os_printf("adc_dma_stop()\n");
    
    pcm_ctrl_blk.adc_dma_conf.dma_enable = 0;
    start_dma_transfer(DMA_CHN_4, &pcm_ctrl_blk.adc_dma_conf);
}

void adc_dma_reset(void)
{
    //os_printf("adc_dma_reset()\n");
    
    pcm_ctrl_blk.adc_dma_conf.dma_enable = 1;
    start_dma_transfer(DMA_CHN_4, &pcm_ctrl_blk.adc_dma_conf);
    
    pcm_ctrl_blk.adc_dma_conf.dma_enable = 0;
    start_dma_transfer(DMA_CHN_4, &pcm_ctrl_blk.adc_dma_conf);
}

int adc_init(uint32 freq, uint32 channels, uint32 bits_per_sample)
{
    //os_printf("adc_init(%d,%d,%d)\n", freq, channels, bits_per_sample);
    //adc_initial(freq, bits_per_sample);
    
    adc_dma_initial();
    adc_dma_stop();
    
    pcm_ctrl_blk.channels = channels;
    
    //pcm_fill_buffer((uint8 *)pcm_ctrl_blk.data_buff, (PCM_BUFF_LEN>>1));
    //aud_mic_volume_set(40);
    return 0;
}

int DRAM_CODE adc_dma_get_data(uint8 *buf, uint32 buf_len)
{
    uint32 cur_len;
    //uint32 i=0;
    driver_ringbuff_t *rb = &pcm_ctrl_blk.aud_rb;
    //uint16 *data = (uint16 *)buf;
    
    if (rb->buffp == NULL) {
        //os_printf("Buffer does not exist!\r\n");
        return -1;
    }
    
    //for(i=0; i<20; i++)
    //    os_printf("data[%d]:0x%x\r\n", i, data[i]);
    //os_printf("cur_len:%d\r\n", cur_len);
    //os_printf("cur_ptr:%x\r\n", reg_DMA_CH1_DST_CUR_PTR);
    //os_printf("dst_ptr:%x\r\n", reg_DMA_CH1_DST_ADDR);

    

    //dma_set_dst_ptr(MICROPHONE_ADC_DMA_CHNL, (uint32)(reg_DMA_CH1_DST_CUR_PTR)-4);
    
    //if(!cur_len)
    return cur_len;
}

uint32 adc_isr_get_data(uint8* buf, uint32 buf_len)
{
	uint32 cur_len=0;
	driver_ringbuff_t *rb = &pcm_ctrl_blk.aud_rb;
	if (rb_get_buffer_size(rb) <= buf_len)
		cur_len = rb_get_buffer_pcm(rb,buf,buf_len);

	return cur_len;
}
#endif
