#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "msg_pub.h"

#include "bt_mini_sched.h"
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
#include "../../../bluetooth/profile/a2dp/bt_sbc.h"
#endif

volatile uint8 linePlay=0;
static linein_cfg_t linein_cfg;
volatile uint8_t linein_audio_flag = 0;
//extern RAM_CODE void Beken_Uart_Rx(void);
extern RAM_CODE t_error BTms_Sched(u_int32 num_interations);

#ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE 
extern u_int8 syspwr_cpu_halt;
#endif
linein_cfg_t *get_line_in_config(void)
{
    return(&linein_cfg);
}

void linein_audio_open(void)
{
	app_env_handle_t  env_h = app_env_get_handle();
    if(linein_audio_flag) 
        return;

    os_printf("linein_audio_open\r\n");
    aud_PAmute_oper(1);
    os_delay_ms(20);
#if (CONFIG_AUDIO_USED_MCU==0)
    aud_aux_ana_gain_set(env_h->env_cfg.feature.vol_linein_ana);
    aud_aux_dig_gain_set(env_h->env_cfg.feature.vol_linein_dig&0x3f);
#endif
    BK3000_Ana_Line_enable(1);
    aud_volume_set(player_vol_bt);
    if((player_vol_bt==0)||(linePlay==0))
        aud_PAmute_oper(1);
    else
        aud_PAmute_oper(0);
    linein_audio_flag = 1;
}

void linein_audio_close(void)
{
    //aud_PAmute_oper(1);
    //os_delay_ms(20);
#if CONFIG_ADC_DMA
    // init ADC_DMA
    if(bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
    {
        adc_dma_stop();
        sdadc_enable(0);
    }
#endif
    BK3000_Ana_Line_enable(0);
    linein_audio_flag = 0;
    os_printf("linein_audio_close:%d\r\n",linein_audio_flag);
}

static inline void linein_attach_judge(void)
{
    if(LINEIN_STATUS_OFF == linein_cfg.status)
    {
        linein_cfg.plugin_count++;
        linein_cfg.pullout_count = 0;
    #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
        app_exit_sniff_mode();
    #endif
        if( linein_cfg.plugin_count > LINEIN_GPIO_DEBOUNCE_COUNT )
        {
            linein_cfg.status = LINEIN_STATUS_ON;
            os_printf("line in insert\r\n");
            msg_put(MSG_LINEIN_ATTACH);
        }
    }	
}
static inline void linein_detach_judge(void)
{
    if(LINEIN_STATUS_ON == linein_cfg.status)
    {
        linein_cfg.pullout_count++;
        linein_cfg.plugin_count = 0;
    #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
        app_exit_sniff_mode();
    #endif
        if(linein_cfg.pullout_count > ( LINEIN_GPIO_DEBOUNCE_COUNT-30) )
        {		   	
            linein_cfg.status = LINEIN_STATUS_OFF;
            os_printf("line in detach\r\n");
            //BK3000_AUD_FIFO_CONF &= ~(1 << sft_AUD_ADC_INT_EN);
            msg_put(MSG_LINEIN_DETACH);
        }
    }
}

static void linein_detect(void)
{
    uint32 linein_gpio_status;
    app_env_handle_t  env_h = app_env_get_handle();

    if (!(env_h->env_cfg.system_para.system_flag &APP_ENV_SYS_FLAG_AD_LINEIN_ENA))
    {
    #ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
	    if ((get_tws_env_stereo_role()==TWS_PRIM_SEC_SECOND)
	        && bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
	        return;
	#endif    
        linein_gpio_status = gpio_input(linein_cfg.pin);
		
        if(linein_cfg.high_flag == linein_gpio_status)//在线
        {
            linein_attach_judge();
        }
        else
        {
            linein_detach_judge();
        }
    }
    else
    {

    }
}

void app_linein_init(void)
{

    app_env_handle_t  env_h = app_env_get_handle();
	
    memset(&linein_cfg, 0, sizeof(linein_cfg_t));
	
    if(env_h->env_cfg.used == 0x01)
    {
        linein_cfg.enable = (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_LINEIN_ENA) >> 3;
        linein_cfg.high_flag = (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_LINEIN_HIGH) >> 4;
    #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
        sniffmode_wakeup_dly = 1000;
    #endif
    }
    else
    {
        linein_cfg.enable = 0;//无配置表则无linein功能，避免默认引脚配置
        linein_cfg.high_flag = LINEIN_GPIO_LOW_VALID;
    }
    
    if(linein_cfg.enable)    
    {
        if(!(env_h->env_cfg.system_para.system_flag &APP_ENV_SYS_FLAG_AD_LINEIN_ENA))
        {
            linein_cfg.pin = env_h->env_cfg.system_para.linein_pin;

            if(LINEIN_GPIO_LOW_VALID == linein_cfg.high_flag)//low valid
                gpio_config(linein_cfg.pin,3);
            else
                gpio_config(linein_cfg.pin,0);
        }
        linein_cfg.detect_func = linein_detect;
        
        linein_cfg.status = LINEIN_STATUS_OFF;
    #if (CONFIG_AUDIO_USED_MCU == 1)
        linein_audio_close();//防止串音
    #endif
        os_printf("app_linein_init pin:%d\r\n",linein_cfg.pin);

    }
}

int app_linein_powerup_check(void)
{
    int ret = 0;
    uint32 linein_gpio_status;
    /* app_env_handle_t env_h = app_env_get_handle(); */

    if(linein_cfg.enable)
    {
        linein_gpio_status = gpio_input(linein_cfg.pin);
        if(linein_cfg.high_flag == linein_gpio_status)
        {
            ret = 1;
        }
    }

    return ret;
}

#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
uint8 app_bt_tws_linein_src_enable(void)
{
    app_handle_t app_h = app_get_sys_handler();
	
    os_printf("app_bt_tws_linein_src_enable:0x%x,0x%x\r\n", app_h->flag_sm1,app_h->flag_sm2);
    if (bt_flag1_is_set(APP_FLAG_LINEIN)
        && bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE)
        && (get_tws_prim_sec()==TWS_PRIM_SEC_PRIMARY)
       // && (!bt_flag1_is_set(APP_FLAG_MUSIC_PLAY))
        && (!bt_flag2_is_set(APP_FLAG2_STEREO_LINEIN_TX))
    )
    {
    	os_printf("linein_src\r\n");
        set_flag_sbc_buffer_play(0);
        // free all sbc node.
        tws_clear_a2dp_buffer(); // It must be excuted after [set_flag_sbc_buffer_play(0)].
	 sbc_init_adjust_param();
        bt_sbc_init(NULL); // for linein_tws sync decode.
        app_set_led_event_action(LED_EVENT_LINEIN_PLAY);
        jtask_schedule(app_h->app_reset_task, 200, (jthread_func)app_start_aux_tx, (void *)1000); //quick disconnect bluetooth connection
        avrcp_master_send_slave(TWS_VENDOR_DEP_LINEIN_CMD, TWS_VENDOR_DEP_LINEIN_ATTACH);
        return 1;
    }
    else
        return 0;
}
#endif

void app_linein_enter(void *arg)
{   
    app_handle_t app_h = app_get_sys_handler();
#if defined(CONFIG_LINE_SD_SNIFF)
    bt_flag2_operate(APP_FLAG2_LINEIN_SD_PLAYING,1);
#endif

#ifdef CONFIG_BLUETOOTH_COEXIST
    if(bt_flag1_is_set(APP_FLAG_SCO_CONNECTION|APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED) )
    {
        app_h->flag_sm1 |= APP_FLAG_LINEIN;

        //aud_adc_initial(8000,1,16);
        hf_audio_handler(1);
        aud_mic_open(1);
    }
    else
#endif
    {
        app_wave_file_play_stop();
        app_h->flag_sm1 |= APP_FLAG_LINEIN;
        //Delay(4000);
    #ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
        if (app_bt_tws_linein_src_enable() == 0)
    #endif  
        {
            linein_audio_open();
        }
    }
    aud_PAmute_oper(0);
}

void app_linein_exit(void)
{   
    app_handle_t app_h = app_get_sys_handler();
#ifdef CONFIG_BLUETOOTH_COEXIST
    if(bt_flag1_is_set(APP_FLAG_SCO_CONNECTION|APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED) )
    {
        app_h->flag_sm1 &= ~APP_FLAG_LINEIN;
    }
    else
#endif
    {
    #if CONFIG_ADC_DMA
        adc_dma_stop();
    #endif
    #ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
        sdadc_enable(0);
        linein_sbc_alloc_free();
        bt_flag1_operate(APP_FLAG_MUSIC_PLAY|APP_BUTTON_FLAG_PLAY_PAUSE, 0);
        bt_flag2_operate( APP_FLAG2_STEREO_INQUIRY_RES, 0 );
    #endif    
        linein_audio_close();
        app_h->flag_sm1 &= ~APP_FLAG_LINEIN;
        Delay(2000);
    }
    os_printf("app_linein_exit\r\n");
}

void app_linein_scanning( void )
{
    if(linein_cfg.detect_func)
    {
        (*linein_cfg.detect_func)();
    }
}

static void playwav_resumelinein(uint32 fieldId)
{
    if((!app_wave_playing()) && (0 == check_wave_file_correct(fieldId)))
    {
        /* close linein audio*/
        linein_audio_close();
        /* start wave*/
        start_wave_and_action(fieldId, NULL); // linein_audio_open
    }
}

void app_playwav_resumelinein(uint32 fieldId)
{
    playwav_resumelinein(fieldId);
}

static int app_linein_vol_up(void)
{
    if(player_vol_bt != AUDIO_VOLUME_MAX)
    {
        player_vol_bt ++;
    }
    if(!app_wave_playing())
        aud_volume_set(player_vol_bt);
    //os_printf("vol_up:%d\r\n",player_vol_bt);
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE	
    a2dp_volume_init(player_vol_bt);
    if( bt_flag2_is_set( APP_FLAG2_STEREO_WORK_MODE ))
    {
        if(get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)
            send_ct_cmd_test((void *)AVC_OP_VOLUME_UP );
        //else
        //    send_cmd_test((void *)AVC_OP_VOLUME_UP);
    }
#endif
    if(AUDIO_VOLUME_MAX == player_vol_bt)
    {
        playwav_resumelinein(APP_WAVE_FILE_ID_VOL_MAX);
    }
#if (SYS_VOL_SAVE == 1)
    app_save_volume(ENV_LINE_VOL_INFO);
#endif
    return 0;
}

static int app_linein_vol_down(void)
{ 
    if(player_vol_bt != AUDIO_VOLUME_MIN)
    {
        player_vol_bt --;
    }
    if(!app_wave_playing())
        aud_volume_set(player_vol_bt);
    //os_printf("vol_down:%d\r\n",player_vol_bt);
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE	
    a2dp_volume_init(player_vol_bt);
    if( bt_flag2_is_set( APP_FLAG2_STEREO_WORK_MODE ))
    {
        if(get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)
            send_ct_cmd_test((void *)AVC_OP_VOLUME_DOWN );
        //else
        //    send_cmd_test((void *)AVC_OP_VOLUME_DOWN);
    }
#endif
    if(AUDIO_VOLUME_MIN == player_vol_bt)
    {
        playwav_resumelinein(APP_WAVE_FILE_ID_VOL_MIN);
    }
#if (SYS_VOL_SAVE == 1)
    app_save_volume(ENV_LINE_VOL_INFO);
#endif
    return 0;
}

static int app_linein_mute_unmute(void)
{
    return 0;
}

uint8 linein_is_attached(void)
{
    return(linein_cfg.status == LINEIN_STATUS_ON);
}

uint8 linein_get_play_status(void)
{
    return linePlay;
}

#ifdef CONFIG_BLUETOOTH_COEXIST
static uint8 linein_player_status = 0; 
static uint8 flag_coexist_linein_player = 0;
void app_coexist_linein_play_pause(uint8 flag)
{
    app_handle_t app_h = app_get_sys_handler();
    os_printf("app_coexist_linein_play_pause(%d,%d)\r\n",flag,flag_coexist_linein_player);

    //if(app_is_linein_mode())
    {
        if(flag && (flag_coexist_linein_player != 1))
        {
            os_printf("coexist_line in playing...:%d\r\n",linein_player_status);
            app_button_type_set(BUTTON_TYPE_NON_HFP);
            app_wave_file_play_stop();
            sbc_mem_free();
            linePlay = 1;
            aud_volume_set(player_vol_bt);
            jtask_stop(app_h->app_coexist_task);
            //if(linein_player_status)
            jtask_schedule(app_h->app_coexist_task,2000,(jthread_func)linein_audio_open,(void *)NULL);
            flag_coexist_linein_player = 1;
        }
        else if((0 == flag) && (flag_coexist_linein_player != 2))
        {
            os_printf("coexist_line in pause...\r\n");
            app_wave_file_play_stop();
            jtask_stop(app_h->app_coexist_task);
            //linein_player_status = linein_get_play_status();
            linePlay = 0;
            app_sleep_func(1);
            linein_audio_close();
            aud_volume_mute(1);
            flag_coexist_linein_player = 2;
        }
    }
}
#endif

void linein_mode_msg_handler(void)
{
    int ret;
    uint8 exit_flag = 0;
    MSG_T msg;
    app_handle_t app_h = app_get_sys_handler();
    
    app_set_led_event_action(LED_EVENT_LINEIN_PLAY);
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
    if(!hci_get_acl_link_count(app_h->unit))
        bt_unit_set_scan_enable(app_h->unit,HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
    else
        bt_unit_set_scan_enable( app_h->unit, HCI_PAGE_SCAN_ENABLE);
#else
#ifdef CONFIG_BLUETOOTH_COEXIST
    flag_coexist_linein_player = 0;
    if(!hci_get_acl_link_count(app_h->unit))
        bt_unit_set_scan_enable(app_h->unit,HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
    else
        bt_unit_set_scan_enable( app_h->unit, HCI_NO_SCAN_ENABLE);
#else
#if defined(CONFIG_LINE_SD_SNIFF)
#ifdef CONFIG_APP_USB 
    app_env_handle_t  env_h = app_env_get_handle();
    if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_USB_ENA)
    {
        bt_unit_set_scan_enable( app_h->unit, HCI_NO_SCAN_ENABLE);
    }
    else
#endif
        bt_unit_set_scan_enable( app_h->unit, HCI_PAGE_SCAN_ENABLE); // HCI_NO_SCAN_ENABLE isr ceva: 20s  
#else
    bt_unit_set_scan_enable(app_h->unit,HCI_NO_SCAN_ENABLE);
#endif	
#endif
#endif

    if(pre_enter_mode_handle(app_h->sys_work_mode))
        return;
	
#if (CONFIG_AUDIO_USED_MCU == 1)
	aud_dsp_mode_mp3_set();
#endif	
#ifndef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE	
    app_restore_volume(ENV_LINE_VOL_INFO);
#endif
    enter_work_mode();

    if(!linein_is_attached())
    {
        //BK3000_AUD_FIFO_CONF &= ~(1 << sft_AUD_ADC_INT_EN);
        msg_put(MSG_LINEIN_DETACH);
    }
#if defined(CONFIG_LINE_SD_SNIFF)
    bt_flag2_operate(APP_FLAG2_LINEIN_SD_PLAYING,1);
#endif	
    linePlay=1;
#ifdef CONFIG_BLUETOOTH_COEXIST
    flag_coexist_linein_player = 1;
#endif
    while(!exit_flag)
    {
        ret = msg_get(&msg);
        if(MSG_FAILURE_RET == ret)
            msg.id = MSG_NULL;
        else
        {
            switch(msg.id)
            {
                case MSG_KEY_PLAY:
                    if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_AUX_MODE_PAUSE_MUTE))
                    {
                    #ifdef CONFIG_BLUETOOTH_COEXIST
                        if(bt_flag1_is_set(APP_FLAG_SCO_CONNECTION|APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
                        {
                        }
                        else
                    #endif
                        {
                            if(linePlay)
                            {
                                os_printf("line in pause...\r\n");
                                linePlay = 0;									
                            #if defined(CONFIG_LINE_SD_SNIFF)
                                bt_flag2_operate(APP_FLAG2_LINEIN_SD_PLAYING,0);
                            #endif
                                app_sleep_func(1);
                                linein_audio_close();
								aud_close();
                                aud_volume_mute(1);
                                app_set_led_event_action(LED_EVENT_LINEIN_PAUSE);
                                bt_flag2_operate(AP_FLAG2_LINEIN_MUTE, 1);
                            }
                            else
                            {
                                os_printf("line in playing...\r\n");
                                linePlay = 1;								
                            #if defined(CONFIG_LINE_SD_SNIFF)
                                bt_flag2_operate(APP_FLAG2_LINEIN_SD_PLAYING,1);
                            #endif
                                bt_flag2_operate(AP_FLAG2_LINEIN_MUTE, 0);
                                app_audio_restore();
                                app_set_led_event_action(LED_EVENT_LINEIN_PLAY);
                            }
                        #ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE	
                            if(bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
                            {
                                if(get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)
                                    avrcp_master_send_slave(TWS_VENDOR_DEP_LINEIN_CMD,!linePlay);
                                else
                                    avrcp_slave_send_master(TWS_VENDOR_DEP_LINEIN_CMD,!linePlay);	
                            }
                        #endif
                        }
                    }
                    break;

                case MSG_KEY_VOL_DOWN:
                    app_linein_vol_down();
                    break;
                case MSG_KEY_VOL_UP:
                    app_linein_vol_up();
                    break;	
                //case MSG_HALF_SECOND:			
                //    break;
            #ifdef CONFIG_TWS
                case MSG_KEY_RESERVED2: // slave start auto conn
                    app_button_stereo_slave_action();
                    break;
                case MSG_KEY_RESERVED3: // master
                    app_button_stereo_master_action();
                    break;
            #endif

            #ifdef CONFIG_BLUETOOTH_COEXIST
                case MSG_KEY_HFP_ACK:
                    app_button_hfack_action();
                    break;
                case MSG_KEY_HFP_REJECT:
                    app_button_reject_action();
                    break;
                case MSG_KEY_NUM_REDIAL:
                	app_button_redial_last_number();
                	break;
                case MSG_KEY_TRANSFER_TOGGLE:
                    app_button_hf_transfer_toggle();
                    break;
            #endif       
                default:
                    exit_flag = common_msg_handler(&msg);
                    break;
            }
        }
        if(app_wave_playing())
            app_wave_file_play();
        else
        {
        #ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
            if(bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE)&& (get_tws_prim_sec()==TWS_PRIM_SEC_PRIMARY))
            {
                sbc_do_encode();
                sbc_do_decode();        
            }
            else if(bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE)&& (get_tws_prim_sec()==TWS_PRIM_SEC_SECOND))
            {
                sbc_do_decode();
            }
        #endif
        }
    #ifdef BT_DUALMODE_RW
        rw_ble_schedule();
    #endif
    #ifdef CONFIG_BLUETOOTH_COEXIST
    #if (CONFIG_APP_AEC  == 1)
	 if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_AEC_ENABLE))
            app_aec_swi();
    #endif
        BTms_Sched(2);//controller的处理
    #else
        BTms_Sched(1);//controller的处理
    #endif
    
        //Beken_Uart_Rx();//调试串口命令处理
        timer_clear_watch_dog();
    #ifdef CONFIG_BLUETOOTH_COEXIST
        BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
    #else
    #if (CONFIG_CPU_CLK_OPTIMIZATION == 1)&& defined(CONFIG_LINE_SD_SNIFF)
        if(( !bt_flag1_is_set( APP_FLAG_WAVE_PLAYING ) )&&( !bt_flag2_is_set( APP_FLAG2_LINEIN_SD_PLAYING ) ))
        {
            BK3000_set_clock(CPU_CLK_SEL,CPU_OPTIMIZATION_DIV);
        }
    #endif
    #endif
    }
#ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
#if defined(CONFIG_LINE_SD_SNIFF)
    bt_flag2_operate(APP_FLAG2_LINEIN_SD_PLAYING,0);
#endif
    app_exit_sniff_mode();
#endif
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
    app_change_mode_flag_clear();
#endif

    os_printf("exit LINEIN mode\r\n");
}

// EOF

// EOF
