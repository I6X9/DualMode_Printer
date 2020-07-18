#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"

#ifdef CONFIG_DRIVER_ADC
void sdadc_init( void )
{   
    //BK3000_AUD_ADC_CONF0  = 0x00E93A22;
    BK3000_AUD_ADC_CONF0  = (58<<18)|(1<<16)|(1<<17)|0x00003A22; //     <23-18>
    BK3000_AUD_ADC_CONF1  = 0x8BBF3A22;
    BK3000_AUD_ADC_CONF2  = 0xC9E6751C;
    BK3000_AUD_AGC_CONF0  = 0x4A019465;
    BK3000_AUD_AGC_CONF1  = 0x02016C01;
    BK3000_AUD_AGC_CONF2  = 0x0F020940;
    BK3000_AUD_FIFO_CONF &= ~(0x1F << sft_AUD_ADC_WR_THRE);
    BK3000_AUD_FIFO_CONF |=  (8 << sft_AUD_ADC_WR_THRE);

    //BK3000_A5_CONFIG |= 0x20000000;

    //sdadc_volume_adjust(60); // 36
}

void sdadc_enable( uint8 enable )
{
    //os_printf("sdadc_enable. %d  %d  %d \r\n", BK3000_AUD_AUDIO_CONF_BAK, enable, (BK3000_AUD_AUDIO_CONF_BAK & (1 << sft_AUD_ADC_ENABLE)));

    if ((BK3000_AUD_AUDIO_CONF_BAK & (1 << sft_AUD_ADC_ENABLE)) && enable)
        return;

#ifdef CONFIG_BLUETOOTH_HFP
    //BK3000_Ana_Adc_enable(enable);
#endif

    if(enable)
    {
        //BK3000_AUD_FIFO_CONF |=  (1 << sft_AUD_ADC_INT_EN);
		BK3000_AUD_AUDIO_CONF_BAK |= (1 << sft_AUD_ADC_ENABLE);
        BK3000_AUD_AUDIO_CONF_BAK |= (1 << sft_AUD_LINE_ENABLE);
		BK3000_AUD_AUDIO_CONF = BK3000_AUD_AUDIO_CONF_BAK;
    #ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
        if (!bt_flag1_is_set(APP_FLAG_LINEIN)|| !bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
        {
        	// for test
			os_printf("Open ad INT_2\n\n");
            //BK3000_AUD_FIFO_CONF |=  (1 << sft_AUD_ADC_INT_EN);
            adc_int_open();
        }
    #else
        BK3000_AUD_FIFO_CONF |=  (1 << sft_AUD_ADC_INT_EN);
    #endif
    }
    else
    {
        BK3000_AUD_FIFO_CONF &= ~(1 << sft_AUD_ADC_INT_EN);
		BK3000_AUD_AUDIO_CONF_BAK &= ~(1 << sft_AUD_ADC_ENABLE);
        BK3000_AUD_AUDIO_CONF_BAK &= ~(1 << sft_AUD_LINE_ENABLE);
		BK3000_AUD_AUDIO_CONF = BK3000_AUD_AUDIO_CONF_BAK;
    }
}


// direct volume adjust, not linear
void sdadc_volume_adjust( uint8 volume )
{  
    int high, low;

    if(volume > SDADC_VOLUME_MAX)
        volume = SDADC_VOLUME_MAX;

	high = volume & 0x70;
	low = volume & 0x0f;
	if(low >0x0c)
	low = 0x0c;
	volume = high | low;


	BK3000_AUD_AGC_CONF2 &= ~(0x7F << sft_AUD_MAN_PGA_VALUE);
	BK3000_AUD_AGC_CONF2 |= (volume << sft_AUD_MAN_PGA_VALUE); // volume

	/*int high, low;

	if(volume > SDADC_VOLUME_MAX)
		volume = SDADC_VOLUME_MAX;

	high = volume / 12;
	low = volume - high*12;

	BK3000_AUD_AGC_CONF2 &= ~(0x7F << sft_AUD_MAN_PGA_VALUE);
	BK3000_AUD_AGC_CONF2 |= (high << (sft_AUD_MAN_PGA_VALUE + 4));
	BK3000_AUD_AGC_CONF2 |= (low << sft_AUD_MAN_PGA_VALUE);*/
}

void sdadc_isr( void )
{
}
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
inline void adc_int_open(void)
{
    //os_printf("%s\n", __func__);
    BK3000_AUD_FIFO_CONF |=  (1 << sft_AUD_ADC_INT_EN);
}

inline void adc_int_close(void)
{
    //os_printf("%s\n", __func__);
    BK3000_AUD_FIFO_CONF &=  ~(1 << sft_AUD_ADC_INT_EN);
}
#endif
#endif

#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
void sdadc_enable( uint8 enable )
{
    app_env_handle_t  env_h = app_env_get_handle();
    os_printf("sdadc_enable=%d\r\n",enable);
    if (enable)
    {
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
    }
    else
    {	
    	// 1. disable digital ADC
    	BK3000_AUD_AUDIO_CONF &= ~(1 << sft_AUD_ADC_ENABLE);
    	BK3000_AUD_FIFO_CONF &= ~(1 << sft_AUD_ADC_INT_EN);
    	//BK3000_PMU_PERI_PWDS	|= (!!(BK3000_AUD_AUDIO_CONF & 0xF)) << 11;
    	// 2. disable analog ADC;
    	BK3000_A6_CONFIG |= (0x03 << 20);
    }
}
#endif

