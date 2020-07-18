#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "bt_mini_sched.h"
#include "bk3000_gpio.h"
#include "sys_irq.h"
#include "hw_leds.h"
//#include "bootloader.h"
#ifdef BT_DUALMODE_RW
#include "rwprf_config.h"
#endif
#include "drv_tl420.h"
#ifdef BT_DUALMODE_RW
extern void appm_stop_advertising(void);
#endif
volatile uint32 sleep_tick  = 0;
volatile uint32 pwdown_tick = 0;
volatile uint8 usb_tf_aux_in_out_flag = 0;//防止在关机过程中拔掉U盘、TF卡及line-in产生的问题
volatile uint8 player_vol_bt = 0;
volatile uint8 player_vol_hfp = 0;
volatile uint8 flag_once_power_on = 0;
#ifdef CONFIG_TWS            
volatile uint16 flag_powerdown_end = 0;
#endif
#if (CONFIG_CHARGE_EN == 1)
volatile uint8 flag_power_charge = 0;
#endif
//volatile uint16 adcmute_cnt = 0;
extern void controller_init(void);
extern void uart_initialise(u_int32 baud_rate);
extern RAM_CODE void Beken_Uart_Rx(void);
extern RAM_CODE int msg_get(MSG_T *msg_ptr);
#ifdef BEKEN_OTA
extern uint32_t oad_updating_user_section_pro(void);
#endif
#ifdef BT_DUALMODE_RW
extern void rw_ble_init(void);
extern void rw_ble_schedule(void);
#endif

static void bsp_init(void) {
#ifdef CONFIG_IRDA
    IrDA_init();
#endif
#ifdef CONFIG_DRIVER_I2C
    i2c_init(100000, 1);
#endif
    //flash_init();
#if defined(CONFIG_BLUETOOTH_HFP) && defined(CONFIG_DRIVER_ADC)
    //sdadc_init();
#endif
    //saradc_reset();

#if (CONFIG_AUDIO_TRANSFER_DMA == 1)
    //dma_initial();
#endif
    os_printf("exit bsp_init\r\n");
}

static void host_init(void) {
    app_env_handle_t  env_h = app_env_get_handle();
	//BK3000_AUD_ADC_CONF0  = (((env_h->env_cfg.feature.vol_mic_dig&0x3f)<<18)|(BK3000_AUD_ADC_CONF0&(~(0x3f<<18))));
#if 1
    app_env_rf_pwr_set(0);
#if(CONFIG_RF_CALI_TYPE&(CALI_BY_8852_BIT|CALI_BY_JZHY_BIT))
    if(env_h->env_data.offset_bak<0x3f)
        env_h->env_cfg.system_para.frq_offset = env_h->env_data.offset_bak;
#endif
    BK3000_XVR_REG_0x0D =XVR_analog_reg_save[13] = (XVR_analog_reg_save[13] &~0x3f)|(env_h->env_cfg.system_para.frq_offset&0x3f);
    os_printf("freq_offset:%d\r\n",env_h->env_cfg.system_para.frq_offset);
#endif

#ifdef WROK_AROUND_DCACHE_BUG
    app_Dcache_initial();
#endif

    bsp_init();

#ifdef CONFIG_DRIVER_I2S
    i2s_init();
#else
    // Only initial adc once
    //FIXME: liaixing
    //aud_adc_initial(8000,1,16);
#endif

    app_init();
    j_stack_init(NULL);
    app_post_init();
    app_env_post_init();
    msg_init();
    msg_clear_all();
    saradc_calibration_first();
    /* WDT CLK 1/16 32K */
    CLEAR_WDT;
    BK3000_start_wdt(0xFFFF);				//wdt reset 0xc000:10s
    
    //set adc/dac vol para
    aud_adc_initial(8000,1,16);
    aud_volume_table_init();
    aud_adc_dig_volume_set(env_h->env_cfg.feature.vol_mic_dig);
    aud_mic_volume_set(env_h->env_cfg.system_para.vol_mic);
#ifdef CONFIG_BLUETOOTH_A2DP
    a2dp_volume_init( env_h->env_cfg.system_para.vol_a2dp );
#endif
#ifdef CONFIG_BLUETOOTH_HFP
    hf_volume_init( env_h->env_cfg.system_para.vol_hfp );
#ifdef CONFIG_APP_HALFDUPLEX
    app_hfp_agc_init( env_h->env_cfg.system_para.vol_mic );
    app_hfp_echo_cfg_ptr_set( &env_h->env_cfg.env_echo_cfg );
#endif
#endif

#if (CONFIG_CHARGE_EN == 1)
    flag_power_charge = 0;
    bt_flag2_operate(APP_FLAG2_CHARGE_POWERDOWN,0);
#endif

    app_linein_init();
    app_env_power_on_check();
    app_set_led_event_action( LED_EVENT_POWER_ON );
#if (CONFIG_APP_MP3PLAYER == 1)
#ifdef CONFIG_APP_SDCARD
    app_sd_init();
#endif
#ifdef CONFIG_APP_USB
    if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_USB_ENA)
    {
        usb_init(USB_HOST_MODE);
    }
#endif
#endif
	
	//audio_dac_analog_init(!(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_DAC_DIFFER));

#ifdef FM_ENABLE
    if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_FM_ENA)
        FM_IC_PowerDown();
#endif

#if (CONFIG_CHARGE_EN == 1)
    if(!bt_flag2_is_set(APP_FLAG2_CHARGE_POWERDOWN)||(!app_env_check_Charge_Mode_PwrDown() && (flag_power_charge==0)))
#endif
        app_wave_file_play_start(APP_WAVE_FILE_ID_START);

    os_printf("| SW Build Time: %s, %s\r\n", __TIME__, __DATE__);

}

static void public_init(void) {
    BK3000_Ana_PLL_enable(CPU_DCO_CLK); /* CPU clk = 96MHz */
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
    BK3000_GPIO_Initial();
    SYSirq_Initialise();
//#if (CONFIG_DEBUG_PCM_TO_UART == 1)
//    uart_initialise(UART_BAUDRATE_460800);//UART_BAUDRATE_460800
//#else
//    uart_initialise(UART_BAUDRATE_115200);
//#endif
#ifdef TRACE32_DEBUG
	/*set cpu_clk 96M once, and never change in Trace32 debug mode*/
    BK3000_PMU_CONFIG = (CPU_CLK_SEL << sft_PMU_CPU_CLK_SEL) | (CPU_CLK_DIV << sft_PMU_CPU_CLK_DIV);
#endif
    flash_init();
    app_env_init();

}

#ifdef BEKEN_DEBUG
#if 1
static void _Stack_Integrity_Check(void) {
    extern uint32 _sbss_end;
    extern uint32 _stack;
    //volatile uint32 *p_sbss = (volatile uint32 *)((uint32)&_sbss_end  & (~3));
    //volatile uint32 *p_dram_code  = (volatile uint32 *)((uint32) &_stack);
    os_printf("===system stack size:%p,%p,%p\r\n",&_stack,&_sbss_end,(uint32)&_stack - (uint32)&_sbss_end);
#if 0
    if (p_sbss[0] != 0XDEADBEEF) {
        os_printf("ShowStack:%p:%p\r\n",  &_sbss_end, &_stack);
        os_printf("Stack overflow!\r\n");
        while(1);
    }
#endif
#if 0
    if(p_dram_code[0] != 0xDEADBEEF)
    {
        os_printf("DRAM_CODE is polluted\r\n");
        while(1);
    }
#endif
}
#endif
#endif

/* This memory for OPUS,Locate at share memory,address:0x00E01000,length = 60*1024 */
extern uint32 _shdata_ram_OPUS;
volatile uint8_t * p_OPUS_data   = (uint8_t*)((uint32_t) &_shdata_ram_OPUS);

int OPTIMIZE_O0 main(void) {
    int ret;
    MSG_T msg;
    uint32 mode;
    app_handle_t app_h = app_get_sys_handler();

    CLEAR_SW_REGISTER(0xFFFFFFFF);
    SET_SW_REGISTER(SYS_MCU_PWR_ON_BOOTING);
    
    public_init();
#if (CONFIG_TWS_SUPPORT_OLD_OPERATE == 1)
    set_tws_flag(TWS_FLAG_STEREO_PAIRING);
#endif  
#ifdef BT_DUALMODE_RW_BLE
    extern void Bk3000_Initialize(void);
    Bk3000_Initialize();
    uart_initialise(UART_BAUDRATE_115200);
    rw_ble_init();
    
    while(1)
    {   
        rw_ble_schedule();
        CLEAR_WDT;
    }
#endif

    controller_init();
    usb_tf_aux_in_out_flag=0;
    start_dsp();
#ifdef BT_DUALMODE_RW
    rw_ble_init();
    appm_stop_advertising();
#endif
    host_init();
    //start_dsp();
#ifdef ENABLE_PWM
    //PWMxinit(0, 1,PWM_PERIOD,PWM_HIGH);
    //PWMxinit(1, 1,PWM_PERIOD,PWM_HIGH);
    PWMxinit(2, 1,PWM_PERIOD,PWM_HIGH);
    PWMxinit(3, 1,PWM_PERIOD,PWM_HIGH);
    PWMxinit(4, 1,PWM_PERIOD,PWM_HIGH);
    //PWMxinit(5, 1,PWM_PERIOD,PWM_HIGH);
#endif


#ifdef JMALLOC_STATISTICAL
    os_printf("JMALLOC_STATISTICAL: POWER ON\r\n");
    memory_usage_show();
#endif
    /* WDT CLK 1/16 32K */
    CLEAR_WDT;
    BK3000_start_wdt(0xFFFF);				//wdt reset 0xc000:10s

    _Stack_Integrity_Check();
#ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    app_exit_sniff_mode();
#endif
    
#ifdef BEKEN_OTA
    app_ota_erase_flash();
#endif

    CLEAR_SW_REGISTER(SYS_MCU_PWR_ON_BOOTING);
    //wait 2 second for BT initialization  && SD/UDISK/LINEIN detection complete
    //then decide enter which mode
    uint64_t tmp =os_get_tick_counter();
    while(1)
    {
        if(((os_get_tick_counter() - tmp) > 250)&&(!app_wave_playing())) // must>250 :just for usb disk
        {
        #if (CONFIG_APP_MP3PLAYER == 1)
        #ifdef CONFIG_APP_USB
            if(udisk_is_enumerated())
            {
                app_h->sys_work_mode = SYS_WM_UDISK_MUSIC_MODE;
                //bt_auto_connect_stop();
            }
            else
        #endif
        #if defined(CONFIG_APP_SDCARD)
            if(sd_is_attached())
            {
                app_h->sys_work_mode = SYS_WM_SD_MUSIC_MODE;
                //bt_auto_connect_stop();
            }
            else
        #endif
        #endif
                if(linein_is_attached())
            {
                app_h->sys_work_mode = SYS_WM_LINEIN_MODE;
                //bt_auto_connect_stop();
            }
            else
            {
                app_h->sys_work_mode = SYS_WM_BT_MODE;
            }
            break;
        }
        app_wave_file_play();
        BT_msg_handler();
        //CLEAR_WDT;
    }
    os_printf("sys_work_mode:%d\r\n",app_h->sys_work_mode);
    while(1)
    {
        mode = get_app_mode();
        switch(mode)
        {
            case SYS_WM_LINEIN_MODE:
                linein_mode_msg_handler();
                break;

        #if (CONFIG_APP_MP3PLAYER == 1)
            case SYS_WM_SD_MUSIC_MODE:
        #ifdef CONFIG_APP_USB
            case SYS_WM_UDISK_MUSIC_MODE:
        #endif
                music_mode_msg_handler();
                break;
                
        #ifdef FM_ENABLE
            case SYS_WM_FM_MODE:
                fm_msg_handler();
                break;
        #endif
        #endif
        
            case SYS_WM_BT_MODE:
            default:
                bt_mode_msg_handler();
                break;
        }
    }    
    return 0;
}
