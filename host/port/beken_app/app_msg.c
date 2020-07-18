#include "app_beken_includes.h"
#include "app_msg.h"
#include "msg_pub.h"
#include "bt_mini_sched.h"

#define MEDIA_ATTACH 1
#define MEDIA_DETATCH 0
uint16 input_number;
//extern RAM_CODE void Beken_Uart_Rx(void);
uint8 mutePa=0;
#ifdef CONFIG_APP_USB
extern void usb_host_start_enumberate(void);
#endif
RAM_CODE uint8 common_msg_handler(MSG_T *msg_ptr)
{
    uint8 ret_val = 0;
    app_env_handle_t env_h = app_env_get_handle();;
    //app_handle_t app_h = app_get_sys_handler();
    //uint32 msg_val = msg_ptr->id;

    switch(msg_ptr->id)
    {
        case MSG_ENV_WRITE_ACTION:
            //os_printf("common_msg_handler. exec->MSG_ENV_WRITE_ACTION \r\n");
            if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_TWS))
                app_env_write_action(&(env_h->env_data.default_btaddr),(uint8)(msg_ptr->arg));
            else
                app_env_write_flash((uint8)(msg_ptr->arg));
            break;
     
        //case MSG_1S_TIMER:
            //CLEAR_WDT;
        //    break;
        case MSG_KEY_MUTE:
            mutePa=!mutePa;
            break;
        case MSG_KEY_LANGE_CHANGE:
            app_button_lang_change();
            break;
    #if (CONFIG_APP_MP3PLAYER == 1)
        case MSG_MEDIA_READ_ERR:
            break;
    #ifdef CONFIG_APP_USB
        case MSG_USB_HOST_ATTACH_UDISK_OK:
            //os_printf("MSG_USB_HOST_ATTACH_UDISK_OK\r\n");
            if (usb_tf_aux_in_out_flag)
            {
                app_powerdown_action();
            }
            else
            {
                ret_val = system_mode_shift_for_Udisk_change(MEDIA_ATTACH);
            }
            break;
        case MSG_USB_HOST_ATTACH_OTHER_DEVICE_OK:
//            os_printf("MSG_USB_HOST_ATTACH_OTHER_DEVICE_OK\r\n");
            break;
	case MSG_USB_HOST_ATTACH_DEVICE:
            //os_printf("MSG_USB_HOST_ATTACH_DEVICE\r\n");
            usb_host_start_enumberate();
            break;
	case MSG_USB_HOST_DETACH_DEVICE:
            //os_printf("MSG_USB_HOST_DETACH_DEVICE\r\n");
            if (usb_tf_aux_in_out_flag)
            {
                app_powerdown_action();
            }
            else
            {
                ret_val = system_mode_shift_for_Udisk_change(MEDIA_DETATCH);
            }
            break;
    #endif
    #ifdef CONFIG_APP_SDCARD
        case MSG_SD_ATTACH:
            os_printf("MSG_SD_ATTACH\r\n");
            clear_sd_detached();
            if(usb_tf_aux_in_out_flag)
            {
                app_powerdown_action();
            }
            else
            {
                ret_val = system_mode_shift_for_sd_change(MEDIA_ATTACH);
            }
            break;
        case MSG_SD_DETACH:
            os_printf("MSG_SD_DETACH\r\n");
            clear_sd_attached();
            clr_sd_noinitial_flag();
            if(usb_tf_aux_in_out_flag)
            {
                app_powerdown_action();
            }
            else
            {
                ret_val = system_mode_shift_for_sd_change(MEDIA_DETATCH);
            }
            break;
    #endif
    #endif
        case MSG_GOTO_BT_MODE:
            ret_val = system_work_mode_direct_change(SYS_WM_BT_MODE);
            break;
	 case MSG_GOTO_AUX_MOTE:
            ret_val = system_work_mode_direct_change(SYS_WM_LINEIN_MODE);
            break;
    #ifdef FM_ENABLE
	 case MSG_GOTO_FM_MODE:
            ret_val = system_work_mode_direct_change(SYS_WM_FM_MODE);
            break;
    #endif
    #ifdef CONFIG_APP_SDCARD
	 case MSG_GOTO_SD_MODE:
            ret_val = system_work_mode_direct_change(SYS_WM_SD_MUSIC_MODE);
            break;
    #endif
    #ifdef CONFIG_APP_USB
	 case MSG_GOTO_UDISK_MODE:
            ret_val = system_work_mode_direct_change(SYS_WM_UDISK_MUSIC_MODE);
            break;
    #endif
	 case MSG_CHANGE_MODE:
	 case MSG_KEY_MODE:
            ret_val = system_work_mode_change_button();
            break;
    #if 0//defined(CONFIG_APP_AEC)
        case MSG_SDADC:
            #ifdef CONFIG_APP_HALFDUPLEX
                app_hfp_echo_erase();
            #endif
                break;
    #endif

        //case MSG_FLASH_WRITE:
        //    break;

        case MSG_KEY_POWER_DOWN:
            usb_tf_aux_in_out_flag=1;
        #if (CONFIG_APP_MP3PLAYER == 1)
            if (player_get_play_status()) //  关掉正在播放的音乐
            {
                app_player_play_pause_caller(0);
            }
        #endif
            app_button_powerdown();
            break;

        case MSG_POWER_UP:
            BK3000_start_wdt(10);
            while(1);
            break;

        case MSG_LOWPOWER_DETECT:
            app_low_power_detect();
            break;
        case MSG_CHARGE_STATE:
            app_battery_charge_process();
            break;
            
        case MSG_LINEIN_ATTACH:
            if(usb_tf_aux_in_out_flag)
            {
                //BK3000_Ana_Line_enable(0);
                linein_audio_close();
                app_powerdown_action();
            }
            else
                ret_val = system_mode_shift_for_linein_change(1);
            break;

        case MSG_LINEIN_DETACH:
            if(usb_tf_aux_in_out_flag)
            {
                //BK3000_Ana_Line_enable(0);
                linein_audio_close();
                app_powerdown_action();
            }
            else
                ret_val = system_mode_shift_for_linein_change(0);
            break;
    #ifdef CONFIG_IRDA    
        case MSG_KEY_0:
	 case MSG_KEY_1:
	 case MSG_KEY_2:
	 case MSG_KEY_3:
	 case MSG_KEY_4:
	 case MSG_KEY_5:
	 case MSG_KEY_6:
	 case MSG_KEY_7:
	 case MSG_KEY_8:
	 case MSG_KEY_9:
            if (input_number > 9999)
                input_number = 0;
            input_number = input_number * 10 + (msg_ptr->id - MSG_KEY_0);
            break;
    #endif            
        case MSG_TIMER_PWM1_PT2_HDL:
            if(msg_ptr->hdl)
            {
                (*((jthread_func)msg_ptr->hdl))((void *)msg_ptr->arg);
            }
            break;

#if 1/*(CONFIG_APP_TOOLKIT_5 == 1)*/||(POWERKEY_5S_PARING == 1)
	case MSG_ENTER_MATCH_STATE:
   		enter_match_state();
		bt_flag1_operate(APP_FLAG_AUTO_CONNECTION, 1);
		bt_auto_connect_stop();
		break;
	case MSG_POWER_ON_START_CONN:
		bt_auto_connect_start();
		break;
#endif
        default:
            break;
    }

//    env_h = app_env_get_handle();
    if( sleep_tick >= env_h->env_cfg.system_para.sleep_timeout )
    {
        app_sleep_func(1);
    }
	return ret_val;
}

extern RAM_CODE t_error BTms_Sched(u_int32 num_interations);
RAM_CODE void BT_msg_handler(void)
{
    int ret;
    MSG_T msg;

    ret = msg_get(&msg);
    if(MSG_FAILURE_RET == ret)
        msg.id = MSG_NULL;
    else
    {
        switch(msg.id)
        {
            case MSG_TIMER_PWM1_PT2_HDL:
                if(msg.hdl)
                {
                    (*((jthread_func)msg.hdl))((void *)msg.arg);
                }
                break;
            //case MSG_1S_TIMER:
            //    CLEAR_WDT;
            //    break;
        #ifdef CONFIG_APP_USB
            case MSG_USB_HOST_ATTACH_DEVICE:
                os_printf("MSG_USB_HOST_ATTACH_DEVICE\r\n");
                usb_host_start_enumberate();
                break;
        #endif   
            default:
                break;
        }
    }
	
#ifdef BT_DUALMODE_RW
	rw_ble_schedule();
#endif
#ifdef CONFIG_BLUETOOTH_COEXIST
    BTms_Sched(2);//controller的处理
#else
	BTms_Sched(1);//controller的处理
#endif
    //Beken_Uart_Rx();//调试串口命令处理
    timer_clear_watch_dog();
}

extern volatile uint32 flag_clr_wdt;
RAM_CODE uint8 pre_enter_mode_handle(uint32 mode)
{
    MSG_T msg;
    int ret = 0;
    uint8 exit_flag = 0;
    uint8 wavstartflag = 0;
    app_handle_t sys_hdl = app_get_sys_handler();
    uint8 flag_wave_timeout = 0;

    CLEAR_WDT;
    msg_clear_all();
    app_wave_file_play_stop();
   
    os_printf("pre_enter flag_sm1:0x%x,flag_sm2:0x%x,wave_play:%d,work_mode:%d\r\n",sys_hdl->flag_sm1,sys_hdl->flag_sm2,app_wave_playing(),sys_hdl->sys_work_mode);
    do
    {
        ret = msg_get(&msg);
        if(MSG_FAILURE_RET == ret)
            msg.id = MSG_NULL;
        else
            exit_flag = common_msg_handler(&msg);
        if(exit_flag == 1)
            break;
		
    #ifdef BT_DUALMODE_RW
        rw_ble_schedule();
    #endif
    #ifdef CONFIG_BLUETOOTH_COEXIST
        BTms_Sched(2);//controller的处理
    #else
        BTms_Sched(1);
    #endif
        app_wave_file_play();
        
        if(!wavstartflag)
        {
            wavstartflag = 1;
            if(enter_mode_wave_and_action(mode, NULL))
                return 0;
        }
        //timer_clear_watch_dog();
        if(flag_clr_wdt > 100) // clr wdt :1s
        {
            CLEAR_WDT;
            flag_clr_wdt = 0;
            flag_wave_timeout ++;
        }
        
        if(flag_wave_timeout > 5) // 5s
        {
            flag_wave_timeout = 0;
            exit_flag = 0;
            os_printf("pre_mode...timeout...:0x%x,0x%x\r\n",sys_hdl->flag_sm1,sys_hdl->flag_sm2);
            app_wave_file_play_stop();
            //msg_put(MSG_KEY_MODE);
        }
    }while(app_wave_playing()
    #ifndef CONFIG_BLUETOOTH_COEXIST
        ||(bt_flag1_is_set(APP_AUDIO_FLAG_SET)&& (sys_hdl->sys_work_mode != SYS_WM_BT_MODE))
    #endif
    );
    os_printf("pre_enter_end flag_sm1:0x%x,flag_sm2:0x%x,wave_play:%d,%d\r\n",sys_hdl->flag_sm1,sys_hdl->flag_sm2,app_wave_playing(),exit_flag);
	
    return exit_flag;
}

// EOF
