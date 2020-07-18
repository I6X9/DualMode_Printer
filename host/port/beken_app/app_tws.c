/*
 * tws.c
 *
 *  Created on: 2017-12-20
 *      Author: jianjun.ye
 */
#include <string.h>
#include "app_tws.h"
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "bt_init.h"
#include "lmp_acl_container.h"
#include "tws_hfp.h"
#include "app_sbc.h"

#ifdef CONFIG_TWS
#define TWS_DBG_ERR(fmt, ...)    os_printf("TWS: "fmt, ##__VA_ARGS__)

#define TWS_DBG_ENABLE       (1)
#if TWS_DBG_ENABLE
#define TWS_DBG(fmt, ...)    os_printf("TWS: "fmt, ##__VA_ARGS__)
int                                 os_printf(const char *fmt, ...);
#else
#define TWS_DBG(fmt, ...)
#endif

extern void Tra_status_show(void);
extern inline t_clock get_btclk_as_piconet_master(void);
extern inline t_clock get_btclk_as_piconet_slave(void);
extern void LMscan_status_show(void);
extern void app_powerdown(void);

static APP_TWS_T g_tws_app;

static uint32 s_tws_flag = 0;
static uint8 stereo_autoconn_cnt = 0;

inline void set_tws_flag(uint32 flag)
{
    s_tws_flag |= flag;
}

inline void unset_tws_flag(uint32 flag)
{
    s_tws_flag &= ~flag;
}

inline uint32 get_tws_flag(uint32 flag)
{
    return s_tws_flag & flag;
}

inline app_tws_t app_get_tws_handler(void)
{
    return &g_tws_app;
}

#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
void app_bt_slave_linein_set(uint8 value)
{
	g_tws_app.linein = value;	
}

uint8 app_bt_slave_linein_get(void)
{
	return g_tws_app.linein; 
}
#endif

static uint8 s_tws_prim_sec = TWS_PRIM_SEC_UNDEFINED;

inline uint8 get_tws_prim_sec(void)
{
    //TWS_DBG("%s, %d\r\n", __func__, s_tws_prim_sec);
    return s_tws_prim_sec;
}

void set_tws_prim_sec(uint8 t_p_s)
{
    TWS_DBG("tws_prim_sec(%d)\r\n",t_p_s);
    s_tws_prim_sec = t_p_s;
    app_env_update_stereo_role_info(s_tws_prim_sec);
}


static boolean s_is_tws_launch_giac_pairing = FALSE;

boolean get_tws_launch_giac_pairing(void)
{
    //TWS_DBG("get_tws_launch_giac_pairing(%d)\r\n", s_is_tws_launch_giac_pairing);
    return s_is_tws_launch_giac_pairing;
}

void set_tws_launch_giac_pairing(boolean tws_giac_pairing)
{
    TWS_DBG("set_tws_pairing(%d)\r\n", tws_giac_pairing);
    
    s_is_tws_launch_giac_pairing = tws_giac_pairing;
}

extern int app_button_stereo_slave_action(void);
void tws_pairing_timeout(void)
{
	app_handle_t app_h = app_get_sys_handler();
	
    TWS_DBG("tws_pairing_timeout()\n");

	if(!bt_flag1_is_set(APP_FLAG_ACL_CONNECTION|APP_FLAG_A2DP_CONNECTION)
        && !bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE)
        && !bt_flag1_is_set(APP_FLAG_POWERDOWN))
    {
    	app_bt_stereo_pairing_exit();
    	jtask_schedule(app_h->tws_linein_task, 500, (jthread_func)app_button_stereo_slave_action, NULL);
    }
}

/* connect to the TWS-M. */
void tws_launch_diac_pairing(void)
{
    //TWS_DBG("tws_pairing()\r\n");
    os_printf(">[TWS inquiry...:%d\r\n",TWS_INQUIRY_AND_PAGE_TIMEOUT);
    app_handle_t app_h = app_get_sys_handler();
    bt_flag2_operate(APP_FLAG2_STEREO_CONNECTTING, 1);
    //set_tws_prim_sec(TWS_PRIM_SEC_UNDEFINED);
    bt_unit_set_scan_enable(app_h->unit, HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE); // 
    //bt_unit_set_scan_enable(app_h->unit, HCI_NO_SCAN_ENABLE); // 
    //bt_unit_set_scan_enable(app_h->unit, HCI_BOTH_INQUIRY_PAGE_SCAN_ENABLE); // 

    //bt_unit_set_inquiry_mode(app_h->unit, 2 /*LM_INQ_EXTENDED_INQUIRY_RESULT*/);

    bt_unit_write_inquiry_IAC(app_h->unit, 1);
#if(CONFIG_TWS_AUTOCONNECT == 1)
    set_tws_prim_sec(TWS_PRIM_SEC_UNDEFINED);
    app_bt_inquiry(TWS_INQUIRY_AND_PAGE_TIMEOUT, 1, APP_LAUNCH_PERIODIC_INQUIRY);    
#else
    #if(CONFIG_TWS_KEY_MASTER == 1)
    set_tws_prim_sec(TWS_PRIM_SEC_PRIMARY);
    #else
    set_tws_prim_sec(TWS_PRIM_SEC_SECOND);
    #endif
    app_bt_inquiry(TWS_INQUIRY_AND_PAGE_TIMEOUT*2, 1, APP_LAUNCH_PERIODIC_INQUIRY);
#endif
	
#if (CONFIG_TWS_SUPPORT_OLD_OPERATE == 0)    
    set_tws_flag(TWS_FLAG_LAUNCH_TWS_INQUIRY_AND_PAGE);
#else
    bt_unit_set_scan_enable(app_h->unit, HCI_NO_SCAN_ENABLE);	
#endif
    /* tws pairing expire time cann't be longer then 15 seconds. */
    //jtask_stop(app_h->app_tws_task);
    //jtask_schedule(app_h->app_tws_task, 30000, (jthread_func)tws_pairing_timeout, NULL);
}

/* Open the DIAC Scan, let the TWS-S to connect to the TWS-M. */
void tws_launch_diac_scan(void)
{
    TWS_DBG("tws_scan()\r\n");
    app_handle_t app_h = app_get_sys_handler();

    bt_flag2_operate(APP_FLAG2_STEREO_CONNECTTING, 1);
    set_tws_prim_sec(TWS_PRIM_SEC_PRIMARY);	
    app_bt_inquiry(0, 0 ,APP_EXIT_PERIODIC_INQUIRY); // close the inqiry.
    bt_unit_write_inquiry_IAC(app_h->unit, 1);

    bt_unit_set_inquiry_scan_type(app_h->unit, STANDARD_SCAN);
    bt_unit_set_page_scan_type(app_h->unit, STANDARD_SCAN);

    bt_unit_set_scan_enable(app_h->unit, HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
}

boolean is_peer_tws(const btaddr_t * const raddr_peer)
{
    app_handle_t app_h = app_get_sys_handler();
    btaddr_t *raddr_local = &(app_h->unit->hci_btaddr);
    if((!raddr_local) || (!raddr_peer))
    {
        //TWS_DBG_ERR("is_peer_tws:raddr_local=%p, raddr_peer=%p \r\n",raddr_local, raddr_peer);
        return FALSE;
    }

    //TWS_DBG("%s, raddr_local "BTADDR_FORMAT"\r\n", __func__, BTADDR(raddr_local));
    //TWS_DBG("%s, raddr_peer  "BTADDR_FORMAT"\r\n", __func__, BTADDR(raddr_peer));

    /*
    Format of BD_ADDR
    LSB                                           MSB
    company_assigned         company_id

    |_1Byte_|_1Byte_|_1Byte_|_1Byte_|_1Byte_|_1Byte_|

    |_LAP___________________|_UAP___|_NAP___________|
    */

    /* The peer's company_id is the same as local's. */

    //if((raddr_local->b[5] == raddr_peer->b[5])
    //&& (raddr_local->b[4] == raddr_peer->b[4])
    //&& (raddr_local->b[3] == raddr_peer->b[3]))
    if(btaddr_same(raddr_peer, app_bt_get_handle(2)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

boolean is_peer_tws_primary(const btaddr_t * const raddr_peer)
{
    app_handle_t app_h = app_get_sys_handler();
    btaddr_t *raddr_local = &(app_h->unit->hci_btaddr);
	
    if((!raddr_local) || (!raddr_peer))
    {
        //TWS_DBG_ERR("%s, invalid param! raddr_local=%p, raddr_peer=%p \r\n", __func__, raddr_local, raddr_peer);
        //os_printf("invalid param! raddr_local=%p, raddr_peer=%p \r\n", raddr_local, raddr_peer);
        return FALSE;
    }

    //TWS_DBG("%s, raddr_local "BTADDR_FORMAT"\r\n", __func__, BTADDR(raddr_local));
    //TWS_DBG("%s, raddr_peer  "BTADDR_FORMAT"\r\n", __func__, BTADDR(raddr_peer));

    /*
    Format of BD_ADDR
    LSB                                           MSB
    company_assigned         company_id

    |_1Byte_|_1Byte_|_1Byte_|_1Byte_|_1Byte_|_1Byte_|

    |_LAP___________________|_UAP___|_NAP___________|
    */

    /* The peer's company_id is the same as local's. 
       The tws primery must connected to the phone first, it can connect the tws second. 
       The tws second shall only can connect to the tws primery and cann't connect to the phone.
    */

    if((raddr_local->b[5] == raddr_peer->b[5])
    && (raddr_local->b[4] == raddr_peer->b[4])
    && (raddr_local->b[3] == raddr_peer->b[3])
    && (!bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void tws_status_show(void)
{
    app_handle_t app_h = app_get_sys_handler();

    os_printf("\r\n\r\n***********************************************\r\n");
    
    os_printf("TWS Compliled at %s, %s¡¾tws flag¡¿:0x%x\r\n\r\n", __TIME__, __DATE__, s_tws_flag);
    
    if(NULL == app_h->unit)
    {
        os_printf("Bluetooth device not initialized yet.\r\n");
        return;
    }

    os_printf("Local Device addr: "BTADDR_FORMAT", ¡¾%d¡¿\r\n", BTADDR(&(app_h->unit->hci_btaddr)), get_tws_prim_sec());

    os_printf("Remote device addr: "BTADDR_FORMAT"\r\n", BTADDR(&(app_h->remote_btaddr)));
    os_printf("Stereo device addr: "BTADDR_FORMAT"\r\n\r\n", BTADDR(&(app_h->stereo_btaddr)));

    hci_link_t *link = NULL, *next = NULL;
    uint32 i = 0;
    
    next = TAILQ_FIRST(&(app_h->unit->hci_links));
    os_printf("next[%d]=%p\r\n", i, next);
    if(NULL != next)
    {
        os_printf("next->self_piconet_role=0x%x-0x%x\r\n", next->self_piconet_role, get_piconet_role(&next->hl_btaddr));
        
        os_printf("DEV[%d]: "BTADDR_FORMAT", status = %d, hl_type = 0x%x, %s\r\n\r\n", 
                    i, 
                    BTADDR(&(next->hl_btaddr)), 
                    next->hl_state, 
                    next->hl_type,
                    ((HCI_ROLE_MASTER == get_piconet_role(&next->hl_btaddr))?"Master":"Slave"));
        i++;
    
    }
    
    while ((link != next) && ((link = next) != NULL))
    {
        next = TAILQ_NEXT(link, hl_next);
        os_printf("next[%d] = %p\r\n", i, next);
        if(NULL == next)/* It shall resulte in unexpected problem if the NULL pointer is used in the os_printf() function. */
        {
            break;
        }        

        os_printf("next->self_piconet_role=0x%x-0x%x\r\n", next->self_piconet_role, get_piconet_role(&next->hl_btaddr));
        os_printf("DEV[%d]: "BTADDR_FORMAT", status = %d, hl_type = 0x%x, %s\r\n\r\n", 
                    i, 
                    BTADDR(&(next->hl_btaddr)), 
                    next->hl_state, 
                    next->hl_type,
                    ((HCI_ROLE_MASTER == get_piconet_role(&next->hl_btaddr))?"Master":"Slave"));
        i++;
    }
        
    if(app_h->flag_sm1 & APP_FLAG_ACL_CONNECTION)
    {

    }
    else
    {
        os_printf("Not connect to other device. app_h->flag_sm1 = 0x%x \r\n", app_h->flag_sm1);
    }

    // two bt_clk.
    os_printf("\r\nget_btclk_as_piconet_master: 0x%08x\r\n"
              "\r\nget_btclk_as_piconet_slave:  0x%08x\r\n", 
                              get_btclk_as_piconet_master(), 
                              get_btclk_as_piconet_slave());
    LMscan_status_show();
    
    os_printf("\r\n***********************************************\r\n\r\n");

    //Tra_status_show();
    memory_usage_show();

    #if TWS_HFP_ENABLE
    os_printf("tws_msbc_frame_cnt = %d\n", tws_hfp_get_msbc_cnt());   
    os_printf("cur msbc node cnt = %d\n", tws_hfp_get_msbc_has_cnt());   
    #endif
}

#if 0
void tws_launch_auto_connect(void)
{
    TWS_DBG("tws_launch_auto_connect: %d-%d\r\n", get_tws_env_stereo_role(), get_tws_prim_sec());

    #if TWS_CONN_MECHANISM_V2
    
    if(get_tws_launch_giac_pairing())
    {
        app_tws_enter_giac_match();
    
        set_tws_launch_giac_pairing(FALSE);
        return;
    }

	if((!app_get_env_stereo_key_used() && !app_get_env_key_num()))
    {
        TWS_DBG("first connect!\r\n");
        /* For the first connection.
        If I am  TWS_PRIM_SEC_UNDEFINED or TWS_PRIM_SEC_SECOND, 
        launch the DIAC Inquiry. 
        */
        tws_launch_diac_pairing();
    	return;
    }

    // Check autoconn flag.
    if(!bt_flag2_is_set(APP_FLAG2_STEREO_AUTOCONN)) 
    {
        TWS_DBG("mistake! not set flag!\r\n");
        return;
    }
    
    // Check TWS-SLAVE status.
    if(get_tws_prim_sec() == TWS_PRIM_SEC_SECOND && bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION))
    {
        TWS_DBG("mistake! SECOND already connected.\r\n");
        return;
    }
    
    // Check TWS-MASTER status.
    if(get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY && bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION))
    {
        TWS_DBG("mistake! PRIMARY already connected.\r\n");
        return;
    }

    app_handle_t app_h = app_get_sys_handler();

    // Close role switch.
    bt_flag1_operate(APP_FLAG_ROLE_SWITCH, 0); 

    if(get_tws_env_stereo_role() == TWS_PRIM_SEC_UNDEFINED)
    {/*
        should not enter into here;
    */
        TWS_DBG_ERR("UNDEFINED connect. mistake\r\n");
        tws_launch_diac_pairing();
    }
    else if(get_tws_env_stereo_role() == TWS_PRIM_SEC_SECOND)
    {/*
        S1. If there is no connection, and last stereo role is [ SECOND ], page the last ¡¾TWS-MASTER¡¿ first;
        S2. If page fail, then launch the tws DIAC pairing;
    */
        TWS_DBG("SECOND connect:"BTADDR_FORMAT"\r\n", BTADDR(&(app_h->stereo_btaddr)));
        #if 1
        /* This can avoid paging TWS-MASTER directly before TWS-MASTER connected the Phone. */
        tws_launch_diac_pairing(); 
        #else

        #endif
    }
    else if(get_tws_env_stereo_role() == TWS_PRIM_SEC_PRIMARY)
    {/*
        S1. If there is no connection, and last stereo role is [ PRIMARY ], page the last ¡¾Phone¡¿ first;
        S2. If page fail, launch the tws DIAC pairing;
    */
        TWS_DBG("PRIMARY connect:"BTADDR_FORMAT"\r\n", BTADDR(&(app_h->remote_btaddr)));
        
        /* Only let Phone to page me, don't allow TWS-SLAVE to page me before I connected Phone, need to do something more. */
        bt_unit_set_scan_enable(app_h->unit, HCI_PAGE_SCAN_ENABLE); 

        // Reconnect back to the Phone.
        #if 1
        app_h->auto_conn_status = INITIATIVE_CONNECT_START;
        app_bt_allpro_conn_start(AUTO_RE_CONNECT_SCHED_DELAY_HFP, &app_h->remote_btaddr);
        #endif
    }
    else
    {/*
        should not enter into here;
    */
		app_bt_stereo_auto_conn_stop();

        TWS_DBG_ERR(" role error! 0x%x, mistake\r\n", get_tws_env_stereo_role());
    }
    
	#endif    
}
#endif

/* For the disconnection between TWS-Master and TWS-Slave. */
void tws_stereo_acl_disconn_wrap(uint8 reason, btaddr_t *raddr)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    os_printf("tws_discn_wrap_reaon: 0x%x,%d\r\n", reason,get_tws_prim_sec());

    bt_flag2_operate(APP_FLAG2_STEREO_WORK_MODE, 0);
    //bt_flag2_operate(APP_FLAG2_STEREO_AUTOCONN, 0);
	
    unset_tws_flag(TWS_FLAG_CONNECTING_SLAVE);
    unset_tws_flag(TWS_FLAG_MASTER_SLAVE_A2DP_STREAM_CONNECTED);
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
    if (app_is_linein_mode())
    {
        app_set_led_event_action(LED_EVENT_LINEIN_PLAY);
    #ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
        linein_sbc_alloc_free();
        if (bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE)
            || bt_flag2_is_set(APP_FLAG2_STEREO_AUTOCONN))
            bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
        else
            bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
    #else
        bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);	
    #endif
        if(!bt_flag1_is_set(APP_FLAG_WAVE_PLAYING))
            BK3000_Ana_Line_enable(1);
    }	
//#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
    app_bt_slave_linein_set(0);
#endif	
#if (CONFIG_APP_MP3PLAYER == 1)
    if(app_is_mp3_mode())
    {
        bt_flag1_operate(APP_FLAG_MUSIC_PLAY|APP_BUTTON_FLAG_PLAY_PAUSE, 0);
        bt_flag2_operate( APP_FLAG2_STEREO_INQUIRY_RES|APP_FLAG2_STEREO_AUTOCONN|APP_FLAG2_STEREO_WORK_MODE, 0 );
        bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);	
        return ;
    }
#endif

    if(bt_flag2_is_set(APP_FLAG2_STEREO_BUTTON_PRESS) && !bt_flag1_is_set(APP_FLAG_DUT_MODE_ENABLE))
    {
        jtask_stop(sys_hdl->app_stereo_task);
        jtask_schedule(sys_hdl->app_stereo_task, 200, (jthread_func)app_button_stereo, (void *)NULL);
        return;
    }
    else if (get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)
    {
        if(!bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION|APP_FLAG_HFP_CONNECTION))
        {
            sys_hdl->flag_sm1 &= ~APP_FLAG_ACL_CONNECTION;
            if(bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION))
            {
                bt_flag2_operate(APP_FLAG2_STEREO_CONNECTTING, 0);
                bt_auto_connect_start();
            }
            else
                app_bt_check_inquiry_set(0);
        }
        else
            bt_unit_twscontrol_scan(HCI_PAGE_SCAN_ENABLE);
    }
    else if ((get_tws_prim_sec()==TWS_PRIM_SEC_SECOND) && !get_tws_flag(TWS_FLAG_POWER_DOWN))
    {
        //set_tws_prim_sec(TWS_PRIM_SEC_UNDEFINED);
        app_bt_check_inquiry_set(0);
        if (!bt_flag2_is_set(APP_FLAG2_STEREO_MATCH_BUTTON))
        {
            sys_hdl->flag_sm2 |= APP_FLAG2_STEREO_AUTOCONN;
            app_set_led_event_action(LED_EVENT_STEREO_RECON_MODE);
	     os_printf("task----app_bt_inquiry_active_conn\r\n");
            jtask_stop(sys_hdl->app_stereo_task);
            jtask_schedule(sys_hdl->app_stereo_task, 1000, (jthread_func)app_bt_inquiry_active_conn,(void *)TWS_PAGE_TIME_INTERVAL);
        }
        else
            sys_hdl->flag_sm2 &= ~APP_FLAG2_STEREO_AUTOCONN;
            
        sys_hdl->flag_sm1 &= ~APP_FLAG_ACL_CONNECTION;
    }

    bt_flag2_operate(APP_FLAG2_STEREO_MATCH_BUTTON, 0);

    if((!bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION|APP_FLAG_HFP_CONNECTION))
        && (!get_tws_flag(TWS_FLAG_TWS_ROLE_SWITCH_FAIL)))
    {
        app_wave_file_play_start(APP_WAVE_FILE_ID_STEREO_DISCONN);	
    }

	app_bt_reenter_dut_mode();
}

void tws_stereo_autoconn_cnt_set(uint8 value)
{
	stereo_autoconn_cnt = value;
}

uint8 tws_stereo_autoconn_cnt_get(void)
{
	return stereo_autoconn_cnt;
}

void tws_page_time_out_cb(void *btaddr)
{
    //TWS_DBG("tws_page_time_out_cb: %d-%d\r\n", get_tws_env_stereo_role(), get_tws_prim_sec());
	if (tws_stereo_autoconn_cnt_get())
	{
		stereo_autoconn_cnt ++;
		if (tws_stereo_autoconn_cnt_get() > 2)
		{
			tws_stereo_autoconn_cnt_set(0);
			
			if (bt_flag1_is_set(APP_FLAG_ACL_CONNECTION))
				return;

			//set_tws_prim_sec(TWS_PRIM_SEC_UNDEFINED);
			app_bt_check_inquiry_set(1);
			app_bt_inquiry_active_conn((void *)TWS_PAGE_TIME_INTERVAL);
		#if CONFIG_TWS_AUTOCONNECT
			app_handle_t sys_hdl = app_get_sys_handler();
			jtask_schedule(sys_hdl->tws_linein_task, 500, (jthread_func)app_button_stereo_slave_action, NULL);;
		#endif	
		}
		os_printf("stereo_autoconn_cnt=%d\n",stereo_autoconn_cnt);
	}	
#if 0
    /* Part 1. TWS-MASTER page Phone time out. */
    if((get_tws_env_stereo_role() == TWS_PRIM_SEC_PRIMARY) 
    && (!bt_flag1_is_set(APP_FLAG_ACL_CONNECTION))
    && get_tws_prim_sec() == TWS_PRIM_SEC_UNDEFINED)
    {
        TWS_DBG("Last PRIMARY paging Phone...\r\n");

        tws_launch_diac_pairing();
        return;
    }    
    
    /* Part 2. TWS-SLAVE pairing TWS-MASTER time out. */
    if((get_tws_env_stereo_role() == TWS_PRIM_SEC_SECOND) 
    && (!bt_flag1_is_set(APP_FLAG_ACL_CONNECTION))
    && get_tws_prim_sec() == TWS_PRIM_SEC_UNDEFINED)
    {
        TWS_DBG("Last SECOND pairing TWS-MASTER...\r\n");

        return;
    }    
    
    /* Part 3. TWS-SLAVE Re-Pairing TWS-MASTER time out. */
    if((!bt_flag1_is_set(APP_FLAG_ACL_CONNECTION))
    && get_tws_prim_sec() == TWS_PRIM_SEC_UNDEFINED)
    {
        TWS_DBG("Re-Pairing TWS-MASTER...\r\n");

        tws_launch_diac_pairing();
        return;
    }    
#endif    
}

extern app_sbc_t  app_sbc;
//extern AUDIO_CTRL_BLK audio_ctrl_blk;
inline uint8 tws_piconet_role_switch_decision(void *bd_addr)
{
    t_error status = NO_ERROR;
#if (CONFIG_TWS_KEY_MASTER==1)
    if (bt_flag2_is_set(APP_FLAG2_STEREO_INQUIRY_RES)
        || bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION|APP_FLAG_HFP_CONNECTION))
    {
        os_printf("TWS ROLE NOT_ALLOWED\r\n");
        return ROLE_CHANGE_NOT_ALLOWED;
    }
    if (bt_flag2_is_set(APP_FLAG2_STEREO_AUTOCONN))
    {
        os_printf("TWS ROLE ALLOWED\r\n");
        return NO_ERROR;
    }
#endif	

    if(NULL == bd_addr)
    {
        status = ROLE_CHANGE_NOT_ALLOWED;
    }

	if(get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)
    {
        status = ROLE_CHANGE_NOT_ALLOWED;        
    }
    os_printf(">[TWS mss:%d\r\n",status);
    return status;
}

boolean tws_judge_piconet_role(void *peer_addr)
{
    btaddr_t *addr = (btaddr_t *)peer_addr;
    app_handle_t sys_hdl = app_get_sys_handler();
    app_tws_t app_tws_h = app_get_tws_handler();
	
    if (!addr)
    {
        TWS_DBG_ERR("invalid param! NULL\r\n");
        return FALSE;
    }

    TWS_DBG("tws_piconet:"BTADDR_FORMAT", M|S: %d, R: %d\r\n", BTADDR(addr), get_tws_prim_sec(), get_piconet_role(peer_addr));
    
    if (TWS_PRIM_SEC_PRIMARY == get_tws_prim_sec())
    {
    
        if (HCI_ROLE_SLAVE == get_piconet_role(peer_addr))
        {
            os_printf("master_tws_judge fail:%d\r\n",is_peer_tws(peer_addr));
            if (is_peer_tws(peer_addr))
            {
                set_tws_flag(TWS_FLAG_TWS_ROLE_SWITCH_FAIL);
                bt_unit_acl_disconnect_with_reason(sys_hdl->unit, peer_addr, HCI_ERR_ROLE_SWITCH_FAILED);
            }
            else  // peer is phone.
            {
            	  app_tws_h->piconet_role = HCI_ROLE_SLAVE;
                bt_flag2_operate(APP_FLAG2_RECONN_AFTER_DISCONNEC, 1);
                bt_unit_acl_disconnect_with_reason(sys_hdl->unit, peer_addr, HCI_ERR_REMOTE_USER_TERMINATED_CONNECTION);
            }

            return FALSE;
        }
        
        app_set_role_switch_enable(APP_ROLE_SWITCH_DISABLE);

        if (is_peer_tws(peer_addr))
        {
            unset_tws_flag(TWS_FLAG_TWS_ROLE_SWITCH_FAIL);
        }
		else
			app_tws_h->piconet_role = HCI_ROLE_MASTER;
    }
    #if 0
    else if (TWS_PRIM_SEC_SECOND == get_tws_prim_sec())
    {

        if (HCI_ROLE_MASTER == get_piconet_role(peer_addr)
            && is_peer_tws(peer_addr))
        {
            set_tws_flag(TWS_FLAG_TWS_ROLE_SWITCH_FAIL);
            os_printf("slave_tws_judge fail!\r\n");
            //bt_unit_acl_disconnect(sys_hdl->unit, peer_addr);
            bt_unit_acl_disconnect_with_reason(sys_hdl->unit, peer_addr, HCI_ERR_ROLE_SWITCH_FAILED);
            return FALSE;
        }
        
        if (is_peer_tws(peer_addr))
        {
            unset_tws_flag(TWS_FLAG_TWS_ROLE_SWITCH_FAIL);
        }
    }
    #endif
	
    tws_set_retry_count(0);
    
    return TRUE;
}

void tws_power_down(void)
{
    set_tws_flag(TWS_FLAG_POWER_DOWN);
    os_printf("tws_power_down()\r\n");
    start_wave_and_action(APP_WAVE_FILE_ID_POWEROFF, app_powerdown);
}

void tws_slave_page(void)
{
    app_handle_t app_h = app_get_sys_handler();
    //TWS_DBG_ERR("%s"BTADDR_FORMAT"\r\n", __func__, BTADDR(&app_h->stereo_btaddr));
    if (!get_tws_flag(TWS_FLAG_POWER_DOWN) && app_h->unit)
    {
        bt_unit_acl_connect(app_h->unit, &app_h->stereo_btaddr);
    }
}

/******************************************************** 
For tws sync_mechanism.
Begin:
********************************************************/

void tws_clear_a2dp_buffer(void) // It must be excuted after [set_flag_sbc_buffer_play(0)].
{
    //S_DBG_ERR("%s, %d-%d\r\n", __func__, app_sbc.sbc_ecout, get_sbc_mem_pool_node_left());
    os_printf("tws_clear_a2dp_buffer:%d,%d\n", app_sbc.sbc_ecout, get_sbc_mem_pool_node_left());
    clean_all_sbc_frame_node();
    set_tws_hfp_flag(TWS_HFP_FLAG_DECODE_FIRST_MSBC);
}



#if TWS_A2DP_AVOID_POP_ENABLE
#define TWS_A2DP_AVOID_POP_DELAY       120
static uint32 s_tws_a2dp_avoid_pop_sound_cnt = 0; //  10ms

uint32 tws_a2dp_get_avoid_pop_sound_cnt(void)
{
    return s_tws_a2dp_avoid_pop_sound_cnt;
}

void tws_a2dp_avoid_pop_sound(void)
{
    //return;
    aud_volume_set(0);
    s_tws_a2dp_avoid_pop_sound_cnt = TWS_A2DP_AVOID_POP_DELAY;
}

void tws_a2dp_avoid_pop_sound_cnt_chk(void)
{
    if (bt_flag1_is_set(APP_FLAG_MUSIC_PLAY)
        && (s_tws_a2dp_avoid_pop_sound_cnt > 0)
    )
    {
        s_tws_a2dp_avoid_pop_sound_cnt--;

        #if 0
        uint32 tmp = a2dp_get_volume(), cur_vol = 0;

        cur_vol = ((TWS_A2DP_AVOID_POP_DELAY - s_tws_a2dp_avoid_pop_sound_cnt) * tmp) / TWS_A2DP_AVOID_POP_DELAY;

        if (cur_vol <= tmp)
        {
            aud_volume_set(cur_vol);
        }
        else
        {
            os_printf("err!%d,%d\n", cur_vol, tmp);
        }
        #endif
        
        if (0 == s_tws_a2dp_avoid_pop_sound_cnt)
        {
            aud_volume_set(a2dp_get_volume());
        }
    }
}
#endif











static boolean s_tws_call_exist = FALSE;
inline void tws_set_call_exist(boolean exist)
{
    //os_printf("%s,%d\n", __func__, exist);
    os_printf("call_e,%d\n", exist);
    
    if((FALSE == s_tws_call_exist)
     && (TRUE == exist)
    )
    {
        #if 0//(CONFIG_AUD_FADE_IN_OUT == 1)
        set_aud_fade_in_out_state(AUD_FADE_OUT);
        #endif
    }

    if (get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY
        && (FALSE == s_tws_call_exist)
        && (TRUE == exist)
        && bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE) 
    )  
    {/* TWS-MASTER tells the TWS-SLAVE call. */
    
        #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
        app_handle_t sys_hdl = app_get_sys_handler();
        uint16_t handle = bt_sniff_get_handle_from_raddr(&sys_hdl->stereo_btaddr);
        app_bt_write_sniff_link_policy(handle, 0);
        #endif
        
        send_ct_cmd_test((void *)AVC_OP_TWS_CALL);
    }

    if (get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY
        && (TRUE == s_tws_call_exist)
        && (FALSE == exist)
        && bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE) 
    )  
    {/* TWS-MASTER tells the TWS-SLAVE call end. */
        send_ct_cmd_test((void *)AVC_OP_TWS_CALL_END);
    }
    
    s_tws_call_exist = exist;
}

inline boolean tws_get_call_exist(void)
{
    return s_tws_call_exist;
}

inline void tws_set_free_mem_flag(void)
{
    set_tws_flag(TWS_FLAG_NEED_FREE_MEMORY);
}

inline void tws_unset_free_mem_flag(void)
{
    unset_tws_flag(TWS_FLAG_NEED_FREE_MEMORY);
}

inline uint32 tws_get_free_mem_flag(void)
{
    return get_tws_flag(TWS_FLAG_NEED_FREE_MEMORY);
}

/********************************************************
For tws sync_mechanism.
End
********************************************************/

void tws_master_linein_encode_open(void)
{
    //TWS_DBG("%s\n", __func__);
    //os_printf("L.O\n");
    unset_tws_flag(TWS_FLAG_MASTER_LINEIN_CLOSE_ENCODE);
}

void tws_master_linein_encode_close(void)
{
    //TWS_DBG("%s\n", __func__);
    //os_printf("L.C-%x\n", get_tws_flag(TWS_FLAG_MASTER_LINEIN_CLOSE_ENCODE));

    if (get_tws_flag(TWS_FLAG_MASTER_LINEIN_CLOSE_ENCODE))
        return;

    set_tws_flag(TWS_FLAG_MASTER_LINEIN_CLOSE_ENCODE);
    app_handle_t app_h = app_get_sys_handler();
    //jtask_schedule(app_h->tws_linein_task, 10 /* ms */, (jthread_func)tws_master_linein_encode_open, NULL);
    // jtask_schedule(app_h->tws_linein_task, 100 /* ms */, (jthread_func)tws_master_linein_encode_open, NULL);
    //jtask_schedule(app_h->tws_linein_task, 500 /* ms */, (jthread_func)tws_master_linein_encode_open, NULL);
    jtask_schedule(app_h->tws_linein_task, TWS_M_RE_SYNC_DELAY_TIME /* ms */, (jthread_func)tws_master_linein_encode_open, NULL);
}



uint8 tws_hci_eir_type_beken(uint8 type)
{
	if (type == 0)/* BEKEN VENDOR DEPENDENT TYPE */
		return HCI_EIR_TYPE_BEKEN;
	else
		return TWS_IN_EIR;
}

static uint8 beken_info_in_eir_data[HCI_EIR_TYPE_BEKEN_SIZE] = {PRH_BS_CFG_MANUFACTURER_NAME & 0xFF,
                                                                                                  (PRH_BS_CFG_MANUFACTURER_NAME >> 8) & 0xFF,
                                                                                                  TWS_IN_EIR, 0, 0, 0};

uint8 *tws_get_beken_info_in_eir_data(void)
{
    return &beken_info_in_eir_data[0];
}

static uint8 s_tws_auto_connect_back_retry_count = 0;

uint8 tws_get_retry_count(void)
{
    return s_tws_auto_connect_back_retry_count;
}

void tws_increase_retry_count(void)
{
    s_tws_auto_connect_back_retry_count++;
}

void tws_set_retry_count(uint8 cnt)
{
    os_printf("tws retry cnt:%d,%d\n", s_tws_auto_connect_back_retry_count, cnt);
    s_tws_auto_connect_back_retry_count = cnt;
}

void tws_slave_conn_back_stage_two(void)
{
    os_printf("tws_slave_conn_two:%d_0x%x\n",get_tws_prim_sec(), bt_flag1_is_set(APP_FLAG_ACL_CONNECTION));
    
    if (TWS_PRIM_SEC_UNDEFINED == get_tws_prim_sec()
        && (!bt_flag1_is_set(APP_FLAG_ACL_CONNECTION))
    )
    {/* The TWS_S befroe power on cann't has any ACL link until now.  */
        tws_set_retry_count(0);
        
        app_handle_t sys_hdl = app_get_sys_handler();
        
        bt_flag2_operate(APP_FLAG2_STEREO_AUTOCONN, 1);
        
        if (1)
        {/* TWS_S re-connect back to the TWS_M. */
            bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
            app_set_led_event_action(LED_EVENT_STEREO_RECON_MODE);
            app_bt_inquiry_active_conn((void *)0);
            jtask_schedule(sys_hdl->app_auto_con_task, 8000, (jthread_func)
                                        app_bt_flag2_set0,
                                        (void *)NULL);
        }
    }
}

void tws_slave_prepare_conn_back_stage_two(void)
{/* The TWS_S befroe power on cann't has any ACL link until now.  */
    os_printf("tws_slave_prepare_conn_two()\n");
    app_handle_t app_h = app_get_sys_handler();
    jtask_schedule(app_h->tws_linein_task, 5120 * 2 /* ms */, (jthread_func)tws_slave_conn_back_stage_two, NULL);
}

uint8 tws_sbc_max_bitpool_set(void)
{
	return TWS_SBC_MAX_BITPOOL; 
}

uint8 tws_slave_auto_calibration_condition(void)
{
	if ((get_tws_prim_sec()==TWS_PRIM_SEC_SECOND)
		|| bt_flag2_is_set(APP_FLAG2_STEREO_AUTOCONN))
	{
		return 1;
	}

	return 0;
}

#if TWS_A2DP_AVOID_SBC_NODE_LEVEL_TOO_LOW_ENABLE

#define BASE_SBC_FREQ_ADJUST_PPM_MAX      (1000)   /* +/- 1000ppm. */
#define BASE_SBC_FREQ_ADJUST_PPM_STEP     (50)     /* 50ppm. */

static int32 s_base_sbc_freq_adjust_ppm = 0;

/* get DAC DIV. */
uint32 tws_a2dp_base_sbc_dac_div(void)
{
    uint32 dac_fraq = (get_dac_sample_rate() == 44100) ? AUDIO_DIV_441K : AUDIO_DIV_48K;

    if (0 == s_base_sbc_freq_adjust_ppm)
    {
        return dac_fraq;
    }

    #if 0
    dac_fraq -= (int32_t)((int64_t)dac_fraq * (s_base_sbc_freq_adjust_ppm) / 1000000);
    #else
    if (AUDIO_DIV_441K == dac_fraq)
    {
        dac_fraq -= (s_base_sbc_freq_adjust_ppm * AUDIO_DIV_441K_1PPM) ;
    }
    else
    {
        dac_fraq -= (s_base_sbc_freq_adjust_ppm * AUDIO_DIV_48K_1PPM) ;
    }
    #endif
    
    return dac_fraq;
}

int32 tws_a2dp_base_sbc_freq_adj_ppm_get(void)
{
    return s_base_sbc_freq_adjust_ppm;
}

/***************************************************
*  It's only for TWS_S.
****************************************************/
void tws_slave_a2dp_base_sbc_freq_adj_ppm_set(int32 base_sbc_freq_adj_ppm)
{
    os_printf("s_adj_ppm:%d,%d\n",base_sbc_freq_adj_ppm, s_base_sbc_freq_adjust_ppm);
    
    if (base_sbc_freq_adj_ppm <= BASE_SBC_FREQ_ADJUST_PPM_MAX
        && base_sbc_freq_adj_ppm >= -BASE_SBC_FREQ_ADJUST_PPM_MAX)
    {
        s_base_sbc_freq_adjust_ppm = base_sbc_freq_adj_ppm;
    }
    else
    {
        os_printf("adj_ppm error!%d,%d\n", base_sbc_freq_adj_ppm, s_base_sbc_freq_adjust_ppm);
    }
}

/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_a2dp_send_base_sbc_freq_adj_ppm_to_twsS(void)
{ 
    uint8_t buff[8] = {0};
    
    buff[0] = TWS_A2DP_CMD_SET_BASE_SBC_FREQ_ADJ_PPM;
    
    uint32 u_adj_ppm = (uint32)s_base_sbc_freq_adjust_ppm;        
    buff[1] = u_adj_ppm & 0xFF;
    buff[2] = (u_adj_ppm >> 8)  & 0xFF;
    buff[3] = (u_adj_ppm >> 16) & 0xFF;
    buff[4] = (u_adj_ppm >> 24) & 0xFF;
    
    tws_hfp_send_cmd(buff, 8);
    
    os_printf("M_S_adj_ppm:%d\n", s_base_sbc_freq_adjust_ppm);
}

/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_a2dp_base_sbc_freq_make_self_slow(void)
{ 
    if (get_tws_flag(TWS_FLAG_BASE_SBC_FREQ_ADJ_PPM_BUSY))
    {
        return;
    }
    tws_a2dp_base_sbc_freq_adj_enter();

    // S1. TWS_M adjust self adjust ppm.
    if (s_base_sbc_freq_adjust_ppm > -BASE_SBC_FREQ_ADJUST_PPM_MAX)
    {
        s_base_sbc_freq_adjust_ppm -= BASE_SBC_FREQ_ADJUST_PPM_STEP;

        // S2. TWS_M send self adjust ppm to TWS_S.
        tws_a2dp_send_base_sbc_freq_adj_ppm_to_twsS();
    }
}

/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_a2dp_base_sbc_freq_make_self_fast(void)
{ 
    if (get_tws_flag(TWS_FLAG_BASE_SBC_FREQ_ADJ_PPM_BUSY))
    {
        return;
    }
    tws_a2dp_base_sbc_freq_adj_enter();

    // S1. TWS_M adjust self adjust ppm.
    if (s_base_sbc_freq_adjust_ppm < BASE_SBC_FREQ_ADJUST_PPM_MAX)
    {
        s_base_sbc_freq_adjust_ppm += BASE_SBC_FREQ_ADJUST_PPM_STEP;

        // S2. TWS_M send self adjust ppm to TWS_S.
        tws_a2dp_send_base_sbc_freq_adj_ppm_to_twsS();
    }
}

/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_master_a2dp_base_sbc_freq_adj_ppm_set(int const ppm)
{ 
    if (get_tws_flag(TWS_FLAG_BASE_SBC_FREQ_ADJ_PPM_BUSY))
    {
        return;
    }

    if (ppm == s_base_sbc_freq_adjust_ppm)
    {
        return;
    }

    tws_a2dp_base_sbc_freq_adj_enter();

    // S1. TWS_M adjust self adjust ppm.
    if (ppm >= -BASE_SBC_FREQ_ADJUST_PPM_MAX
        && ppm <=BASE_SBC_FREQ_ADJUST_PPM_MAX)
    {
        s_base_sbc_freq_adjust_ppm = ppm;

        // S2. TWS_M send self adjust ppm to TWS_S.
        tws_a2dp_send_base_sbc_freq_adj_ppm_to_twsS();
    }
}


#define BASE_SBC_FREQ_ADJ_WAIT_CNT       11 // 11 * 10ms.
uint32 s_base_sbc_adj_ppm_wait_cnt = 0;
void tws_a2dp_base_sbc_freq_adj_enter(void)
{
    s_base_sbc_adj_ppm_wait_cnt = BASE_SBC_FREQ_ADJ_WAIT_CNT;
    set_tws_flag(TWS_FLAG_BASE_SBC_FREQ_ADJ_PPM_BUSY);
}

void tws_a2dp_base_sbc_freq_adj_chk(void)
{
    if (s_base_sbc_adj_ppm_wait_cnt > 0)
    {
        s_base_sbc_adj_ppm_wait_cnt--;
        if (0 == s_base_sbc_adj_ppm_wait_cnt)
        {
            unset_tws_flag(TWS_FLAG_BASE_SBC_FREQ_ADJ_PPM_BUSY);
        }
    }
}

#endif

#ifdef CONFIG_BLUETOOTH_AVDTP_SCMS_T
/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_a2dp_send_content_protect_flag_to_twsS(uint8 flag)
{ 
    uint8_t buff[4] = {0};
    
    buff[0] = TWS_A2DP_CMD_SET_CONTENT_PROTECT_FLAG;
    
    buff[1] = flag;
    
    tws_hfp_send_cmd(buff, 4);
    
    os_printf("tws_M_send_SCMS_T_to_S(%d)\n",flag);
}
#endif




#endif /* CONFIG_TWS */
//EOF
