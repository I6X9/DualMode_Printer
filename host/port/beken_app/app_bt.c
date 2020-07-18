#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "bt_init.h"
#ifdef CONFIG_TWS
#include "app_tws.h"
#include "tws_sco_sync.h"
#include "tc_const.h"
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
#include "../../../bluetooth/profile/a2dp/bt_sbc.h"
#endif
#endif
#ifdef IPHONE_BAT_DISPLAY
extern uint8 iphone_bat_lever_bak;
extern uint8 iphone_bat_lever_bak_cnt;
#endif

#ifdef CONFIG_PRODUCT_TEST_INF
static btaddr_t remote_btaddr_def = {{0x62, 0x32, 0x17, 0x20, 0x18, 0x05}};
#endif
#if 0
static BT_ENTITY_T g_bluetooth = {
    0,              /* init flag*/
    {{0x77, 0x77, 0x69, 0x96, 0x66, 0x66}}, /*btaddr_def default bluetooth address*/
};
#endif
#if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
#ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
static BT_SNIFF_T  g_sniff_st[NEED_SNIFF_DEVICE_COUNT];
#endif
#endif


static BT_ENTITY_T Remote_BlueDev = {0,{{0}}};
//BT_ENTITY_T dis_dev = {0,{{0}}};
static int32 retry_count = 0;
/* api declaration*/
result_t uart_init_and_attach();
void *a2dp_get_current_app(void);
//uint32_t a2dp_app_get_count(void);
//uint32_t a2dp_all_is_not_music(void);
void *hfp_select2_current_app(void *arg);
uint32 hfp_get_remote_addr(void *arg, btaddr_t *r_addr);
extern uint32 a2dp_get_remote_addr(void *arg, btaddr_t *r_addr);
//extern btaddr_t a2dp_get_addr_from_ptr(void *arg);


#if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
#ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
void bt_sniff_alloc_st(uint8_t idx,uint16_t handle,btaddr_t *rbtaddr)
{
    idx &= 0x01;
    g_sniff_st[idx].is_active = 1;
    g_sniff_st[idx].is_used = 1;
    g_sniff_st[idx].is_policy = 0;
    g_sniff_st[idx].link_handle = handle;

    memcpy((uint8 *)&g_sniff_st[idx].remote_btaddr,(uint8 *)rbtaddr,sizeof(btaddr_t));
}

void bt_sniff_free_st(uint8_t idx)
{
    idx &= 0x01;
    g_sniff_st[idx].is_active = 0;
    g_sniff_st[idx].is_used = 0;
    g_sniff_st[idx].link_handle = 0;
    g_sniff_st[idx].is_policy = 0;
}
uint8_t bt_sniff_find_st_available(void)
{
    uint8_t idx = 0;
    for(; idx<NEED_SNIFF_DEVICE_COUNT; idx++)
    {
        if(g_sniff_st[idx].is_used == 0)
        {
            return idx;
        }
    }
    return 0;
}
uint16_t bt_sniff_get_handle_from_raddr(btaddr_t *rbtaddr)
{
    uint8_t idx = 0;
    for(; idx<NEED_SNIFF_DEVICE_COUNT; idx++)
    {
        if(btaddr_same(rbtaddr,&g_sniff_st[idx].remote_btaddr))
        {
            return g_sniff_st[idx].link_handle;
        }
    }
    return 0;
}
uint8_t bt_sniff_get_index_from_raddr(btaddr_t *rbtaddr)
{
    uint8_t idx = 0;
    for(; idx<NEED_SNIFF_DEVICE_COUNT; idx++)
    {
        if(btaddr_same(rbtaddr,&g_sniff_st[idx].remote_btaddr))
        {
            return idx;
        }
    }
    return 0;
}
uint8_t bt_sniff_get_index_from_handle(uint16_t handle)
{
    uint8_t idx = 0;
    for(; idx<NEED_SNIFF_DEVICE_COUNT; idx++)
    {
        if(g_sniff_st[idx].link_handle == handle)
        {
            return idx;
        }
    }
    return 0;
}
uint16_t bt_sniff_get_handle_from_idx(uint8_t idx)
{
    return g_sniff_st[idx].link_handle;
}

btaddr_t bt_sniff_get_rtaddr_from_idx(uint8_t idx)
{
    return g_sniff_st[idx].remote_btaddr;
}
btaddr_t *bt_sniff_get_addrptr_from_idx(uint8_t idx)
{
    return (btaddr_t *)&g_sniff_st[idx].remote_btaddr;
}
uint8_t bt_sniff_is_used(uint8_t idx)
{
    return(g_sniff_st[idx].is_used);
}
uint8_t bt_sniff_is_active(uint8_t idx)
{
    return(g_sniff_st[idx].is_active);
}
uint8_t bt_sniff_is_policy(uint8_t idx)
{
    return(g_sniff_st[idx].is_policy);
}
void bt_sniff_set_policy(uint8_t idx,uint8_t enable)
{
    g_sniff_st[idx].is_policy = enable;
}
void bt_sniff_set_active(uint8_t idx,uint8_t enable)
{
    g_sniff_st[idx].is_active = enable&0x01;
}
#ifdef CONFIG_TWS
inline uint8_t tws_is_all_active(void)
{
    uint8_t idx = 0;
    for(idx = 0; idx < NEED_SNIFF_DEVICE_COUNT; idx++)
    {
        if(!g_sniff_st[idx].is_active)
        {
            return FALSE;
        }
    }
    
    return TRUE;
}
#endif
#else
uint8_t bt_sniff_get_index_from_handle(uint16_t handle)
{
    return 0;
}
uint16_t bt_sniff_get_handle_from_idx(uint8_t idx)
{
    return 0;
}
uint8_t bt_sniff_is_used(uint8_t idx)
{
    return 0;
}
uint8_t bt_sniff_is_active(uint8_t idx)
{
    return 0;
}
void bt_sniff_set_active(uint8_t idx,uint8_t enable)
{

}
#ifdef CONFIG_TWS
uint8_t tws_is_all_active(void)
{   
    return 0;
}
#endif

#endif
#endif
#if 0
/* api routines*/
void bt_install(void)
{
    uint32 ret;

    if(g_bluetooth.init_flag)
    {
        return;
    }

    #ifdef CONFIG_BLUETOOTH
    ret = bt_init();
    if(ret)
    {
        os_printf("bt_init_failed\r\n");
        return;
    }

    //app_bt_env_post_init();
    #endif

    uart_init_and_attach();

    g_bluetooth.init_flag = 1;
}

uint32 bt_is_install(void)
{
    return g_bluetooth.init_flag;
}

btaddr_t *bt_get_default_btaddr_ptr(void)
{
    return &g_bluetooth.btaddr_def;
}
#endif

static void bt_create_reconnect_wrap(void * arg)
{
    uint32 interval;
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();
//    int32 retry = (int32)arg;
//    bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);

    interval = env_h->env_cfg.bt_para.disconn_retry_interval;
#if 0
    if(retry == APP_RETRY_FOREVER)
    {
    	//os_printf("bt_enhance_inquiry_page_scan1:%d\r\n", env_h->env_cfg.bt_para.auto_conn_interval);
        jtask_schedule(sys_hdl->app_auto_con_task,
                        0,
                        bt_create_reconn_timeout_wrap,
                        (void *)arg);
         return;
    }
#endif
    os_printf("bt_reconnect:%d\r\n", interval);

    if(interval >= 500)
    {
        //os_printf("bt_inquiry_page_scan0_r:%d\r\n", interval);
        jtask_schedule(sys_hdl->app_auto_con_task,
                        interval - 200,
                        bt_create_reconn_timeout_wrap,
                        (void *)arg);
    }
    else
    {
        //os_printf("bt_inquiry_page_scan1_r:0\r\n");
        jtask_schedule(sys_hdl->app_auto_con_task,
                        0,
                        bt_create_reconn_timeout_wrap,
                        (void *)arg);
    }
}
static void bt_create_autoconn_wrap(void * arg)
{
    uint32 interval;
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();
    retry_count = (int32)arg;
//    bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);

    interval = env_h->env_cfg.bt_para.auto_conn_interval;
    if(retry_count == APP_RETRY_FOREVER)
    {
    	//os_printf("bt_enhance_inquiry_page_scan1:%d\r\n", env_h->env_cfg.bt_para.auto_conn_interval);
        jtask_schedule(sys_hdl->app_auto_con_task,
                        0,
                        bt_create_conn_timeout_wrap,
                        (void *)retry_count);
         return;
    }
    os_printf("bt_autoconn:%d\r\n", interval);
    if(interval >= 500)
    {
    #ifdef CONFIG_AUTO_CONN_AB
        jtask_schedule(sys_hdl->app_auto_con_task,
                        interval - 200,
                        bt_create_autoconn_timeout_AB_wrap,
                        (void *)retry_count);
    #else
        jtask_schedule(sys_hdl->app_auto_con_task,
                        interval - 200,
                        bt_create_conn_timeout_wrap,
                        (void *)retry_count);
    #endif
    }
    else
    {
    #ifdef CONFIG_AUTO_CONN_AB
        jtask_schedule(sys_hdl->app_auto_con_task,
                        0,
                        bt_create_autoconn_timeout_AB_wrap,
                        (void *)retry_count);
    #else
        jtask_schedule(sys_hdl->app_auto_con_task,
                        0,
                        bt_create_conn_timeout_wrap,
                        (void *)retry_count);
    #endif
    }
}

static void bt_auto_con_timerout_wrap(void *arg)
{
//    bt_flag1_operate(APP_FLAG_DEBUG_FLAG2, 0);

    bt_auto_connect_stop();

#if 0// def CONFIG_TWS
    app_handle_t sys_hdl = app_get_sys_handler();
    if((app_bt_get_disconn_event()==0)
		&& bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
    {
        bt_unit_set_scan_enable(sys_hdl->unit,
                                HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
    }
    else
    {
        bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
    }
#endif
#if 0
    if(bt_flag1_is_set( APP_FLAG_MATCH_ENABLE ) )
    {
        app_set_led_event_action( LED_EVENT_MATCH_STATE);
    }
    else
    {
        #ifndef BT_ONLY_ONE_BT
        app_set_led_event_action( LED_EVENT_NO_MATCH_NO_CONN_STATE );
		#endif
    }
#endif
}

void bt_create_conn_status_wrap(uint8 status)
{
    app_handle_t sys_hdl = app_get_sys_handler();

    //os_printf("create_conn_status_wrap,%x\r\n",status);
    if(bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION) )
    {
        if(status == 0)  // auto connection success
        {
            sys_hdl->flag_sm1 |= APP_FLAG_ACL_CONNECTION;
        }
        else    //auto connection failed
        {
            if(bt_flag1_is_set(APP_FLAG_MATCH_ENABLE))
            {
                app_set_led_event_action(LED_EVENT_MATCH_STATE);
                if(app_check_bt_mode(BT_MODE_1V1))
                {
                #ifdef CONFIG_BLUETOOTH_COEXIST
                    if(app_is_bt_mode())
                #endif
                        app_wave_file_play_start(APP_WAVE_FILE_ID_ENTER_PARING);
                }
            }
            else
            {
                app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);
            }
        }
    }
    else if(bt_flag1_is_set(APP_FLAG_RECONNCTION))
    {
        //os_printf("create_conn1\r\n");
        if(status == 0)
        {
            sys_hdl->flag_sm1 |= APP_FLAG_ACL_CONNECTION;
        }
	#if 0
		// will lead to reconnection fail (link num<10 ,reconnect ->auto conn)
        else
        {
            jtask_stop(sys_hdl->app_auto_con_task);
            if(!(bt_flag1_is_set( APP_FLAG_ACL_CONNECTION)))
            {
                unit_acl_connect(sys_hdl->unit, &sys_hdl->remote_btaddr);
            }

            jtask_schedule(sys_hdl->app_auto_con_task,
                                APP_DISCONN_ACTION_TIMEOUT,
                                bt_create_conn_timeout_wrap,
                                (void *)0);
        }
	#endif
    }
}

void bt_create_reconn_timeout_wrap(void *arg)
{
    uint32 max_count = 0;
    hci_link_t *link = 0;
    btaddr_t *address_ptr = NULL;
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();
    if((int32)arg != APP_RETRY_FOREVER)
        retry_count = (int32)arg;

    CLEAR_SLEEP_TICK;
    bt_flag1_operate(APP_FLAG_AVRCP_FLAG, 0);
    jtask_stop(sys_hdl->app_match_task);
#if (BT_MODE==BT_MODE_1V2)
#ifndef  CONN_WITH_MUSIC
    if(a2dp_has_music())
    {
        bt_flag2_operate(APP_FLAG2_RECONN_AFTER_PLAY,1);
        return;
    }
#endif
    if(hfp_has_sco_conn())
    {
        bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL,1);
        return;
    }
#endif

    if(bt_flag1_is_set(APP_FLAG_RECONNCTION))// process actions after disconnect
    {
    #ifndef CONFIG_BLUETOOTH_COEXIST
        if(SYS_WM_BT_MODE !=sys_hdl->sys_work_mode)
        {
            bt_flag1_operate(APP_FLAG_RECONNCTION, 0);
            bt_auto_connect_stop();
            return ;
        }
    #endif
    	 address_ptr = &sys_hdl->reconn_btaddr;
        max_count = env_h->env_cfg.bt_para.disconn_retry_count;

        //if ((retry_count <= max_count) || (max_count == APP_RETRY_FOREVER))
        if (((retry_count <= max_count) || (max_count == APP_RETRY_FOREVER))
        #ifdef CONFIG_TWS
            || bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_DISCONNEC) /* if judge role with phone fail, continue re-connect back to the phone. */
        #endif
        )
        {
        #ifdef CONFIG_TWS
            hci_get_acl_link_check(sys_hdl->unit, &sys_hdl->remote_btaddr);
        #endif
            link = hci_link_lookup_btaddr(sys_hdl->unit, address_ptr, HCI_LINK_ACL);
        #if 0
            if(link && (HCI_LINK_OPEN != link->hl_state))
            {
                unit_send_cmd(sys_hdl->unit,
                                HCI_CMD_CREATE_CON_CANCEL,
                                address_ptr,
                                BTADDR_LEN);
                jtask_schedule(sys_hdl->app_auto_con_task,
                               200,
                               bt_create_reconnect_wrap,
                               (void *)APP_RETRY_FOREVER);
            }
        #endif
            if(!link)
            {
            	  retry_count ++;
                app_sleep_func(0);
                bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
                os_printf("acl connect:%d,%d " BTADDR_FORMAT "\r\n", retry_count, max_count,BTADDR(address_ptr));
                hf_clear_autoconn_diconned_flag((void*)address_ptr);
                bt_unit_acl_connect(sys_hdl->unit, &sys_hdl->reconn_btaddr);
                sys_hdl->auto_conn_status = INITIATIVE_CONNECT_START;
            }

            os_printf("link is:%p\r\n", link);

            if(link && (HCI_LINK_OPEN == link->hl_state))
            {
            #ifdef BEKEN_DEBUG
                os_printf("Link already exist;reconn_flag:%d\r\n",sys_hdl->reconn_num_flag);
            #endif
            }
            else
            {
                if(link && (HCI_LINK_FREE != link->hl_state))
                {    
                    //os_printf("0000000000:%p,%d\r\n", link,link->hl_state);
                    hci_link_free(link,UWE_CONNRESET);
                }
                jtask_schedule(sys_hdl->app_auto_con_task,
                               200,
                               bt_create_reconnect_wrap,
                               (void *)retry_count);
            }
        }
        else // sleep issue
        {
			//走远回连计数超时退出		
 			// sys_hdl->flag_sm1 &= ~APP_FLAG_RECONNCTION;
            os_printf("END\r\n");
        #if 0
            link = hci_link_lookup_btaddr(sys_hdl->unit, address_ptr, HCI_LINK_ACL);
            if(link && (HCI_LINK_OPEN != link->hl_state))
            {
                unit_send_cmd(sys_hdl->unit,
                                HCI_CMD_CREATE_CON_CANCEL,
                                address_ptr,
                                BTADDR_LEN);
            }
        #endif
        #if (BT_MODE==BT_MODE_1V2)
            bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL,0);
            sys_hdl->reconn_num_flag &= ~0x01;
            if(sys_hdl->reconn_num_flag == 2)
            {
            	//继续回连第2部手机
                 os_printf("----copy addr-----\r\n");
                 memcpy((uint8 *)&sys_hdl->reconn_btaddr,(uint8 *)&sys_hdl->reconn_btaddr2,sizeof(btaddr_t));
	             sys_hdl->reconn_num_flag = 0;
                 jtask_schedule(sys_hdl->app_auto_con_task,
                                                    500,
                                                    bt_create_reconn_timeout_wrap,
                                                   (void *)0);
            }
            else
        #endif
            {
                bt_auto_connect_stop();
            }
        #if 0
            if(!hci_get_acl_link_count(sys_hdl->unit))
            {
                app_set_led_event_action(LED_EVENT_MATCH_STATE);
            }
            else
            {
                if(a2dp_has_music())
                    app_set_led_event_action(LED_EVENT_BT_PLAY);
                else
                    app_set_led_event_action(LED_EVENT_CONN);
            }
        #endif
        }
    }
}

void bt_create_conn_timeout_wrap(void *arg)
{
    int index;
    uint32 max_count;
    hci_link_t *link = NULL;
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();
    if((int32)arg != APP_RETRY_FOREVER)
        retry_count = (int32)arg;

    jtask_stop(sys_hdl->app_match_task);
    bt_flag1_operate(APP_FLAG_AVRCP_FLAG, 0);
    CLEAR_SLEEP_TICK;
#if (BT_MODE==BT_MODE_1V2)
    if(hfp_has_sco_conn())
    {
         bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL, 1);
         return;
    }
#ifndef  CONN_WITH_MUSIC
    if(a2dp_has_music())
    {
        bt_flag2_operate(APP_FLAG2_RECONN_AFTER_PLAY,1);
        return;
    }
#endif
#endif

    if(bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION))
    {
        //retry_count++;
        max_count = env_h->env_cfg.bt_para.auto_conn_count;
        index = sys_hdl->auto_conn_id-1;
        if(((retry_count < max_count)
             || (max_count == APP_RETRY_FOREVER))
             && (env_h->env_data.key_pair[index].used != 0))
        {
        #ifdef BT_ONE_TO_MULTIPLE
            uint32 acl_count = hci_get_acl_link_count(sys_hdl->unit);
            os_printf("acl link num:%d\r\n", acl_count);
            if(acl_count >= BT_MAX_AG_COUNT)
            {
                bt_auto_connect_stop();
                return;
            }
        #endif

            if(env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_ADDR_POLL)
            {
                if(app_get_env_key_num()!=0)
                {
                    index = sys_hdl->auto_conn_id-1;
                    if((env_h->env_data.key_pair[index].used == 1) || (env_h->env_data.key_pair[index].used == 2))
                    {
                        btaddr_t *address_ptr;
                        address_ptr = &env_h->env_data.key_pair[index].btaddr;
                    #if 0
                        link = hci_link_lookup_btaddr(sys_hdl->unit, address_ptr, HCI_LINK_ACL);
                        if(link && (HCI_LINK_OPEN != link->hl_state))
                        {
                            unit_send_cmd(sys_hdl->unit, HCI_CMD_CREATE_CON_CANCEL,address_ptr, BTADDR_LEN);
                            jtask_schedule(sys_hdl->app_auto_con_task,
                                                200,
                                                bt_create_autoconn_wrap,
                                                (void *)APP_RETRY_FOREVER);
                        }
                    #endif

                        memcpy((uint8 *)&sys_hdl->reconn_btaddr,(uint8 *)address_ptr,sizeof(btaddr_t));
                    #if CONFIG_RF_CALI_TYPE
                        XVR_analog_reg_save[13] &= 0xffffffc0;
                    #if (0==(CONFIG_RF_CALI_TYPE&CALI_BY_PHONE_BIT))
                        env_h->env_data.key_pair[index].crystal_cali_data = env_h->env_cfg.system_para.frq_offset;
                    #endif
                        XVR_analog_reg_save[13] |= env_h->env_data.key_pair[index].crystal_cali_data;
                        BK3000_XVR_REG_0x0D = XVR_analog_reg_save[13];
                    #else
                        XVR_analog_reg_save[13] &= 0xffffffc0;  
                        XVR_analog_reg_save[13] |= env_h->env_data.key_pair[index].crystal_cali_data;                       
                        BK3000_XVR_REG_0x0D = XVR_analog_reg_save[13];
                    #endif
                        os_printf("index0:%d,%d\r\n", index,env_h->env_data.key_pair[index].crystal_cali_data);
                    }
                }
                else
                {
                    os_printf("no_other_btaddress\r\n");
                    bt_auto_connect_stop();
                    return;
                }
            }
            else
            {
                os_printf("no_address_poll\r\n");
            }

            //if(!(link && (HCI_LINK_OPEN == link->hl_state)))
            if(!link)
            {
            	retry_count++;
                #if (CONFIG_APP_RECONNECT_MATCH == 0)
                bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
                #endif
                os_printf("auto conn:%d,link_connect: "BTADDR_FORMAT"\r\n", retry_count,BTADDR(&(sys_hdl->reconn_btaddr)));
                hf_clear_autoconn_diconned_flag((void*)(&(sys_hdl->reconn_btaddr)));
                bt_unit_acl_connect(sys_hdl->unit, &sys_hdl->reconn_btaddr);
            }

            if(link && (HCI_LINK_OPEN == link->hl_state))
            {
                os_printf("link_exist:%d:%d\r\n", sys_hdl->auto_conn_id, link->hl_state);
                bt_auto_connect_stop();
            }
            else
            {
                jtask_schedule(sys_hdl->app_auto_con_task,
                               100,
                               bt_create_autoconn_wrap,
                               (void *)retry_count);
            }
        }
        else  //auto connection timeout
        {
        //自动回连计数超时退出

        /* send cmd_connection_cancel(); HCI_CMD_CREATE_CON_CANCEL */
        #if 0
            if(app_get_env_key_num()!=0)
            {
                index = sys_hdl->auto_conn_id-1;
                if((env_h->env_data.key_pair[index].used == 1) || (env_h->env_data.key_pair[index].used == 2))
                {
                    btaddr_t *tmp_addr;

                    tmp_addr = &env_h->env_data.key_pair[index].btaddr;

                    link = hci_link_lookup_btaddr(sys_hdl->unit, tmp_addr, HCI_LINK_ACL);

                    if(link && (HCI_LINK_OPEN != link->hl_state))
                    {
                        unit_send_cmd(sys_hdl->unit,HCI_CMD_CREATE_CON_CANCEL, tmp_addr,BTADDR_LEN);
                    }
                }
            }
        #endif
//          bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
        
        #if (BT_MODE==BT_MODE_1V2)
            if(app_check_bt_mode(BT_MODE_1V2))
            {
                if(bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_CALL) )
                    bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL,0);

                if(bt_flag1_is_set( APP_FLAG_AUTO_CONNECTION))
                {
                    // The second auto-connection fails, so set the stop flag
                    if (sys_hdl->auto_conn_id == sys_hdl->auto_conn_id2)
                        sys_hdl->auto_conn_status = INITIATIVE_CONNECT_STOP;
                    bt_auto_connect_next();
                }
            }
            else
        #endif
            {
                os_printf("bt_auto_connect_over\r\n");
                jtask_schedule(sys_hdl->app_auto_con_task,
                        200,
                        bt_auto_con_timerout_wrap,
                        (void *)NULL);
            }
        }
    }
}

#if (BT_MODE==BT_MODE_1V2)
void bt_auto_connect_next(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t  env_h = app_env_get_handle();

    if((INITIATIVE_CONNECT_STOP == sys_hdl->auto_conn_status)
		|| (sys_hdl->auto_conn_id2 == 0)
		|| (hci_get_acl_link_count(sys_hdl->unit)>=2))
    {
        bt_auto_connect_stop();
        return;
    }

    // The second auto-connection is already linked, stop the auto-connection, if it's on-going,
    // leave it alone
    if (sys_hdl->auto_conn_id == sys_hdl->auto_conn_id2)
    {
        os_printf("bt_auto_connect_next:"BTADDR_FORMAT"\r\n", BTADDR(&(env_h->env_data.key_pair[sys_hdl->auto_conn_id - 1].btaddr)));
        if (hci_link_lookup_btaddr(sys_hdl->unit, &(env_h->env_data.key_pair[sys_hdl->auto_conn_id - 1].btaddr), HCI_LINK_ACL))
        {
            bt_auto_connect_stop();
            return;
        }
    }
    else
    {
        sys_hdl->auto_conn_id = sys_hdl->auto_conn_id2;
        os_printf("set auto_next:%d\r\n", sys_hdl->auto_conn_id);
    }

#if (BT_MODE==BT_MODE_1V2)
#ifndef  CONN_WITH_MUSIC
    if(a2dp_has_music())
    {
        bt_flag2_operate(APP_FLAG2_RECONN_AFTER_PLAY,1);
        return;
    }
#endif
    if(hfp_has_sco_conn())
    {
        bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL,1);
        return;
    }
#endif

    // delay more time to make sure: Behave consistently if two phone play music before power on,auto connect
    if (!jtask_is_pending(sys_hdl->app_auto_con_task))
    {
        os_printf("bt_auto_connect_next\r\n");
        jtask_schedule(sys_hdl->app_auto_con_task,
                               6000,
                               bt_create_conn_timeout_wrap,
                               (void *)0);
    }
}
#endif

#ifdef CONFIG_AUTO_CONN_AB
static uint8 conn_AB = 0;
void bt_create_autoconn_timeout_AB_wrap(void *arg)
{
    int index;
    uint32 max_count;
    hci_link_t *link = NULL;
    btaddr_t *address_ptr;
    //static uint8 conn_AB = 0; // 0: first in function; 1: 1 or less not connected; 2: A/B changed
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();
    if((int32)arg != APP_RETRY_FOREVER)
        retry_count = (int32)arg; 
    
    jtask_stop(sys_hdl->app_match_task);
    bt_flag1_operate(APP_FLAG_AVRCP_FLAG, 0);
    CLEAR_SLEEP_TICK;
#if (BT_MODE==BT_MODE_1V2)
    if(hfp_has_sco_conn())
    {
         bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL, 1);
         return;
    }
#ifndef  CONN_WITH_MUSIC
    if(a2dp_has_music())
    {
        bt_flag2_operate(APP_FLAG2_RECONN_AFTER_PLAY,1);
        return;
    }
#endif
#endif

    if(conn_AB == 0)
    {
        if(app_get_env_key_num() <= 1)
            conn_AB = 1; // don't need change A/B
        else
            conn_AB = 2;
    }
    else if(conn_AB == 2)
    {
         index = sys_hdl->auto_conn_id-1;
        if(sys_hdl->auto_conn_id == sys_hdl->auto_conn_id1)
        {
            address_ptr = &env_h->env_data.key_pair[index].btaddr;
            if(hci_check_acl_link(address_ptr) || (env_h->env_data.key_pair[index].used == 0))
            {
                conn_AB = 1;
            }
            sys_hdl->auto_conn_id = sys_hdl->auto_conn_id2;
        }
        else
        {
            address_ptr = &env_h->env_data.key_pair[index].btaddr;
            if(hci_check_acl_link(address_ptr)|| (env_h->env_data.key_pair[index].used == 0))
            {
                conn_AB = 1;
            }
            sys_hdl->auto_conn_id = sys_hdl->auto_conn_id1;
        }
    }

    if(bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION))
    {
        //retry_count++;
        max_count = env_h->env_cfg.bt_para.auto_conn_count;

        if((retry_count < max_count)
             || (max_count == APP_RETRY_FOREVER))
        {
        #ifdef BT_ONE_TO_MULTIPLE
            uint32 acl_count = hci_get_acl_link_count(sys_hdl->unit);
            os_printf("acl link num:%d\r\n", acl_count);
            if(acl_count >= app_get_env_key_num() )
            {
                bt_auto_connect_stop();
                conn_AB = 0;
                return;
            }
        #endif

            if(env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_ADDR_POLL)
            {
                if(app_get_env_key_num()!=0)
                {
                    index = sys_hdl->auto_conn_id-1;
                    if((env_h->env_data.key_pair[index].used == 1) || (env_h->env_data.key_pair[index].used == 2))
                    {
                        address_ptr = &env_h->env_data.key_pair[index].btaddr;
                        memcpy((uint8 *)&sys_hdl->reconn_btaddr,(uint8 *)address_ptr,sizeof(btaddr_t));
                    #if CONFIG_RF_CALI_TYPE
                        XVR_analog_reg_save[13] &= 0xffffffc0;
                    #if (0==(CONFIG_RF_CALI_TYPE&CALI_BY_PHONE_BIT))
                        env_h->env_data.key_pair[index].crystal_cali_data = env_h->env_cfg.system_para.frq_offset;
                    #endif
                        XVR_analog_reg_save[13] |= env_h->env_data.key_pair[index].crystal_cali_data;
                        BK3000_XVR_REG_0x0D = XVR_analog_reg_save[13];
                    #else
                        XVR_analog_reg_save[13] &= 0xffffffc0;  
                        XVR_analog_reg_save[13] |= env_h->env_data.key_pair[index].crystal_cali_data;                       
                        BK3000_XVR_REG_0x0D = XVR_analog_reg_save[13];
                    #endif

                        os_printf("index1:%d,%d\r\n", index,env_h->env_data.key_pair[index].crystal_cali_data);
                    }
                }
                else
                {
                    os_printf("no_other_btaddress\r\n");
                    bt_auto_connect_stop();
                    //if(!a2dp_has_music())
                    //    app_set_led_event_action(LED_EVENT_MATCH_STATE);
                    return;
                }
            }
            
            //if(!(link && (HCI_LINK_OPEN == link->hl_state)))
            if(!link)
            {
                os_printf("auto connect time:%d,link_connect: "BTADDR_FORMAT"\r\n", retry_count+1,BTADDR(&(sys_hdl->reconn_btaddr)));
                if(((conn_AB == 2)&&(sys_hdl->auto_conn_id == sys_hdl->auto_conn_id2)) || (conn_AB == 1))
                    retry_count++;
                bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
                hf_clear_autoconn_diconned_flag((void*)(&(sys_hdl->reconn_btaddr)));
                bt_unit_acl_connect(sys_hdl->unit, &sys_hdl->reconn_btaddr);
            }

            if(link && (HCI_LINK_OPEN == link->hl_state))
            {
                os_printf("link_exist:%d:%d\r\n", sys_hdl->auto_conn_id, link->hl_state);
                bt_auto_connect_stop();
            }
            else
            {
                jtask_schedule(sys_hdl->app_auto_con_task,
                                       100,
                                       bt_create_autoconn_wrap,
                                       (void *)retry_count);
            }
        }
        else  //auto connection timeout
        {
            if(bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_CALL) )
                bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL,0);

            if(bt_flag1_is_set( APP_FLAG_AUTO_CONNECTION))
            {
                bt_auto_connect_stop();
                conn_AB = 0;
            }
        }
    }
}
#endif

#ifdef CONFIG_TWS
void app_bt_check_inquiry_set(uint8 tone)
{
    app_handle_t app_h = app_get_sys_handler();

    if(app_env_check_inquiry_always())
    {
        app_set_led_event_action(LED_EVENT_MATCH_STATE);
        bt_unit_set_scan_enable(app_h->unit, HCI_PAGE_SCAN_ENABLE|HCI_INQUIRY_SCAN_ENABLE);
        //if (tone)
        //    app_wave_file_play_start(APP_WAVE_FILE_ID_ENTER_PARING);
    }
    else
    {
        app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);
        bt_unit_set_scan_enable(app_h->unit, HCI_PAGE_SCAN_ENABLE);
    }
}

void app_bt_flag2_set0(void* flag)
{
    if (bt_flag1_is_set(APP_FLAG_ACL_CONNECTION))
        return;

    set_tws_prim_sec(TWS_PRIM_SEC_UNDEFINED);
    app_bt_check_inquiry_set(1);
}
#endif

#if 1/*(CONFIG_APP_TOOLKIT_5 == 1)*/||(POWERKEY_5S_PARING == 1)
void power_on_auto_connect(void)
{
	app_env_handle_t  env_h = app_env_get_handle();
    uint8 wakup_pin = env_h->env_cfg.system_para.wakup_pin + GPIO0;

    if((gpio_input(wakup_pin)) 
		#ifdef CONFIG_TWS
		|| get_tws_flag(TWS_FLAG_NEED_LAUNCH_TWS_PAIRING)
		#endif
		)
	{
		bt_auto_connect_start();
	}
}
#endif
//end
void bt_auto_connect_start(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
#if (0==(CONFIG_RF_CALI_TYPE&CALI_BY_PHONE_BIT))
	app_set_crystal_calibration(0);
#endif
#if defined(CONFIG_TWS)&&(CONFIG_TWS_SUPPORT_OLD_OPERATE == 0)
    if (get_tws_flag(TWS_FLAG_NEED_LAUNCH_TWS_PAIRING))
    {
        app_wave_file_play_stop();
        unset_tws_flag(TWS_FLAG_NEED_LAUNCH_TWS_PAIRING);
        app_button_stereo();
        app_wave_file_play_start(APP_WAVE_FILE_ID_STEREO_MATCH);
        //tws_launch_diac_pairing();
        return;
    }
#endif
    if(
    #if (CONFIG_CHARGE_EN == 1)
    	(bt_flag2_is_set(APP_FLAG2_CHARGE_POWERDOWN) &&(app_env_check_Charge_Mode_PwrDown()||(flag_power_charge==1)))||
    #endif
	#ifndef CONFIG_BLUETOOTH_COEXIST
	    (!app_is_bt_mode())||
	#endif
        ((app_get_env_key_num()==0)
        #ifdef CONFIG_TWS
            &&(get_tws_env_stereo_role() !=TWS_PRIM_SEC_SECOND)
        #endif
            )
    )
    {
    #ifdef CONFIG_PRODUCT_TEST_INF
        os_printf("~~~~~~~connect to Tester\r\n");
        sys_hdl->flag_sm1 |= APP_FLAG_AUTO_CONNECTION;
        sys_hdl->auto_conn_status = INITIATIVE_CONNECT_START;
        app_env_handle_t env_h = app_env_get_handle();
        if(env_h->env_cfg.feature.feature_flag & APP_ENV_FEATURE_FLAG_AUTCONN_TESTER)
        {
            memcpy((char *)&sys_hdl->reconn_btaddr,(char *)&env_h->env_cfg.feature.tester_bt_addr, 6 );
            bt_unit_acl_connect(sys_hdl->unit, &sys_hdl->reconn_btaddr);
            os_printf("~~~~~~~connect to Tester: "BTADDR_FORMAT"\r\n", BTADDR(&(sys_hdl->reconn_btaddr)));
	      jtask_schedule(sys_hdl->app_auto_con_task,
	                        5000,
	                        bt_create_conn_timeout_wrap,
	                        (void *)0);
        }
        else
        {
            bt_auto_connect_stop();
        }
    #else
        bt_auto_connect_stop();
    #endif
    	return;
    }
			
	
	os_printf("bt_auto_connect_start,flag1:0x%x,flag2:0x%x\r\n",sys_hdl->flag_sm1,sys_hdl->flag_sm2);

#ifdef CONFIG_TWS
    if(bt_flag2_is_set(APP_FLAG2_STEREO_AUTOCONN))
    {
    	tws_stereo_autoconn_cnt_set(1);
        retry_count = 0;
        bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
        app_set_led_event_action(LED_EVENT_STEREO_RECON_MODE);
        jtask_schedule(sys_hdl->app_auto_con_task, 100, (jthread_func)
                                    app_bt_inquiry_active_conn,
                                    (void *)6000);
    }
    else 
#endif
    {
        if(bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION))
        {
            app_set_led_event_action( LED_EVENT_AUTO_CONNECTION);

            sys_hdl->auto_conn_status = INITIATIVE_CONNECT_START;
            sys_hdl->unit->hci_link_policy |= HCI_LINK_POLICY_ENABLE_ROLE_SWITCH;

            // Take a snap shot. The "used" values are dynamlc during the process
            sys_hdl->auto_conn_id1 = app_get_active_linkey(0);
            sys_hdl->auto_conn_id2 = app_get_active_linkey(1);

            // When there is only one paired device, app_get_active_linkey(0) returns 0
            // TWS always connect id2
            if (sys_hdl->auto_conn_id1)
                sys_hdl->auto_conn_id = sys_hdl->auto_conn_id1;
            else
                sys_hdl->auto_conn_id = sys_hdl->auto_conn_id2;


            os_printf("bt_auto_connect_start,id1:%d,id2:%d,flag1:0x%x\r\n",sys_hdl->auto_conn_id1,sys_hdl->auto_conn_id2,sys_hdl->flag_sm1);

        #ifndef CONFIG_BLUETOOTH_SSP
            bt_unit_acl_connect(sys_hdl->unit, &sys_hdl->reconn_btaddr);

            jtask_schedule(sys_hdl->app_auto_con_task,
                            100,
                            bt_create_conn_timeout_wrap,
                            (void *)0);
        #else
            #ifdef CONFIG_AUTO_CONN_AB
                jtask_schedule(sys_hdl->app_auto_con_task,
                            100,
                            bt_create_autoconn_timeout_AB_wrap,
                            (void *)0);
            #else
                jtask_schedule(sys_hdl->app_auto_con_task,
                            100,
                            bt_create_conn_timeout_wrap,
                            (void *)0);
            #endif
        #endif
        }
        else
        {
            //os_printf("bt_auto_connect_start1\r\n");
            if(bt_flag1_is_set(APP_FLAG_MATCH_ENABLE))
            {
                app_set_led_event_action( LED_EVENT_MATCH_STATE );
            	if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_TWS))
                {
                #ifdef CONFIG_BLUETOOTH_COEXIST
                    if(app_is_bt_mode())
                #endif
                        app_wave_file_play_start(APP_WAVE_FILE_ID_ENTER_PARING);
                }
            }
        	else if (!bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION))
            {
            #ifdef CONFIG_TWS
        		if (bt_flag2_is_set(APP_FLAG2_STEREO_CONNECTTING)) 
					app_set_led_event_action(LED_EVENT_STEREO_MATCH_MODE);	
				else
			#endif
                    app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);
            }
        }

        if(bt_flag1_is_set(APP_FLAG_DUT_MODE_ENABLE))
        {
            os_printf("Reset device success!\r\n");
            sys_hdl->flag_sm1 &= ~APP_FLAG_DUT_MODE_ENABLE;
        }

    }
}

#if 0
uint32 bt_get_auto_connecting_status(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();

    return (INITIATIVE_CONNECT_START == sys_hdl->auto_conn_status);
}
#endif
#if (BT_MODE==BT_MODE_1V2)&& !defined(CONN_WITH_MUSIC)
static void app_bt_1v2_match_timeout_timerfunc(void *arg)
{
	app_handle_t sys_hdl = app_get_sys_handler();
	
	bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
}

void app_bt_set_1v2_match_timeout(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
	
    if((hci_get_acl_link_count(sys_hdl->unit)<BT_MAX_AG_COUNT) && app_check_bt_mode(BT_MODE_1V2))
    {
        jtask_stop(sys_hdl->app_match_task);
        jtask_schedule(sys_hdl->app_match_task,
                    1000*180,
                    (jthread_func)app_bt_1v2_match_timeout_timerfunc,
                    (void *)NULL);
    }
}
#endif

void bt_auto_connect_stop(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
#if CONFIG_TWS_AUTOCONNECT	
	static uint8 stereo_autoconn=0;
#endif
    os_printf("bt_auto_connect_stop:0x%x,0x%x\r\n", sys_hdl->flag_sm1,sys_hdl->flag_sm2);

    jtask_stop(sys_hdl->app_auto_con_task);
    retry_count = 0;

    if(bt_flag1_is_set(APP_FLAG_RECONNCTION|APP_FLAG_AUTO_CONNECTION))
    {
    #ifdef CONFIG_TWS
 	    hci_get_acl_link_check(sys_hdl->unit,&sys_hdl->remote_btaddr);
        if(bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION|APP_FLAG_HFP_CONNECTION))
    #elif defined(BT_ONE_TO_MULTIPLE)
        if(hci_get_acl_link_count(sys_hdl->unit) >= BT_MAX_AG_COUNT)
    #endif
        {
            bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
            os_printf("---SCAN---111\r\n");
        }
        else // one or more devices have not be connected, then set scan enable.
        {
        #if CONFIG_TWS_AUTOCONNECT
            if(!bt_flag1_is_set(APP_FLAG_ACL_CONNECTION|APP_FLAG_A2DP_CONNECTION|APP_FLAG_LINEIN)
                && !bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE)
                && !bt_flag2_is_set(APP_FLAG2_CHARGE_POWERDOWN)
                && (stereo_autoconn==0))
            {
                stereo_autoconn = 1;
                //jtask_schedule(sys_hdl->tws_linein_task, 2000, (jthread_func)app_button_stereo_slave_action, NULL);

                /* In env data,there are not any linkkeys */
                if((app_get_env_key_num() == 0) && !app_get_env_stereo_key_used())
                {
                    app_button_stereo_slave_action();
                }
                else
                {
                    bt_unit_set_scan_enable(sys_hdl->unit,HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);                        
                }
                return;
            }
        #endif
            #ifdef NO_SCAN_WHEN_WORKING
             if(hfp_has_sco_conn() || a2dp_has_music())
             {
                bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
             }
             else
        #endif
            {
                if(app_env_check_inquiry_always() || (app_get_env_key_num()==0))
                {
                #ifndef CONFIG_TWS
                    if(app_bt_get_disconn_event()==1)
                    {
                        app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);
                        bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
                    }
                    else
                #endif
                    {
                        if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_1V2|BT_MODE_TWS)
                        #if ((CONFIG_CHARGE_EN == 1))
                            &&(!app_env_check_Charge_Mode_PwrDown()||!get_Charge_state())
                            && !bt_flag2_is_set(APP_FLAG2_CHARGE_POWERDOWN)
                        #endif
                            && !a2dp_has_connection())
                        {
                        #ifdef CONFIG_BLUETOOTH_COEXIST
                            if(1) 
                        #else
                            if(SYS_WM_BT_MODE == sys_hdl->sys_work_mode) 
                        #endif
                            {
                            #ifdef CONFIG_BLUETOOTH_COEXIST
                                if(app_is_bt_mode())
                            #endif
                                {
                                    if( !bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_DISCONNEC))
                                        app_wave_file_play_start(APP_WAVE_FILE_ID_ENTER_PARING);
                                }
                                app_set_led_event_action(LED_EVENT_MATCH_STATE);
                                bt_unit_set_scan_enable(sys_hdl->unit,HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
                                os_printf("---SCAN---222\r\n");
                            }
                            else
                            {
                                bt_unit_set_scan_enable(sys_hdl->unit,HCI_NO_SCAN_ENABLE);
                                os_printf("---SCAN---333\r\n");
                            }
                        }
                    }
                }
                else
                {
                #ifndef CONFIG_TWS
                    if(hci_get_acl_link_count(sys_hdl->unit))
                    {
                        if(a2dp_has_music())
                            app_set_led_event_action(LED_EVENT_BT_PLAY);
                    #if (BT_MODE==BT_MODE_1V2)
                        else if(has_hfp_flag_1toN(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
                            app_set_led_event_action(LED_EVENT_CALL_ESTABLISHMENT);
                    #endif
                        else
                            app_set_led_event_action(LED_EVENT_CONN);
                    }
                    else
                #endif
                    {
                        app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);
                    }
                    bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
                    os_printf("---SCAN---444\r\n");
                }
            }
        }
    }
    else
    {
    	if(!hci_get_acl_link_count(sys_hdl->unit))    // have no connection
       {
            if(app_env_check_inquiry_always() || (app_get_env_key_num()==0)) // unknow reason close scan, reset scan status
            {
                if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_1V2|BT_MODE_TWS)
                    &&(SYS_WM_BT_MODE == sys_hdl->sys_work_mode)
                #if ((CONFIG_CHARGE_EN == 1))
                    &&(!app_env_check_Charge_Mode_PwrDown()||!get_Charge_state())
                    && !bt_flag2_is_set(APP_FLAG2_CHARGE_POWERDOWN)
                #endif
                    && !bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_DISCONNEC)
                    && !a2dp_has_connection())
                {
                        bt_unit_set_scan_enable(sys_hdl->unit,HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
                        os_printf("---SCAN---555\r\n");
                }
            }
         
            if(!app_env_check_inquiry_always())
            {
                if(bt_flag1_is_set( APP_FLAG_MATCH_ENABLE ) )
                {
                    app_set_led_event_action( LED_EVENT_MATCH_STATE);
                }
                else
                {
                    app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);
                }
            }
            else
            {
                app_set_led_event_action(LED_EVENT_MATCH_STATE);
            }
        }
    	else                                        // have connection;
        {
        #ifndef CONFIG_TWS
            if(a2dp_has_music())
                app_set_led_event_action(LED_EVENT_BT_PLAY);
            else if(has_hfp_flag_1toN(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
            	app_set_led_event_action(LED_EVENT_CALL_ESTABLISHMENT);
            else
                app_set_led_event_action(LED_EVENT_CONN);
        #endif
        }
    }


    if(bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION))
    {
        bt_flag1_operate(APP_FLAG_AUTO_CONNECTION, 0);
    }
#ifdef CONFIG_TWS
    if(bt_flag1_is_set(APP_FLAG_PROFILE_AUTO_CONN))
    {
        bt_flag1_operate(APP_FLAG_PROFILE_AUTO_CONN, 0);    
    }
#endif
    if(bt_flag1_is_set(APP_FLAG_RECONNCTION))
    {
        bt_flag1_operate(APP_FLAG_RECONNCTION, 0);
    #if (BT_MODE==BT_MODE_1V2)
        // Make sure all reconnection flags are cleared
        bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL,0);
        bt_flag2_operate(APP_FLAG2_RECONN_AFTER_PLAY,0);
    #endif
        sys_hdl->reconn_num_flag = 0;
    }
    sys_hdl->auto_conn_status = INITIATIVE_CONNECT_STOP;
    app_bt_reset_policy_iocap();
#if (CONFIG_CHARGE_EN == 1) 
    app_charge_powerdown();
#endif

#ifdef CONFIG_TWS
    if(bt_flag2_is_set( APP_FLAG2_STEREO_BUTTON_PRESS ))
    {
        app_button_stereo();
    }
    else if( ! bt_flag2_is_set( APP_FLAG2_STEREO_WORK_MODE ) )
    {
        if(bt_flag2_is_set(APP_FLAG2_STEREO_AUTOCONN))
        {
            jtask_schedule( sys_hdl->app_stereo_task, 1000, (jthread_func)
                              app_bt_inquiry_active_conn,
                              (void *)TWS_PAGE_TIME_INTERVAL);
        }
    }
    if ((get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)
        && bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE)
        && !bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION))
    {
    	app_bt_check_inquiry_set(0);
    }
#endif
    if( bt_flag1_is_set( APP_FLAG_LINEIN ))
    {
        app_wave_file_play_stop();
        linein_audio_open();
    #ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
        if (bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE)||bt_flag2_is_set(APP_FLAG2_STEREO_AUTOCONN))
            bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
        else
            bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
    #else
    #ifndef CONFIG_BLUETOOTH_COEXIST
        bt_unit_set_scan_enable(sys_hdl->unit,HCI_NO_SCAN_ENABLE);
    #endif
    #endif
    }
	

}
#ifndef OTT_STRETIGG_LINK_COEXIST
// this function is called when 1toN and OTT_STR....
void otm_current_a2dp_stream_off_handler(void *arg)
{
    app_handle_t sys_hdl = app_get_sys_handler();

    bt_auto_connect_stop();
    if((uint32)arg == (uint32)a2dp_get_current_app())
    {
        a2dp_clear_current_app();
        sys_hdl->auto_conn_id = 0;
        bt_auto_connect_start();
    }
}
#endif

#ifndef CONFIG_TWS
int bt_is_reject_new_connect(void)
{
    int ret = 0;

    app_handle_t sys_hdl = app_get_sys_handler();
#ifdef BT_ONE_TO_MULTIPLE
    int acl_count = hci_get_acl_link_count(sys_hdl->unit);
#endif
    os_printf("bt reject:0x%x,0x%x\r\n",sys_hdl->flag_sm1,sys_hdl->flag_sm2);

#if defined(CONFIG_LINE_SD_SNIFF)
    if((SYS_WM_LINEIN_MODE == sys_hdl->sys_work_mode)
        ||(SYS_WM_SD_MUSIC_MODE == sys_hdl->sys_work_mode)
        ||(SYS_WM_UDISK_MUSIC_MODE == sys_hdl->sys_work_mode))
    {
        ret = 1;
        return ret;
    }
#endif

#ifdef BT_ONE_TO_MULTIPLE
    if(hfp_has_sco_conn() // bt_flag2_is_set(APP_FLAG2_CALL_PROCESS)
#else
    if (
    #ifdef CONFIG_TWS
        0
    #else
        bt_flag2_is_set(APP_FLAG2_CALL_PROCESS)
    #endif
#endif
#ifndef  CONN_WITH_MUSIC
        || (a2dp_has_music())
#endif
#ifdef BT_ONE_TO_MULTIPLE
     	|| (acl_count >= BT_MAX_AG_COUNT)
#endif
    )
    {
#if 1 //def BT_ONE_TO_MULTIPLE
        os_printf("reject flag2:0x%x,music:%d,acl cnt:%d\r\n", sys_hdl->flag_sm2, a2dp_has_music(), acl_count);
#endif
        ret = 1;
    }

    if (bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION)
	#if (CONFIG_APP_RECONNECT_MATCH == 0)	
		|| bt_flag1_is_set(APP_FLAG_RECONNCTION)
	#endif	
		)
    {
        //os_printf("reject flag:0x%x\r\n",sys_hdl->flag_sm1);

        ret = 1;
    }

    return ret;
}
#endif

void hci_err_page_timeout_callback(uint8 status)
{			
#if (CONFIG_APP_RECONNECT_MATCH == 1)
	app_handle_t sys_hdl = app_get_sys_handler();

	if (status == HCI_ERR_PAGE_TIMEOUT)
	{
		if(bt_flag1_is_set(APP_FLAG_RECONNCTION))
			bt_unit_set_scan_enable(sys_hdl->unit, HCI_INQUIRY_SCAN_ENABLE|HCI_PAGE_SCAN_ENABLE); 			
	}
#endif	
}


void bt_select_current_hfp(void *arg)
{
    hfp_select2_current_app(arg);
}

void bt_select_current_a2dp_avrcp(void *arg)
{
    btaddr_t raddr;
    void *current_ptr = 0;

    current_ptr = a2dp_select1_current_app(arg);

    os_printf("current_ptr:%x\r\n", current_ptr);
    if((a2dp_is_connection(current_ptr))
        && (UWE_OK == a2dp_get_remote_addr(current_ptr, &raddr)))
    {
        if(avrcp_update_current_app(&raddr))
        {
            //os_printf("Failed\r\n");
            app_bt_active_conn_profile(PROFILE_ID_AVRCP, &raddr);
        }
    }
}

void hfpcall_select_a2dp(void * arg)
{
    btaddr_t hfp_raddr;

    hfp_get_remote_addr(arg, &hfp_raddr);
    os_printf("---select a2dp:"BTADDR_FORMAT"---\r\n",BTADDR(&hfp_raddr));
    select_a2dp_avrcp(&hfp_raddr);
}

void get_cur_hfp_raddr(btaddr_t **raddr)
{
    *raddr=hf_get_curhfp_raddr();
}

#if 0
void bt_select_other_a2dp_avrcp(uint8 addr_same)
{
    app_handle_t sys_hdl = app_get_sys_handler();
	uint32 tmp;
	jtask_stop(sys_hdl->app_a2dp_task);
	if(UWE_OK == get_play_a2dp_avrcp_state(&tmp))
	{
		if(!bt_flag2_is_set(APP_FLAG2_CALL_PROCESS) || addr_same)
			jtask_schedule(sys_hdl->app_a2dp_task,1000,select_play_a2dp_avrcp,0);
	}
}
void bt_enhanced_select_avrcp(void *arg)
{
    btaddr_t a2dp_raddr;
    btaddr_t avrcp_raddr;
    void *current_ptr = 0;

    current_ptr = a2dp_get_current_app();
    if((current_ptr)
         && UWE_OK == a2dp_get_remote_addr(current_ptr, &a2dp_raddr)
         && (UWE_OK == avrcp_get_raddr(arg, &avrcp_raddr)))
    {
        if(btaddr_same(&a2dp_raddr, &avrcp_raddr))
        {
            avrcp_select_current_app(arg);
        }
    }
}
#endif
#ifdef CONFIG_TWS
CONST btaddr_t g_ff_addr={{0xff,0xff,0xff,0xff,0xff,0xff}};
CONST btaddr_t g_00_addr={{0,0,0,0,0,0}};
uint8 app_bt_stereo_addr_env_empty(void)					
{
    app_env_handle_t env_h = app_env_get_handle();

    if (btaddr_same(&env_h->env_data.stereo_key.btaddr,&g_ff_addr)
        || btaddr_same(&env_h->env_data.stereo_key.btaddr,&g_00_addr))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
#endif					
void app_bt_enable_complete_wrap( hci_unit_t *unit)
{
    uint32 i;
    uint8 cmd[136];
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();
#ifdef CONFIG_TWS
    jtask_stop( sys_hdl->app_reset_task );
#endif
#ifdef CONFIG_AUTO_CONN_AB
    conn_AB = 0;
#endif
    CLEAR_PWDOWN_TICK;
    CLEAR_SLEEP_TICK;
    if(!(bt_flag1_is_set(APP_FLAG_DUT_MODE_ENABLE)))
    {
        sys_hdl->flag_sm1 |= APP_BUTTON_FLAG_BT_ENABLE;
        sys_hdl->unit = unit;
        
    #ifdef CONFIG_TWS
        memcpy((uint8 *)&sys_hdl->stereo_btaddr,(uint8 *)&env_h->env_data.stereo_key.btaddr,BYTE_BD_ADDR_SIZE);
	#else
	    btaddr_copy(&sys_hdl->remote_btaddr, &env_h->env_data.default_btaddr);
    #endif
    }

#ifdef CONFIG_TWS
    os_printf("TWS:%d, remote_btaddr1: "BTADDR_FORMAT",stereo_btaddr: "BTADDR_FORMAT"\r\n", 
                      get_tws_env_stereo_role(),BTADDR(&(sys_hdl->remote_btaddr)),BTADDR(&(sys_hdl->stereo_btaddr)));
    //os_printf("stereo_btaddr: "BTADDR_FORMAT"\r\n",  BTADDR(&(sys_hdl->stereo_btaddr)));
#endif
    for(i = 0; i < MAX_KEY_STORE; i++)
    {
        if((env_h->env_data.key_pair[i].used == 0x01) || (env_h->env_data.key_pair[i].used == 0x02))
        {
            memcpy(&cmd[i*22 + 1], (uint8 *)&env_h->env_data.key_pair[i].btaddr, 6);
            memcpy(&cmd[i*22 + 7], (uint8 *)env_h->env_data.key_pair[i].linkkey, 16);
        }
        else
        {
            break;
        }
    }
#ifdef CONFIG_TWS
	bt_flag2_operate(APP_FLAG2_STEREO_AUTOCONN, 0);
#if 0
    //There is connection record.
    //if(get_tws_env_stereo_role() == TWS_PRIM_SEC_SECOND)// reconn phone 
    if(app_env_get_stereo_role_stored() == TWS_PRIM_SEC_SECOND)	//// reconn tws
    {        
        bt_flag2_operate(APP_FLAG2_STEREO_AUTOCONN, 1);
        bt_flag1_operate(APP_FLAG_AUTO_CONNECTION, 0);
    }   
    os_printf(">[TWS env role:%d\r\n",get_tws_env_stereo_role());
    if(get_tws_env_stereo_role() == TWS_PRIM_SEC_PRIMARY)      //Last as TWS-MASTER.
    {
        set_tws_prim_sec(TWS_PRIM_SEC_PRIMARY);        
    }
    else if(get_tws_env_stereo_role() == TWS_PRIM_SEC_SECOND) //Last as TWS-SLAVE.
    {
        set_tws_prim_sec(TWS_PRIM_SEC_SECOND);          
    }
    else
    {
        set_tws_prim_sec(TWS_PRIM_SEC_UNDEFINED);     
    }
 #else
    if(get_tws_prim_sec() == TWS_PRIM_SEC_SECOND)
    {        
        bt_flag2_operate(APP_FLAG2_STEREO_AUTOCONN, 1);
        bt_flag1_operate(APP_FLAG_AUTO_CONNECTION, 0);
    } 
    os_printf(">[TWS env role:%d\r\n",get_tws_prim_sec());
 #endif
    if(env_h->env_data.stereo_key.used == 0x01)
    {
    	memcpy(&cmd[i*22 + 1], (uint8 *)&env_h->env_data.stereo_key.btaddr, BYTE_BD_ADDR_SIZE);
    	memcpy(&cmd[i*22 + 7], (uint8 *)env_h->env_data.stereo_key.linkkey, LINK_KEY_SIZE);
    } 
    
#endif

    if(i > MAX_KEY_STORE)
    {
        i = MAX_KEY_STORE;
    }

    if(i > 0)
    {
        cmd[0] = i;
        hci_send_cmd(sys_hdl->unit,HCI_CMD_WRITE_STORED_LINK_KEY,cmd,i*22 + 1);
    }
    else
    {
        unit->app_cbs.write_linkkey_complete_cb(unit->app_ctx, 0);
    }

#ifndef CONFIG_BLUETOOTH_SSP
    if(bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION))
    {
        bt_auto_connect_start();
    }
#endif
#ifdef CONFIG_TWS
    if ((app_get_env_key_num() == 0)&&flag_once_power_on)
    {
        bt_unit_set_scan_enable(sys_hdl->unit, HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
    }
#endif
}

static void app_bt_match_timeout_timerfunc(void *arg)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();
#ifndef CONFIG_BLUETOOTH_COEXIST
    if(SYS_WM_BT_MODE !=sys_hdl->sys_work_mode)
    {
        jtask_stop(sys_hdl->app_match_task);
        return ;
    }
#endif    
    app_sleep_func(0);
    CLEAR_SLEEP_TICK;

    if((env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_AUTO_CONN_PERIOD)
        && (0 == app_bt_get_disconn_event()))
    {
        if(! (bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION)))
        {
            bt_flag1_operate(APP_FLAG_AUTO_CONNECTION, 1);
            bt_auto_connect_start();
        }
    }
    else
    {   
        bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
        app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);
    }
}

void app_bt_set_match_timeout(void)
{
    app_env_handle_t env_h = app_env_get_handle();
    app_handle_t sys_hdl = app_get_sys_handler();

    if(env_h->env_cfg.bt_para.match_timeout == -1)
        return;

    jtask_stop(sys_hdl->app_match_task);

    jtask_schedule(sys_hdl->app_match_task,
                    env_h->env_cfg.bt_para.match_timeout*1000,
                    (jthread_func)app_bt_match_timeout_timerfunc,
                    (void *)NULL);
}
extern void app_powerdown_action( void );
void app_bt_disable_complete_wrap(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    os_printf("app_bt_disable_complete_wrap()\r\n");

    jtask_stop(sys_hdl->app_match_task);

    bt_flag1_operate(APP_BUTTON_FLAG_BT_ENABLE, 0);

    if(bt_flag1_is_set(APP_FLAG_DUT_MODE_ENABLE))
    {
        CONST static char bluecmdDut0[] = { 0x01, 0x1A, 0x0c, 0x01, 0x03};
        CONST static char bluecmdDut1[] = { 0x01, 0x05, 0x0c, 0x03, 0x02, 0x00, 0x02};
        CONST static char bluecmdDut2[] = { 0x01, 0x03, 0x18, 0x00 };
        //BK3000_GPIO_18_CONFIG = 0x40;   // radio on status;
        //BK3000_GPIO_GPIODCON |= (1<<18);
        uart_send_poll((uint8 *)bluecmdDut0, sizeof(bluecmdDut0));
        uart_send_poll((uint8 *)bluecmdDut1, sizeof(bluecmdDut1));
        uart_send_poll((uint8 *)bluecmdDut2, sizeof(bluecmdDut2));
        os_printf("Enter Dut test mode success!\r\n");
        app_set_led_event_action(LED_EVENT_TEST_MODE);
    }
    else if(bt_flag1_is_set(APP_FLAG_POWERDOWN))
    {
    	aud_PAmute_oper(1);
        app_set_led_event_action(LED_EVENT_POWER_OFF);
        //app_led_action(1);

        if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_TWS))
        {
            app_bt_shedule_task((jthread_func)app_powerdown_action, (void *)sys_hdl->unit, 500);
        }
        else
        {
            app_bt_shedule_task((jthread_func)app_powerdown_action, (void *)sys_hdl->unit, 1000);
        }
    }

}

#ifdef CONFIG_BLUETOOTH_HID
extern void Set_photoKey(void);
#endif
void app_bt_conn_compl_wrap(hci_link_t * link, const btaddr_t * rbtaddr)
{
    app_handle_t sys_hdl = app_get_sys_handler();

#if(CONFIG_CUSTOMER_SET_SUPERVISION_TIME == 1)
    hci_write_link_supervision_timeout_cp cp;
#endif

#ifndef CONFIG_BLUETOOTH_SSP
    uint32  a2dp_conn_delay;
    app_env_handle_t env_h = app_env_get_handle();
#endif

    bt_flag2_operate(APP_FLAG2_RECONN_AFTER_DISCONNEC, 0);
    
#if defined(CONFIG_BLUETOOTH_HID)
    if(app_env_check_HID_profile_enable())
        Set_photoKey();
#endif

    CLEAR_PWDOWN_TICK;
    CLEAR_SLEEP_TICK;
#ifndef CONFIG_BLUETOOTH_COEXIST
    if(SYS_WM_BT_MODE !=sys_hdl->sys_work_mode)
    {
        bt_auto_connect_stop();
        return ;
    }
#endif
/*
#ifdef CONN_WITH_MUSIC
    bt_flag2_operate(APP_FLAG2_SW_MUTE,1);
#endif
*/
#ifdef CONFIG_TWS
    app_bt_stereo_pairing_exit();
    if(!bt_flag1_is_set(APP_FLAG_POWERDOWN))
        jtask_stop(sys_hdl->app_tws_task);
#endif
	
#if(CONFIG_CUSTOMER_SET_SUPERVISION_TIME == 1)
    //memcpy( (uint8 *)&sys_hdl->stereo_btaddr, (uint8 *)rbtaddr, sizeof(btaddr_t) );

    cp.con_handle = link->hl_handle;
    cp.timeout = 8000; // 5s
    unit_send_cmd(sys_hdl->unit, HCI_CMD_WRITE_LINK_SUPERVISION_TIMEOUT, (void *)&cp, sizeof(cp));
#endif
#ifdef CONFIG_TWS
    set_tws_prim_sec(TWS_PRIM_SEC_PRIMARY);
	//set_tws_env_stereo_role(TWS_PRIM_SEC_PRIMARY);
#endif
    sys_hdl->flag_sm1 |= APP_FLAG_ACL_CONNECTION;
    memcpy((uint8 *)&sys_hdl->remote_btaddr,(uint8 *)rbtaddr,sizeof(btaddr_t));
    os_printf("app_bt_conn_compl_wrap: "BTADDR_FORMAT"\r\n",BTADDR(&sys_hdl->remote_btaddr));
    sys_hdl->link_handle = link->hl_handle;

    if(bt_flag1_is_set(APP_FLAG_RECONNCTION) && btaddr_same(rbtaddr,&sys_hdl->reconn_btaddr))
        jtask_stop(sys_hdl->app_auto_con_task);

    jtask_stop(sys_hdl->app_match_task);
#ifdef CONFIG_TWS
    jtask_stop(sys_hdl->app_stereo_task);
#endif 
#if defined(BT_ONE_TO_MULTIPLE)&&defined(CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE)
    uint8_t idx=0;
    idx = bt_sniff_find_st_available();
    if(idx < NEED_SNIFF_DEVICE_COUNT)
        bt_sniff_alloc_st(idx, sys_hdl->link_handle, &sys_hdl->remote_btaddr);
#endif
#if defined(CONFIG_TWS)&&defined(CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE)
    //uint8_t idx=0;
    //idx = bt_sniff_find_st_available();
    //if(idx < BT_MAX_AG_COUNT)
    bt_sniff_alloc_st(0, sys_hdl->link_handle, &sys_hdl->remote_btaddr);
#endif
#ifndef CONFIG_BLUETOOTH_SSP
    if((bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION)) ||
	 ((bt_flag1_is_set(APP_FLAG_RECONNCTION)) &&
	 (env_h->env_cfg.bt_para.disconn_action == ENV_ACTION_DISCONNECT_CONN)))
    {
        app_bt_active_allpro_conn((void *)rbtaddr);
    #if 0
	    sys_hdl->flag_sm1 &= ~APP_FLAG_RECONNCTION;
        a2dp_conn_delay = 0;

        jtask_schedule(sys_hdl->app_auto_con_task,
                            a2dp_conn_delay,
                            app_bt_active_allpro_conn,
                            (void *)rbtaddr);
    #endif
    }
    else if((bt_flag1_is_set(APP_FLAG_RECONNCTION)) &&
	 (env_h->env_cfg.bt_para.disconn_action == ENV_ACTION_DISCONNECT_PAGE)) /*only page*/
    {
        if((link->hl_flags & (HCI_LINK_AUTH | HCI_LINK_ENCRYPT))
            == (HCI_LINK_AUTH | HCI_LINK_ENCRYPT))
        {
            app_bt_active_allpro_conn((void *)rbtaddr);
        }
        else
        {
            link->hl_flags |= HCI_LINK_AUTH_REQ | HCI_LINK_ENCRYPT_REQ;
            hci_acl_setmode(link);
        }
    }
#else
    if((sys_hdl->flag_sm1 & (APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
	#if 0//def CONFIG_BLUETOOTH_HID
		&& (!app_env_check_HID_profile_enable()) // SSP start earlier, Sony Dongle won't reconnect  (only support HID mode)
	#endif
		)
    {
        if(!(link->hl_flags & (HCI_LINK_AUTH | HCI_LINK_ENCRYPT)))
        {
            link->hl_flags |= HCI_LINK_AUTH_REQ | HCI_LINK_ENCRYPT_REQ;
            hci_acl_setmode(link);
        }
        else
        {
            app_bt_allpro_conn_start(AUTO_RE_CONNECT_SCHED_DELAY_HFP,&sys_hdl->remote_btaddr);
        }
    }
#endif
}

void app_bt_active_allpro_conn(void *arg)
{
    app_env_handle_t env_h = app_env_get_handle();
#ifdef CONFIG_BLUETOOTH_HID
    app_handle_t sys_hdl = app_get_sys_handler();
    extern uint8_t hid_mode ;
#endif

    if(((env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_HFP)||(env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_APP_BAT_DISPLAY))
        && !(hfp_is_connection_based_on_raddr((btaddr_t *)arg))
        && !(hfp_check_autoconn_disconned_flag((btaddr_t *)arg)))
    {
        os_printf("----active_hfp\r\n");
        app_bt_active_conn_profile(PROFILE_ID_HFP, arg);
    }
    else if(env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_A2DP)
    {
        hf_clear_autoconn_diconned_flag(arg);

        if((bt_flag1_is_set(APP_FLAG_AVRCP_FLAG))
            && (!avrcp_is_connected_based_on_raddr((btaddr_t *)arg)))
        {
            bt_flag1_operate(APP_FLAG_AVRCP_FLAG, 0);
            os_printf("-----active_avrcp\r\n");
            app_bt_active_conn_profile(PROFILE_ID_AVRCP, arg);
        }
        else if(!a2dp_is_connected_based_on_raddr((btaddr_t *)arg))
        {
            os_printf("------active_a2dp\r\n");
            app_bt_active_conn_profile(PROFILE_ID_A2DP, arg);
        }
	#if defined(CONFIG_BLUETOOTH_HID)
		else if((app_env_check_HID_profile_enable())&&(!(sys_hdl->flag_sm1 & APP_FLAG_HID_CONNECTION))&&(hid_mode==0))
		{
			os_printf("------active PROFILE_ID_HID\r\n");
			app_bt_active_conn_profile(PROFILE_ID_HID, arg);
		}
	#endif
        else
        {
            os_printf("----active_null\r\n");
            if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_TWS))
            {
                app_bt_auto_conn_ok();
            }
            else if(bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
            {
            #ifdef CONFIG_BLUETOOTH_COEXIST
                if(app_is_bt_mode())
            #endif
                app_wave_file_play_start(APP_WAVE_FILE_ID_CONN);
            }
        #if (BT_MODE==BT_MODE_1V2)
            if(app_check_bt_mode(BT_MODE_1V2))
            {
                if(bt_flag1_is_set( APP_FLAG_AUTO_CONNECTION))
                {
                #ifdef CONFIG_AUTO_CONN_AB
                    bt_create_autoconn_timeout_AB_wrap((void*)retry_count);
                #else
                    bt_auto_connect_next();
                #endif
                }

                if(bt_flag1_is_set( APP_FLAG_RECONNCTION))
                {
                    app_handle_t app_h = app_get_sys_handler();
                    // 连上的地址不是回连的地址还是要继续回连
                     //两手机都走远回连，但 刚开始回连第一个时第二个主动连接上
                    if(btaddr_same(&app_h->reconn_btaddr,(btaddr_t *)arg))
                    {
                        app_h->reconn_num_flag &= ~0x01;
                        if(app_h->reconn_num_flag == 2)
                       {
                            //os_printf("----copy addr-----\r\n");
                            memcpy((uint8 *)&app_h->reconn_btaddr,
                                (uint8 *)&app_h->reconn_btaddr2,
                                sizeof(btaddr_t));
                            app_h->reconn_num_flag = 0;
                            jtask_schedule(app_h->app_auto_con_task,
                                500,
                                bt_create_reconn_timeout_wrap,
                                (void *)0);
                        }
                        else
                        {
                            bt_auto_connect_stop();
                        }
                    }
                    else
                    {// 一个已连上，一个已在回连(有没有可能没在回连?)
                        app_h->reconn_num_flag = 0;
                    }
                }
            }
            else
        #endif
            {
                bt_auto_connect_stop();
            }
        }
    }
    else  // won't be here if default config
    {
		os_printf("----not active----\r\n");
	#if (BT_MODE==BT_MODE_1V2)
		if(app_check_bt_mode(BT_MODE_1V2))
        {
            if(bt_flag1_is_set( APP_FLAG_AUTO_CONNECTION))
            {
            #ifdef CONFIG_AUTO_CONN_AB
                bt_create_autoconn_timeout_AB_wrap((void*)retry_count);
            #else
                bt_auto_connect_next();
            #endif
            }
        }
        else
    #endif
        {
            app_bt_auto_conn_ok();
        }
    }

    return;
}

void app_bt_shedule_task(jthread_func func, void *arg, uint32 delay_ms)
{

    app_handle_t sys_hdl = app_get_sys_handler();

    jtask_stop(sys_hdl->app_bt_common_task);

     if(func == NULL)
        return;

     CLEAR_PWDOWN_TICK;
     CLEAR_SLEEP_TICK;
     jtask_schedule(sys_hdl->app_bt_common_task, delay_ms, func, arg);
}

void app_bt_allpro_conn_start(int delay,btaddr_t* rtaddr)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();
    CLEAR_SLEEP_TICK;
	os_printf("app_bt_allpro_conn_start:%d,0x%x,0x%x\r\n", sys_hdl->auto_conn_status,sys_hdl->flag_sm1,sys_hdl->flag_sm2);

    if(INITIATIVE_CONNECT_STOP == sys_hdl->auto_conn_status)
    {
        return;
    }

    if((bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION))
        || ((bt_flag1_is_set(APP_FLAG_RECONNCTION))
        && (btaddr_same(rtaddr,&sys_hdl->reconn_btaddr)) //
        && (env_h->env_cfg.bt_para.disconn_action == ENV_ACTION_DISCONNECT_CONN))
    #if defined(CONFIG_BLUETOOTH_HID)
        ||(app_env_check_HID_profile_enable()&& (sys_hdl->auto_conn_status == INITIATIVE_CONNECT_HID))
    #endif
      )
    {
        os_printf("allpro_start1:%d,addr:"BTADDR_FORMAT"\r\n", delay,BTADDR(rtaddr));
        memcpy((uint8 *)&sys_hdl->remote_btaddr,(uint8 *)rtaddr,sizeof(btaddr_t));
        jtask_stop(sys_hdl->app_auto_con_task);
        jtask_schedule(sys_hdl->app_auto_con_task,
                            delay,
                            app_bt_active_allpro_conn,
                            (void *)&sys_hdl->remote_btaddr);
    }
    else if((bt_flag1_is_set(APP_FLAG_RECONNCTION))
        && (btaddr_same(rtaddr,&sys_hdl->reconn_btaddr))
        && (env_h->env_cfg.bt_para.disconn_action == ENV_ACTION_DISCONNECT_PAGE))
    {
        os_printf("allpro_start2:%d\r\n", delay);
	    sys_hdl->flag_sm1 &= ~APP_FLAG_RECONNCTION;
    }
    else
    {
        os_printf("allpro_start3:%d\r\n", delay);
    }
}

int app_bt_get_disconn_event(void)
{
    app_env_handle_t env_h = app_env_get_handle();

    return env_h->env_cfg.bt_para.action_disconn;
}

#if (BT_MODE==BT_MODE_1V2)
void app_bt_reconn_after_callEnd(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t  env_h = app_env_get_handle();

    //os_printf("----app_bt_reconn_after_callEnd:0x%x_0x%x\r\n",bt_flag1_is_set(APP_FLAG_RECONNCTION|APP_FLAG_AUTO_CONNECTION),bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_CALL|APP_FLAG2_RECONN_AFTER_PLAY));

    if (bt_flag1_is_set(APP_FLAG_RECONNCTION))
    {
        if(bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_CALL) )
        {
            jtask_schedule(sys_hdl->app_auto_con_task,
                            env_h->env_cfg.bt_para.disconn_start,
                            bt_create_reconn_timeout_wrap,
                            (void *)0);
            bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL,0);
        }
    }
    if (bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION))
    {
        if(bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_CALL) )
        {
        #ifndef CONFIG_AUTO_CONN_AB
            jtask_schedule(sys_hdl->app_auto_con_task,
                                    env_h->env_cfg.bt_para.disconn_start,
                                    bt_create_conn_timeout_wrap,
                                    (void *)0);
        #else
            jtask_schedule(sys_hdl->app_auto_con_task,
                                    env_h->env_cfg.bt_para.disconn_start,
                                    bt_create_autoconn_timeout_AB_wrap,
                                    (void *)0);
        #endif
            bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL,0);
        }
    }
}

#ifndef  CONN_WITH_MUSIC
void app_bt_reconn_after_play(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t  env_h = app_env_get_handle();

    os_printf("----app_bt_reconn_after_play:0x%x_0x%x\r\n",bt_flag1_is_set(APP_FLAG_RECONNCTION|APP_FLAG_AUTO_CONNECTION),bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_CALL|APP_FLAG2_RECONN_AFTER_PLAY));

    if (bt_flag1_is_set(APP_FLAG_RECONNCTION))
    {
        if(bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_PLAY))
        {
            jtask_schedule(sys_hdl->app_auto_con_task,
                                    env_h->env_cfg.bt_para.disconn_start,
                                    bt_create_reconn_timeout_wrap,
                                    (void *)0);
            bt_flag2_operate(APP_FLAG2_RECONN_AFTER_PLAY,0);
        }
    }

    if (bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION))
    {
        if(bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_PLAY))
        {
        #ifndef CONFIG_AUTO_CONN_AB
	    	jtask_schedule(sys_hdl->app_auto_con_task,
                            env_h->env_cfg.bt_para.disconn_start,
                            bt_create_conn_timeout_wrap,
                            (void *)0);
        #else
            jtask_schedule(sys_hdl->app_auto_con_task,
                            env_h->env_cfg.bt_para.disconn_start,
                            bt_create_autoconn_timeout_AB_wrap,
                            (void *)0);
        #endif
            bt_flag2_operate(APP_FLAG2_RECONN_AFTER_PLAY,0);
        }
    }

}
#endif
#endif
extern void clr_a2dp_flags(btaddr_t *btaddr);
extern void clr_hf_flags(btaddr_t *btaddr);
extern void clr_avrcp_flags(btaddr_t *btaddr);

void app_bt_disconn_wrap(uint8 reason, btaddr_t *raddr)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t  env_h = app_env_get_handle();
    int disconn_event;
    uint8_t  flag_scan = 0;  //used to avoid scan enable repeated too much
    btaddr_t *tmp;
#ifdef CONFIG_TWS
    app_tws_t app_tws_h = app_get_tws_handler();
#endif
#if (BT_MODE==BT_MODE_1V2)||defined(CONFIG_TWS)
    int ret;
    MSG_T msg;
#endif

#ifdef BT_ONE_TO_MULTIPLE
    #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    uint8_t  idx=0;
    idx = bt_sniff_get_index_from_raddr(raddr);
    bt_sniff_free_st(idx);
    #endif
#endif
#if defined(CONFIG_TWS)&&defined(CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE)
    bt_sniff_free_st(0);
#endif
    env_h->env_data.disconnect_reason = reason;
    // keep the addr printf for confirm problem(happan very rarely): A connect BK3262,fail(reason:0x8), to reconnect B , then A connected
    os_printf("bt_disconn_reason:0x%x, f_sm1:0x%x,f_sm2:0x%x, raddr:"BTADDR_FORMAT"\r\n", reason,sys_hdl->flag_sm1,sys_hdl->flag_sm2,BTADDR(raddr));

#ifdef CONFIG_BLUETOOTH_COEXIST
    app_coexist_play_pause(1);
#endif

#ifdef BT_ONLY_ONE_BT
    if(!btaddr_same(raddr,a2dp_get_current_app_remote_addr())&&(a2dp_has_connection()))
    {//按键Paring后连接新手机，旧手机偶尔会断开慢,会重新进入disconn wrap
        return ;
    }
#endif
    if(((SYS_WM_BT_MODE == sys_hdl->sys_work_mode)&& (!bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_DISCONNEC)))
        &&app_check_bt_mode(BT_MODE_1V1|BT_MODE_TWS)	
        && !bt_flag2_is_set(APP_FLAG2_CHARGE_POWERDOWN)
	#ifdef CONFIG_TWS
		&& (get_tws_prim_sec()!=TWS_PRIM_SEC_SECOND) 
		&& !bt_flag2_is_set(APP_FLAG2_STEREO_BUTTON_PRESS)
		&& (app_tws_h->piconet_role != HCI_ROLE_SLAVE) 
	#endif
        )
	{
		if(!get_Charge_is_Preparing())
		{
			app_wave_file_play_start(APP_WAVE_FILE_ID_DISCONN);
		}
		else
		{
			if(sys_hdl->bt_cb)
			{
				(sys_hdl->bt_cb)(); 
			}
			sys_hdl->bt_cb = NULL;
		}
    }

    bt_flag2_operate(APP_FLAG2_VOL_SYNC,0);
    bt_flag2_operate(APP_FLAG2_VOL_SYNC_OK,0);
    bt_flag1_operate(APP_FLAG_AVRCP_FLAG, 0);
#ifdef CONFIG_TWS
    unset_tws_flag(TWS_FLAG_A2DP_CONTENT_PROTECT);
#endif
    /* bt_flag1_operate(APP_FLAG_GET_REMOTE_ADDR,0); */
#ifdef CONFIG_BLUETOOTH_HID
    bt_flag1_operate(APP_FLAG_HID_CONNECTION,0);
#endif
#if (BT_MODE==BT_MODE_1V2)
    if(app_check_bt_mode(BT_MODE_1V2))
    {
        memcpy( (uint8 *)&env_h->env_data.default_btaddr, (uint8 *)raddr, sizeof(btaddr_t));
        ret = app_env_write_action(&(env_h->env_data.default_btaddr),0);
        if(ret)
        {
            msg.id = MSG_ENV_WRITE_ACTION;
            msg.arg = 0x00;
            msg.hdl = 0;
            msg_lush_put(&msg);
        }
    }
#endif

    if((sys_hdl->flag_sm1 & (APP_FLAG_DUT_MODE_ENABLE|APP_FLAG_POWERDOWN)))
    {
    #ifdef CONFIG_TWS
        if (!bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
    #endif
        {
            bt_unit_disable(sys_hdl->unit);
        //if(bt_flag1_is_set(APP_FLAG_LINEIN))
        //{
        //    jtask_stop(sys_hdl->app_reset_task);
        //    jtask_schedule(sys_hdl->app_reset_task, 1000, (jthread_func)hci_disable_task, (void *)sys_hdl->unit);
        //}
        #ifndef CONFIG_TWS
            if(bt_flag1_is_set(APP_FLAG_POWERDOWN))
                app_bt_shedule_task((jthread_func)app_powerdown_action, (void *)sys_hdl->unit, 1000);
		#endif
        //os_printf("app_bt_disconn_wrap, Exit 1, 0x%x \r\n", sys_hdl->flag_sm1);
            return;
        }
    }


#if (BT_MODE==BT_MODE_1V2)
    if(0 == sys_hdl->reconn_num_flag)
#endif
    {
        jtask_stop(sys_hdl->app_auto_con_task);
    }

// if(!(sys_hdl->flag_sm1 & (APP_FLAG_DEBUG_FLAG2)))
    {
        app_bt_reset_policy_iocap();
    }
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
    if(bt_flag1_is_set(APP_FLAG_LINEIN) && bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
        return;
#endif

#if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
    clr_a2dp_flags(raddr);
    clr_hf_flags(raddr);
    clr_avrcp_flags(raddr);
    get_cur_hfp_raddr(&tmp);
    if (!hfp_has_sco_conn())
    {
        sys_hdl->flag_sm1 &= ~(APP_FLAG_HFP_CALLSETUP | APP_FLAG_SCO_CONNECTION| APP_FLAG_HFP_OUTGOING | APP_FLAG_CALL_ESTABLISHED);
        sys_hdl->flag_sm2 &= ~(APP_FLAG2_HFP_INCOMING | APP_FLAG2_CALL_PROCESS | APP_FLAG2_HFP_SECOND_CALLSETUP);
    }
    if (!hci_get_acl_link_count(sys_hdl->unit))
    {
        sys_hdl->flag_sm1 &= ~(APP_AUDIO_FLAG_SET);
    }
#else
    sys_hdl->flag_sm1 &= ~(APP_AUDIO_FLAG_SET);
#endif

#ifdef CONFIG_TWS
    if (app_tws_h->piconet_role == HCI_ROLE_SLAVE)
    {
        memcpy( (uint8 *)&env_h->env_data.default_btaddr, (uint8 *)raddr, sizeof(btaddr_t));
        ret = app_env_write_action(&(env_h->env_data.default_btaddr),1);       
        if(0)//(ret)        
        {            
            msg.id = MSG_ENV_WRITE_ACTION;            
            msg.arg = 0x01;            
            msg.hdl = 0;            
            msg_lush_put(&msg);        
        } 
        app_tws_h->piconet_role = HCI_ROLE_MASTER;
        bt_flag2_operate(APP_FLAG2_STEREO_AUTOCONN,0);
        bt_flag1_operate(APP_FLAG_AUTO_CONNECTION, 1);
        //bt_auto_connect_start();
        return;
    }
#endif	
    /* This disconnect reaon may be caused by the expired key */
    if(((reason == HCI_ERR_CONNECTION_TERMINATED_BY_LOCAL_HOST)
        || reason == HCI_ERR_REMOTE_USER_TERMINATED_CONNECTION)
        && env_h->env_cfg.bt_para.disconn_action
        && !bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_DISCONNEC))
    {
        //sys_hdl->flag_sm1 |= APP_FLAG_RECONNCTION;
        //bt_reconn_after_disconnect();
    #if (BT_MODE==BT_MODE_1V2)
        if((bt_flag1_is_set( APP_FLAG_AUTO_CONNECTION | APP_FLAG_RECONNCTION) &&( !bt_flag2_is_set(APP_FLAG2_MATCH_BUTTON)))
            &&(app_check_bt_mode(BT_MODE_1V2))
        )
        {
            os_printf("discon reason:TERMINATED_BY_LOCAL_HOST\r\n");
            if (bt_flag1_is_set( APP_FLAG_AUTO_CONNECTION))
            {
                flag_scan = 1;
                if(sys_hdl->auto_conn_id2 != 0)
                {
                    btaddr_t *address_ptr;
                    address_ptr = &env_h->env_data.key_pair[sys_hdl->auto_conn_id2 -1].btaddr;

                    if(btaddr_same(address_ptr,raddr))
                    {
                        sys_hdl->auto_conn_status = INITIATIVE_CONNECT_STOP;
                    }
                }
            #ifdef CONFIG_AUTO_CONN_AB
                bt_create_autoconn_timeout_AB_wrap((void*)retry_count);
            #else
                bt_auto_connect_next();
            #endif

            #if 0
                // 上电回连时在以0x16/0x13失败继续回连
                if(sys_hdl->auto_conn_id2 != 0)
                {
                    btaddr_t *address_ptr;
                    address_ptr = &env_h->env_data.key_pair[sys_hdl->auto_conn_id2 -1].btaddr;
                    // second reconnection fail,
                    if( (btaddr_same(address_ptr,raddr)) && (!((retry_count<=env_h->env_cfg.bt_para.auto_conn_count) && (reason == HCI_ERR_REMOTE_USER_TERMINATED_CONNECTION))))
                        sys_hdl->auto_conn_status = INITIATIVE_CONNECT_STOP;
                }
                if((retry_count<=env_h->env_cfg.bt_para.auto_conn_count) && (reason != HCI_ERR_REMOTE_USER_TERMINATED_CONNECTION))
                {
                    jtask_schedule(sys_hdl->app_auto_con_task,
                           6000,
                           bt_create_conn_timeout_wrap,
                           (void *)retry_count);
                }
                else
                {
                    bt_auto_connect_next();
                }
            #endif
            }

            if (bt_flag1_is_set( APP_FLAG_RECONNCTION))
            {
                os_printf("RECONNCTION:%d,%d,%d\r\n",retry_count,env_h->env_cfg.bt_para.disconn_retry_count,sys_hdl->reconn_num_flag);
                if((env_h->env_cfg.bt_para.disconn_retry_count ==APP_RETRY_FOREVER)||(retry_count<=env_h->env_cfg.bt_para.disconn_retry_count))
                {
                    jtask_schedule(sys_hdl->app_auto_con_task,
    	                           500,
    	                           bt_create_reconn_timeout_wrap,
    	                           (void *)retry_count);
                }
                else
                {
                    sys_hdl->reconn_num_flag &= ~0x01;// A,B disconnect(0x08), reconnect A fail, reconnect B
                    if(sys_hdl->reconn_num_flag == 2)
                    {
                        //os_printf("----copy addr1-----\r\n");
                        memcpy((uint8 *)&sys_hdl->reconn_btaddr,
    	                       (uint8 *)&sys_hdl->reconn_btaddr2,
    	                        sizeof(btaddr_t));
                        sys_hdl->reconn_num_flag = 0;
                        jtask_schedule(sys_hdl->app_auto_con_task,
    	                           500,
    	                           bt_create_reconn_timeout_wrap,
    	                           (void *)0);
                    }
                    else
                    {
                        if(btaddr_same(raddr,&sys_hdl->reconn_btaddr)) // if disconnect addr == reconnect addr
                        {
                            bt_auto_connect_stop();
                            flag_scan = 1;
                        }
                        else if(bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_PLAY|APP_FLAG2_RECONN_AFTER_CALL))
                        {
                            // B music playing or calling,A disconnect (HCI_ERR_CONNECTION_TIMEOUT), B disconnected, need reconnect A
                            jtask_schedule(sys_hdl->app_auto_con_task,
                                        env_h->env_cfg.bt_para.disconn_start,
                                        bt_create_reconn_timeout_wrap,
                                        (void *)0);
                            bt_flag2_operate(APP_FLAG2_RECONN_AFTER_PLAY,0);
                            bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL,0);
                        }
                    }
                }
            }
        }
        else
    #endif
        {
            jtask_schedule(sys_hdl->app_auto_con_task,
                                200,
                                bt_auto_con_timerout_wrap,
                                (void *)NULL);
        }
    }
    else if(!bt_flag2_is_set(APP_FLAG2_MATCH_BUTTON))
    {
        disconn_event =  app_bt_get_disconn_event();
    #ifndef CONFIG_TWS
        if(app_bt_get_disconn_event()==1)  
            disconn_event=0;
    #endif
        if(disconn_event == 0)
        {
            if(((reason == HCI_ERR_CONNECTION_TIMEOUT)
	    	     ||(reason == HCI_ERR_AUTHENTICATION_FAILURE)
                   ||(reason == HCI_ERR_LMP_RESPONSE_TIMEOUT)
	    	     || bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_DISCONNEC))
	            && env_h->env_cfg.bt_para.disconn_action
	            && (!bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION)))
            {
                if (bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_DISCONNEC))
                {
                    //os_printf("Reconnection after disconnection ====================\r\n");
                    bt_flag2_operate(APP_FLAG2_RECONN_AFTER_DISCONNEC, 0);
                 #if (BT_MODE==BT_MODE_1V2)
                    //app_wave_file_play_stop();
                    //memory_usage_show();
                    //if(!bt_flag1_is_set(APP_FLAG_MUSIC_PLAY))
                     //   sbc_mem_free();
                    //memory_usage_show();
                    int ret = 0;
                    if(app_check_bt_mode(BT_MODE_1V2))
                    {
                        ret = app_env_write_action(&(env_h->env_data.default_btaddr),1);
                        if(ret)
                        {
                            msg.id = MSG_ENV_WRITE_ACTION;
                            msg.arg = 0x01;
                            msg.hdl = 0;
                            //msg_lush_put(&msg);
                            app_env_write_flash((uint8)(msg.arg));
                        }
                    }
                #endif					
                }

                app_set_led_event_action(LED_EVENT_AUTO_CONNECTION); 
            #ifdef BEKEN_DEBUG
	        	os_printf("==reconnecting:"BTADDR_FORMAT",reconn_flag:%d,retry_count:%d\r\n", BTADDR(raddr),sys_hdl->reconn_num_flag,retry_count);
            #endif
            #if (BT_MODE==BT_MODE_1V2)
                if(app_check_bt_mode(BT_MODE_1V2))
                {
                    ret = 0;  // to record : the disconnected phone is the reconnected phone and, do  the retry time reach the max times?
                    if(sys_hdl->reconn_num_flag == 0)
                    {
                        if(bt_flag1_is_set( APP_FLAG_RECONNCTION) && ((retry_count<=env_h->env_cfg.bt_para.disconn_retry_count)||(env_h->env_cfg.bt_para.disconn_retry_count ==APP_RETRY_FOREVER)))
                        {
                        // A,B走远回连,A回连成功,B回连,A播音乐,B回连被终止,A走远回连,该先回连B
                            if(!btaddr_same(raddr,&sys_hdl->reconn_btaddr))
                            {
                                sys_hdl->reconn_btaddr2 = *raddr;
                                sys_hdl->reconn_num_flag |= 2;
                            }
                            ret = 1;
                        }
                        else  //
                        {
                            sys_hdl->reconn_btaddr = *raddr;
                            sys_hdl->reconn_num_flag = 1;
                            #ifdef BEKEN_DEBUG
                             //os_printf("---Set First Addr---\r\n");
                            #endif
                        }
                    }
                    else if((!btaddr_same(raddr,&sys_hdl->reconn_btaddr)) && (sys_hdl->reconn_num_flag !=0))
                    {
                        sys_hdl->reconn_btaddr2 = *raddr;
                        sys_hdl->reconn_num_flag |= 2;
                    #ifdef BEKEN_DEBUG
                        os_printf("First reconn:"BTADDR_FORMAT"\r\n", BTADDR(&sys_hdl->reconn_btaddr));
                        os_printf("second reconn:"BTADDR_FORMAT"\r\n", BTADDR(&sys_hdl->reconn_btaddr2));
                    #endif
                    }
                    else
                    {
                        if((env_h->env_cfg.bt_para.disconn_retry_count == APP_RETRY_FOREVER)||(retry_count<=env_h->env_cfg.bt_para.disconn_retry_count))
                        {
                           os_printf("reconnect phone failed,retry_count : %d\r\n ",retry_count);
                           ret  = 1;
                        }
                        else if(sys_hdl->reconn_num_flag&0x02)
                        {
                            sys_hdl->reconn_num_flag &= ~0x01;
                            //os_printf("----copy addr2-----\r\n");
                            memcpy((uint8 *)&sys_hdl->reconn_btaddr,
    	                            (uint8 *)&sys_hdl->reconn_btaddr2,
    	                            sizeof(btaddr_t));
                        }
                        else  // no second phone need to reconnection and last reconnection reach max time
                        {
                            sys_hdl->reconn_num_flag = 0;
                        }
                    }
                    sys_hdl->unit->hci_link_policy |= HCI_LINK_POLICY_ENABLE_ROLE_SWITCH;
                    sys_hdl->flag_sm1 |= APP_FLAG_RECONNCTION;
                    if(hfp_has_sco_conn())
                    {
                        //os_printf("====call process\r\n");
                        get_cur_hfp_raddr(&tmp);
                        if(!btaddr_same(tmp,raddr))
                            bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL, 1);
                        else
                            bt_flag2_operate(APP_FLAG2_CALL_PROCESS,0);
                    }
                #ifndef  CONN_WITH_MUSIC
                    else if(get_other_a2dp_play_flag(raddr))
                    {
                        //os_printf("====music play process\r\n");
                        sys_hdl->reconn_btaddr = *raddr;
                        bt_flag2_operate(APP_FLAG2_RECONN_AFTER_PLAY,1);
                    }
                #endif
                    else
                    {
                        if( ret == 1)
                        {
                            jtask_schedule(sys_hdl->app_auto_con_task,
                                    env_h->env_cfg.bt_para.disconn_start,
                                    bt_create_reconn_timeout_wrap,
                                    (void *)retry_count);
                        }
                        else if(sys_hdl->reconn_num_flag !=  0) //
                        {
                            jtask_schedule(sys_hdl->app_auto_con_task,
                                    env_h->env_cfg.bt_para.disconn_start,
                                    bt_create_reconn_timeout_wrap,
                                    (void *)0);
                            if(sys_hdl->reconn_num_flag == 2)
                                sys_hdl->reconn_num_flag = 0;
                        }

                        if(!hci_get_acl_link_count(sys_hdl->unit))
                        {
                            app_set_led_event_action(LED_EVENT_AUTO_CONNECTION);
                        }
                        else
                        {
                            if(a2dp_has_music())
                                app_set_led_event_action(LED_EVENT_BT_PLAY);
                            else
                                app_set_led_event_action(LED_EVENT_BT_PAUSE);
                        }
                    }
                }
                else
            #endif
                {
                    sys_hdl->reconn_btaddr = *raddr;
                    sys_hdl->unit->hci_link_policy |= HCI_LINK_POLICY_ENABLE_ROLE_SWITCH;
                    sys_hdl->flag_sm1 |= APP_FLAG_RECONNCTION;
                    jtask_schedule(sys_hdl->app_auto_con_task,
                               env_h->env_cfg.bt_para.disconn_start,
                               bt_create_reconn_timeout_wrap,
                               (void *)0);
                }
            }
            else
            {
                if(bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION))
                {
                    flag_scan = 1;
                    if(sys_hdl->auto_conn_id2 != 0)
                    {
                        btaddr_t *address_ptr;
                        address_ptr = &env_h->env_data.key_pair[sys_hdl->auto_conn_id2 -1].btaddr;

                        if( (btaddr_same(address_ptr,raddr)) && (retry_count>env_h->env_cfg.bt_para.auto_conn_count))
                        {
                            sys_hdl->auto_conn_status = INITIATIVE_CONNECT_STOP;
                        }
                    }

                    if(retry_count<=env_h->env_cfg.bt_para.auto_conn_count)
                    {
                    #ifndef CONFIG_AUTO_CONN_AB
                        jtask_schedule(sys_hdl->app_auto_con_task,
                           6000,
                           bt_create_conn_timeout_wrap,
                           (void *)retry_count);
                    #else
                        bt_create_autoconn_timeout_AB_wrap((void*)retry_count);
                    #endif
                    }
                    else
                    {
                    #ifdef CONFIG_AUTO_CONN_AB
                        bt_create_autoconn_timeout_AB_wrap((void*)retry_count);
                    #else
                    #if (BT_MODE==BT_MODE_1V2)
                        bt_auto_connect_next();
                    #endif
                    #endif
                    }
                }
			#if (BT_MODE==BT_MODE_1V2)&& !defined(CONN_WITH_MUSIC)
                if(bt_flag1_is_set(APP_FLAG_RECONNCTION) && bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_PLAY))
                {
                    jtask_schedule(sys_hdl->app_auto_con_task,
                                env_h->env_cfg.bt_para.disconn_start,
                                bt_create_reconn_timeout_wrap,
                                (void *)0);
                }
            #endif
            #if 0
                if(!hci_get_acl_link_count(sys_hdl->unit))
                {
                    app_set_led_event_action(LED_EVENT_MATCH_STATE);
                }
            #endif
            }
        }
        else if(!bt_flag1_is_set(APP_FLAG_MATCH_ENABLE) && disconn_event == 1)// won't be here if default setting
        {
            sys_hdl->unit->hci_link_policy |= HCI_LINK_POLICY_ENABLE_ROLE_SWITCH;
            bt_flag1_operate(APP_FLAG_AUTO_CONNECTION, 1);
            app_set_led_event_action( LED_EVENT_AUTO_CONNECTION);
            jtask_schedule(sys_hdl->app_auto_con_task,
                        200,
                        bt_create_conn_timeout_wrap,
                        (void *)0);
        }
    }
    
#ifdef CONFIG_TWS
    if( bt_flag2_is_set( APP_FLAG2_STEREO_STREAMING ) )
        a2dp_src_cmd_stream_suspend();

	bt_flag2_operate(APP_FLAG2_STEREO_AUTOCONN_RETRY|APP_FLAG2_STEREO_STREAMING, 0 );

    if(bt_flag2_is_set(APP_FLAG2_STEREO_BUTTON_PRESS))
        app_button_stereo();	
    else if((0 == app_bt_get_disconn_event()) && bt_flag2_is_set(APP_FLAG2_MATCH_BUTTON))
    {
        bt_flag2_operate(APP_FLAG2_MATCH_BUTTON, 0);
        app_set_led_event_action(LED_EVENT_MATCH_STATE);
        bt_unit_set_scan_enable(sys_hdl->unit, HCI_INQUIRY_SCAN_ENABLE|HCI_PAGE_SCAN_ENABLE);
    }
    else
    {
        app_bt_check_inquiry_set(0);
    }
#else

    if(bt_flag2_is_set(APP_FLAG2_MATCH_BUTTON))
    {
        /* Match flag can only be cleared when all disconnection is done */
    #if (BT_MODE==BT_MODE_1V2)
        if (hci_get_acl_link_count(sys_hdl->unit)&&app_check_bt_mode(BT_MODE_1V2))
        {
        #ifdef NO_SCAN_WHEN_WORKING
            if(hfp_has_sco_conn() || a2dp_has_music())
            {
                bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
            }
            else
        #endif
            {
                if(app_env_check_inquiry_always())
                    bt_unit_set_scan_enable(sys_hdl->unit,HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
                else
                    bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
            }
        }
        else
    #endif
        {
            bt_flag2_operate(APP_FLAG2_MATCH_BUTTON, 0);
            bt_unit_set_scan_enable(sys_hdl->unit,HCI_INQUIRY_SCAN_ENABLE| HCI_PAGE_SCAN_ENABLE);
	    //os_printf("==========match button clear==========\r\n");
         }
    }
    else if((flag_scan == 0) && (! bt_flag1_is_set(APP_FLAG_RECONNCTION|APP_FLAG_AUTO_CONNECTION)))
    {
    #ifdef NO_SCAN_WHEN_WORKING
        if(hfp_has_sco_conn() || a2dp_has_music())
        {
            bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
        }
        else
    #endif
        {
        #ifdef CONFIG_BLUETOOTH_COEXIST
            if(1) 
        #else
            if(SYS_WM_BT_MODE == sys_hdl->sys_work_mode) 
        #endif
            {
                if(app_env_check_inquiry_always())
                {
                    app_set_led_event_action( LED_EVENT_MATCH_STATE );
                    bt_unit_set_scan_enable(sys_hdl->unit,HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
                }
                else
                {
                    app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);
                    bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
                }
            }
        }
    }
#endif
#ifdef CONFIG_TWS
	app_bt_reenter_dut_mode();
#endif
    (void)flag_scan;
}

void app_bt_auto_conn_ok(void)
{
//	#ifndef BT_ONE_TO_MULTIPLE

    os_printf("app_bt_auto_conn_ok\r\n");

    app_handle_t sys_hdl = app_get_sys_handler();
    jtask_stop( sys_hdl->app_reset_task );

    if(sys_hdl->flag_sm1 & (APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
    {
        if(bt_flag1_is_set(APP_FLAG_SCO_CONNECTION))
        {
            bt_flag1_operate(APP_FLAG_HFP_CALLSETUP, 1);
        }

        if(a2dp_has_connection())
        {
        #ifdef CONFIG_BLUETOOTH_COEXIST
            if(app_is_bt_mode())
        #endif
            app_wave_file_play_start(APP_WAVE_FILE_ID_CONN);
        }

        sys_hdl->flag_sm1 &= ~(APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION);

    #ifdef IPHONE_BAT_DISPLAY
        // clear the bat_lever  backup
        iphone_bat_lever_bak = 0;
        iphone_bat_lever_bak_cnt = 0;
    #endif
    }

    app_bt_reset_policy_iocap();
    
#ifdef CONFIG_TWS
    if(bt_flag2_is_set( APP_FLAG2_STEREO_BUTTON_PRESS ))
	{
	//app_button_sw_action(BUTTON_MODE);
		app_button_stereo();
	}
#endif
}

extern void LSLC_crystal_calibration_set(uint8 calib);
//extern uint32_t avrcp_has_the_connection(btaddr_t);
void app_bt_profile_conn_wrap(uint8 profileId,btaddr_t* rtaddr)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t  env_h = app_env_get_handle();
    uint32 pflag = 0;
    APP_LED_EVENT led_event = LED_EVENT_END;
    int waveid = -1;
    MSG_T msg;
#ifdef CONFIG_TWS
    app_tws_t app_tws_h = app_get_tws_handler();
	uint8 wave_flag=0;
#endif
#if 1/*(CONFIG_APP_TOOLKIT_5 == 1)*/||((defined(BT_ONE_TO_MULTIPLE) && !defined(BT_ONLY_ONE_BT)) || defined(CONFIG_TWS))
    int ret = 0;
#endif
    os_printf("app_bt_profile_conn_wrap(%d)\r\n", profileId);

    //jtask_stop(sys_hdl->app_auto_con_task);

    CLEAR_PWDOWN_TICK;
    CLEAR_SLEEP_TICK;
#ifdef CONFIG_TWS
	tws_stereo_autoconn_cnt_set(0);
#endif
#if defined(CONFIG_BLUETOOTH_HID)
    if((app_env_check_HID_profile_enable())
        &&(profileId == PROFILE_ID_AVRCP||profileId == PROFILE_ID_A2DP||profileId == PROFILE_ID_HFP))
    {
        if((a2dp_has_connection())&&(hfp_has_connection()) )//&& (avrcp_is_connection()))
        {
            os_printf("profileId=%x\r\n",profileId);
            //if((sys_hdl->flag_sm1 & (APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
            //        && !(sys_hdl->flag_sm1 & APP_FLAG_HID_CONNECTION))
            if(!(sys_hdl->flag_sm1 & APP_FLAG_HID_CONNECTION))
            {
                os_printf("PROFILE_ID_HID\r\n");
                sys_hdl->auto_conn_status = INITIATIVE_CONNECT_HID;//手机自动回连
                app_bt_allpro_conn_start(100,rtaddr);
            }

            if(profileId == PROFILE_ID_AVRCP)
            {
                return;
            }
        }
    }
#endif
#ifdef CONFIG_TWS
    bt_flag2_operate(APP_FLAG2_STEREO_CONNECTTING|APP_FLAG2_STEREO_INQUIRY_RES|APP_FLAG2_STEREO_AUTOCONN, 0);
#endif
    if(profileId == PROFILE_ID_A2DP)
    {
    #ifdef CONFIG_TWS
    	if (!bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION))
			wave_flag = 1;
	#endif	
        pflag = APP_FLAG_A2DP_CONNECTION;
        led_event = LED_EVENT_CONN;
    #ifndef BT_ONE_TO_MULTIPLE
        bt_flag1_operate(APP_BUTTON_FLAG_PLAY_PAUSE, 0);
    #endif
    }
#ifdef CONFIG_BLUETOOTH_HFP
    else if(profileId == PROFILE_ID_HFP)
    {
    #ifdef CONFIG_TWS
    	if (!bt_flag1_is_set(APP_FLAG_HFP_CONNECTION))
			wave_flag = 1;
	#endif	
        pflag = APP_FLAG_HFP_CONNECTION;
        led_event = LED_EVENT_CONN;
        /* app_env_write_action(&sys_hdl->remote_btaddr,0x01); */
        //msg.id = MSG_ENV_WRITE_ACTION;
        //msg.arg = 0x01;
        //msg.hdl = 0;
        //msg_lush_put(&msg);
    }
#endif
    sys_hdl->flag_sm1 |= pflag;
#ifdef CONFIG_TWS
    if(bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE) 
        && (get_tws_prim_sec() == TWS_PRIM_SEC_SECOND))
	{
		bt_flag2_operate(APP_FLAG2_STEREO_MATCH_BUTTON, 0);
		app_set_led_event_action(LED_EVENT_STEREO_CONN_MODE);
		app_wave_file_play_start(APP_WAVE_FILE_ID_STEREO_CONN);
		bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
		led_event = LED_EVENT_END;
	}
#endif
    
    if(a2dp_has_music() )
        app_set_led_event_action(LED_EVENT_BT_PLAY);
    else if(has_hfp_flag_1toN(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
        app_set_led_event_action(LED_EVENT_CALL_ESTABLISHMENT);
    else
        app_set_led_event_action(led_event);

#ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    bt_flag1_operate(APP_FLAG_SNIFF_MODE_CHANGE,0);
#endif

#ifdef IPHONE_BAT_DISPLAY
    if (!(sys_hdl->flag_sm1 & (APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION)))
    {
    // clear the bat_lever  backup
        iphone_bat_lever_bak = 0;
        iphone_bat_lever_bak_cnt = 0;
    }
#endif

    app_env_get_handle()->env_data.disconnect_reason = 0xff;
    /* app_env_write_action(&sys_hdl->remote_btaddr,0x01);*/

    if(profileId == PROFILE_ID_A2DP)
    {
    #if (CONFIG_RF_CALI_TYPE&CALI_BY_PHONE_BIT)
        if(!bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE)
            && bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION))
        {
            LSLC_crystal_calibration_set(0);
        }
	#endif	
       if( !bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION | APP_FLAG_RECONNCTION)
    #if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
           && ( !hfp_has_the_connection(*rtaddr) ))
    #else
          && ( !bt_flag1_is_set(APP_FLAG_HFP_CONNECTION) ))
    #endif
        {
            //app_env_write_action(&sys_hdl->remote_btaddr,0x01);
            memcpy( (uint8 *)&env_h->env_data.default_btaddr, (uint8 *)rtaddr, sizeof(btaddr_t));
        #ifdef CONFIG_TWS
            if( bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE)
                && (get_tws_prim_sec()==TWS_PRIM_SEC_SECOND))
            {
                /*
                flash_erase_sector(FLASH_ENVDATA_DEF_ADDR);
                flash_write_data( (uint8 *)&(env_h->env_data), FLASH_ENVDATA_DEF_ADDR,
                sizeof(app_env_data_t));
                */
                app_flash_write_env_data();
            }
            else
        #endif
            {
            #ifdef CONFIG_TWS
                if ((app_tws_h->piconet_role != HCI_ROLE_SLAVE) && (wave_flag))
            #endif
			{
            #ifdef CONFIG_BLUETOOTH_COEXIST
                if(app_is_bt_mode())
            #endif
                {
                waveid = APP_WAVE_FILE_ID_CONN;
                app_wave_file_play_start(waveid);
                }
			}
                ret = app_env_write_action(&(env_h->env_data.default_btaddr),1);
                if(ret ||app_check_bt_mode(BT_MODE_1V1))
                {
                    msg.id = MSG_ENV_WRITE_ACTION;
                    msg.arg = 0x01;
                    msg.hdl = 0;
                    msg_lush_put(&msg);
                }
            }

            if(sys_hdl->unit)
            {
            #ifdef BT_ONE_TO_MULTIPLE
                if(hci_get_acl_link_count(sys_hdl->unit) >= BT_MAX_AG_COUNT)
                {
                     bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
                }
                else
                {
                #ifdef NO_SCAN_WHEN_WORKING
                    if(hfp_has_sco_conn() || a2dp_has_music())
                    {
                        bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
                    }
                    else
                #endif
                    {
                        if(app_env_check_inquiry_always())
                        {
                            bt_unit_set_scan_enable(sys_hdl->unit,HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
                        }
                        else
                        {
                            bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
                        }

                    }
                }
				#if (BT_MODE==BT_MODE_1V2)&& !defined(CONN_WITH_MUSIC)
                    app_bt_set_1v2_match_timeout();
                #endif
            #else
                #ifdef CONFIG_TWS
                #else
                    bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
                #endif
            
            #endif
            }
        }
    #if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
        if(!has_hfp_flag_1toN(APP_FLAG_HFP_CALLSETUP|APP_FLAG_SCO_CONNECTION))//if we connect the a2dp while another phone is calling up and toggle native,we shoundn't set BUTTON_TYPE_NON_HFP
    #else
        if(!get_current_hfp_flag(APP_FLAG_HFP_CALLSETUP|APP_FLAG_SCO_CONNECTION))
    #endif
        {
          app_button_type_set(BUTTON_TYPE_NON_HFP);
      	}
        //bt_flag1_operate(APP_FLAG_FIRST_ENCRYP, 0);
    }
#ifdef CONFIG_BLUETOOTH_HFP
    else if(profileId == PROFILE_ID_HFP)
    {
        if( !bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION | APP_FLAG_RECONNCTION)
        #ifdef BT_ONE_TO_MULTIPLE
           && ( !a2dp_has_the_connection(*rtaddr) ))
        #else
           && ( !bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION)) )
        #endif
        {
        #ifdef CONFIG_BLUETOOTH_COEXIST
            if(app_is_bt_mode())
        #endif
            {
            #ifdef CONFIG_TWS
             	if (wave_flag)
			#endif
                {
                waveid = APP_WAVE_FILE_ID_CONN;
                app_wave_file_play_start(waveid);
				}
            }
            //app_env_write_action(&sys_hdl->remote_btaddr,0x01);
            memcpy( (uint8 *)&env_h->env_data.default_btaddr, (uint8 *)rtaddr, sizeof(btaddr_t));
            ret = app_env_write_action(&(env_h->env_data.default_btaddr),1);
            if(ret ||app_check_bt_mode(BT_MODE_1V1))
            {
                msg.id = MSG_ENV_WRITE_ACTION;
                msg.arg = 0x01;
                msg.hdl = 0;
                msg_lush_put(&msg);
            }

            if(sys_hdl->unit)
            {
            #ifdef BT_ONE_TO_MULTIPLE
                if(hci_get_acl_link_count(sys_hdl->unit) >= BT_MAX_AG_COUNT)
                {
                     bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
                }
                else
                {
                #ifdef NO_SCAN_WHEN_WORKING
                    if(hfp_has_sco_conn() || a2dp_has_music())
                    {
                        bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
                    }
                    else
                #endif
                    {
                        if(app_env_check_inquiry_always())
                            bt_unit_set_scan_enable(sys_hdl->unit,HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
                        else
                            bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
                    }
                }
				#if (BT_MODE==BT_MODE_1V2)&& !defined(CONN_WITH_MUSIC)
                    app_bt_set_1v2_match_timeout();
                #endif
            #else
                bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
            #endif
            }
        }
        if((sys_hdl->flag_sm1 & (APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
        #ifdef BT_ONE_TO_MULTIPLE
           && ( !a2dp_has_the_connection(*rtaddr) ))
        #else
           && !a2dp_has_connection())
        #endif
        {
            
            //if(!(sys_hdl->flag_sm1 & APP_FLAG_ROLE_SWITCH))
            if(bt_flag1_is_set(APP_FLAG_HFP_CALLSETUP))
                app_bt_allpro_conn_start(AUTO_RE_CONNECT_SCHED_DELAY_A2DP_SWITCH * 3,rtaddr);
            else
                app_bt_allpro_conn_start(AUTO_RE_CONNECT_SCHED_DELAY_A2DP_SWITCH,rtaddr);
        }

        // Allow auto/re-connection to exit properly
        if ((sys_hdl->flag_sm1 & (APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
        #ifdef BT_ONE_TO_MULTIPLE
            && (a2dp_has_the_connection(*rtaddr))
            && (avrcp_has_the_connection(*rtaddr)))
        #else
            && (a2dp_has_connection())
            && (avrcp_is_connection()))
        #endif
        {
            app_bt_allpro_conn_start(201,rtaddr);
        }

        //bt_flag1_operate(APP_FLAG_FIRST_ENCRYP, 0);

    }
#endif

#ifdef CONFIG_TWS
    if(bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
    { 
    
        if(get_tws_prim_sec() == TWS_PRIM_SEC_SECOND) 
        {
            app_bt_stereo_auto_conn_stop();
            bt_flag2_operate(APP_FLAG2_STEREO_CONNECTTING|APP_FLAG2_MATCH_BUTTON, 0);
        }
        else
        {

        }
    }	
    if((get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)&&a2dp_has_connection()&&hfp_has_connection())
    {
        hci_link_t *tmp_link = NULL;
        hci_write_default_pkt_type_cp cp;
    	tmp_link = hci_link_lookup_btaddr(sys_hdl->unit, rtaddr, HCI_LINK_ACL);
        cp.pswd = 0x9527;
        cp.handle = tmp_link->hl_handle;
        cp.pkt_type = 1;        // poll pkt;
        cp.poll_interval = 40;  // 40 * 0.625
        hci_link_set_default_pkt_type(&cp);
        if((!bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
            && sys_hdl->unit)  
        {
            bt_unit_twscontrol_scan(HCI_PAGE_SCAN_ENABLE);
        }
    }
#endif    

}

void app_bt_profile_disconn_wrap(uint8 profileId, void *app_ptr)
{
    uint32 pflag = 0;
    uint32 cflag = 0,cflag2 = 0;
    btaddr_t r_addr;
//#ifdef BT_ONLY_ONE_BT
//    int32 waveid = -1;
//#endif
    app_handle_t sys_hdl = app_get_sys_handler();
    uint32 ret;
#if 0//def CONFIG_TWS
    app_tws_t app_tws_h = app_get_tws_handler();
#endif
    if(profileId == PROFILE_ID_A2DP)
    {
        ret = a2dp_get_remote_addr(app_ptr, &r_addr);
    #ifdef BT_ONE_TO_MULTIPLE
        if(a2dp_all_apps_is_unused())
    #else
        if(1)
    #endif
        {
            aud_PAmute_oper(1);
            pflag = APP_FLAG_A2DP_CONNECTION;
            cflag = APP_BUTTON_FLAG_PLAY_PAUSE|APP_FLAG_MUSIC_PLAY|APP_FLAG_A2DP_CONNECTION;
        }
//    #ifdef BT_ONLY_ONE_BT
//        waveid = APP_WAVE_FILE_ID_DISCONN;
//    #endif

    }
#ifdef CONFIG_BLUETOOTH_HFP
    else if(profileId == PROFILE_ID_HFP)
    {
        ret = hfp_get_remote_addr(app_ptr, &r_addr);
    #if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
        if(!hfp_has_connection())//for two devices' HFP connection,if one discon,the flag shoundn't clear
    #endif
        {
            aud_PAmute_oper(1);
            pflag = APP_FLAG_HFP_CONNECTION;
            cflag = APP_FLAG_HFP_CONNECTION | APP_FLAG_HFP_CALLSETUP | APP_FLAG_HFP_OUTGOING | APP_FLAG_CALL_ESTABLISHED;
            cflag2 = APP_FLAG2_HFP_INCOMING | APP_FLAG2_CALL_PROCESS | APP_FLAG2_HFP_SECOND_CALLSETUP;
            app_button_type_set(BUTTON_TYPE_NON_HFP);
        }
    #if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
        /* When the one of hfps disconnected abnormally,some flags maybe  not be clear */
        else if(!hfp_has_sco_conn())
        {
            pflag = APP_FLAG_HFP_CONNECTION;
            cflag = APP_HFP_AT_CMD_FLAG1_SET;
            cflag2 = APP_HFP_AT_CMD_FLAG2_SET;

        }
    #endif
    }
#endif

    //当手机没有开启hfp选项时，回连可以继续
    //如果没有A2DP，回连A2DP;已经有A2DP,则允许回连完成退出
    if((profileId == PROFILE_ID_HFP)
        && (sys_hdl->flag_sm1 & (APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION)))
    {
        ret = hfp_get_remote_addr(app_ptr, &r_addr);
        if (UWE_OK == ret)
        {
            hci_link_t * tmp_link = NULL;
            tmp_link = hci_link_lookup_btaddr(sys_hdl->unit, &r_addr, HCI_LINK_ACL);
            if((hf_check_flag(app_ptr,APP_FLAG_DISCONN_RECONN))&&(tmp_link->hl_state == HCI_LINK_OPEN) && (btaddr_same(&sys_hdl->reconn_btaddr,&r_addr)))
            {
                os_printf("link state:%d,remote addr:"BTADDR_FORMAT"\r\n",tmp_link->hl_state,BTADDR(&sys_hdl->reconn_btaddr));
                os_printf("diconnect addr:"BTADDR_FORMAT"\r\n",BTADDR(&r_addr));
                hf_set_flag(app_ptr, FLAG_HFP_AUTOCONN_DISCONNED);
                hf_clear_flag(app_ptr, APP_FLAG_DISCONN_RECONN);
                bt_flag1_operate(FLAG_HFP_AUTOCONN_DISCONNED, 1);

                app_bt_allpro_conn_start(8000,&r_addr);
            }
        }
    }

    // If A2DP failed for Auto/Re-connection
    // terminate and exit the auto/re-connection
    if((profileId == PROFILE_ID_A2DP)
        && (sys_hdl->flag_sm1 & (APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION)))
    {
        hci_link_t *tmp_link = NULL;
        int max_retry = 0;
        app_env_handle_t env_h = app_env_get_handle();

        max_retry = env_h->env_cfg.bt_para.disconn_retry_count + 1;
        ret = a2dp_get_remote_addr(app_ptr, &r_addr);

        //os_printf("a2dp_get_remote_addr ret is %d\r\n",ret);

        if (UWE_OK == ret)
        {
            // If ACL is connected but A2DP vanishes,exit the re-connection but allow next link
            // to be connected if there is one
            // call the timeout_wrap functions with the arg set to the MAX retry number to indicate
            // abnormal exit from the auto/re-connection process
            tmp_link = hci_link_lookup_btaddr(sys_hdl->unit, &r_addr, HCI_LINK_ACL);

            if (tmp_link->hl_state == HCI_LINK_OPEN)
            {
                if (sys_hdl->flag_sm1 & APP_FLAG_AUTO_CONNECTION)
                {
                    int index;
                    index = sys_hdl->auto_conn_id-1;
                    if((env_h->env_data.key_pair[index].used == 1) || (env_h->env_data.key_pair[index].used == 2))
                    {
                        btaddr_t *address_ptr;
                        address_ptr = &env_h->env_data.key_pair[index].btaddr;

                        if(btaddr_same(address_ptr,&r_addr))
                        {
                            jtask_stop(sys_hdl->app_auto_con_task);
                            //os_printf("a2dp not available, exit auto connection\r\n");
                            bt_create_conn_timeout_wrap((void *)max_retry);
                        }
                    }
                  }

                if ((sys_hdl->flag_sm1 & APP_FLAG_RECONNCTION) && (btaddr_same(&sys_hdl->reconn_btaddr,&r_addr)))
                {
                    jtask_stop(sys_hdl->app_auto_con_task);
                    //os_printf("a2dp not available, exit re-connection\r\n");
                    bt_create_reconn_timeout_wrap((void *)max_retry);
                }
            }
            else
                os_printf("tmp_link->hl_state is %d\r\n", tmp_link->hl_state);
        }
    }
#ifdef CONFIG_TWS
    //if( !bt_flag2_is_set( APP_FLAG2_STEREO_BUTTON_PRESS ))
        bt_unit_write_inquiry_IAC( sys_hdl->unit, 0 );
    
    if( bt_flag2_is_set( APP_FLAG2_STEREO_STREAMING )
        && (profileId == PROFILE_ID_A2DP))
        a2dp_src_cmd_stream_suspend();
#endif

#if defined(BT_ONE_TO_MULTIPLE)//||defined(CONFIG_TWS)
//#if 1/*(CONFIG_APP_TOOLKIT_5 == 1)*/||defined(BT_ONE_TO_MULTIPLE)
    if((SYS_WM_BT_MODE ==sys_hdl->sys_work_mode) && (!bt_flag2_is_set(APP_FLAG2_RECONN_AFTER_DISCONNEC))
        && (!a2dp_has_the_connection(r_addr))
         && (!hfp_has_the_connection(r_addr))
	 &&app_check_bt_mode(BT_MODE_1V2)
    )
    {
        app_wave_file_play_start(APP_WAVE_FILE_ID_DISCONN);
    }
#endif

    if(!(sys_hdl->flag_sm1 & pflag))
    {
        return;
    }

    sys_hdl->flag_sm1 &= ~cflag;
    sys_hdl->flag_sm2 &= ~cflag2;
#if 0//def CONFIG_TWS
    if(!bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
        set_tws_prim_sec(TWS_PRIM_SEC_UNDEFINED);	
#endif
}
#if TWS_HFP_ENABLE
void tws_hfp_audio_init(void)
{
    os_printf("tws_hfp_audio_init()\n");
    //BK3000_GPIO_8_CONFIG = 2; // for toggle, GPIO4, 
    
    CLEAR_WDT;
    
    app_wave_file_play_clean();
    
    aud_close();
    #if 1
    aud_initial(FREQ_OF_MSBC_ON_ESCO, 2, 16);
    #else
    extern AUDIO_CTRL_BLK  audio_ctrl_blk;
    //aud_dma_reset();
    aud_dma_initial();
    rb_init(&audio_ctrl_blk.aud_rb, (uint8 *)audio_ctrl_blk.data_buff, 1, AUDIO_BUFF_LEN);
    aud_fill_buffer((uint8 *)audio_ctrl_blk.data_buff, (AUDIO_BUFF_LEN - (480 * 3) - (4*17)));
    #endif
    
    //aud_initial(get_current_hfp_freq(), 2, 16);
#if (CONFIG_AUDIO_TRANSFER_DMA == 1)
    //aud_dma_initial();
#endif

    //rb_init(&audio_ctrl_blk.aud_rb, (uint8 *)audio_ctrl_blk.data_buff, 1, AUDIO_BUFF_LEN);
    //aud_fill_buffer((uint8 *)audio_ctrl_blk.data_buff, (AUDIO_BUFF_LEN - (480 * 1) - (4*16)));
    //aud_fill_buffer((uint8 *)audio_ctrl_blk.data_buff, (AUDIO_BUFF_LEN - (480 * 2) - (4*16)));
    //aud_fill_buffer((uint8 *)audio_ctrl_blk.data_buff, (AUDIO_BUFF_LEN - (480 * 3) - (4*17)));

    //for test
    os_printf("++CS\n\n");

    //tws_msbc_decode_prapare();
    aud_mic_open(1);
    aud_volume_set(get_current_hfp_volume());
    
    CLEAR_WDT;
    
    //BK3000_GPIO_8_CONFIG = 0; // for toggle, GPIO4, 
}
#endif

void bt_flag1_operate(uint32 flag, uint8 oper)
{
    app_handle_t sys_hdl;
    uint32 interrupts_info, mask;

    sys_hdl = app_get_sys_handler();

    VICMR_disable_interrupts(&interrupts_info, &mask);
    if (1 == oper)
    {
        sys_hdl->flag_sm1 |= flag;
    }
    else if (0 == oper)
    {
        sys_hdl->flag_sm1 &= ~flag;
    }
    VICMR_restore_interrupts(interrupts_info, mask);

    return;
}

RAM_CODE uint32_t bt_flag1_is_set(uint32 flag)
{
    app_handle_t sys_hdl = app_get_sys_handler();

    return (sys_hdl->flag_sm1 & flag);
}

RAM_CODE void bt_flag2_operate(uint32 flag, uint8 oper)
{
    app_handle_t sys_hdl;
    uint32 interrupts_info, mask;

    sys_hdl = app_get_sys_handler();

    VICMR_disable_interrupts(&interrupts_info, &mask);
    if (1 == oper)
    {
        sys_hdl->flag_sm2 |= flag;
    }
    else if (0 == oper)
    {
        sys_hdl->flag_sm2 &= ~flag;
    }
    VICMR_restore_interrupts(interrupts_info, mask);

    return;
}
#if 0//def CONN_WITH_MUSIC
void bt_flag2_operate_0(uint32 flag)
{
    app_handle_t sys_hdl;
	uint32 interrupts_info, mask;

    sys_hdl = app_get_sys_handler();

    VICMR_disable_interrupts(&interrupts_info, &mask);
    sys_hdl->flag_sm2 &= ~flag;
    VICMR_restore_interrupts(interrupts_info, mask);

    return;
}
#endif

RAM_CODE uint32_t bt_flag2_is_set(uint32 flag)
{
    app_handle_t sys_hdl = app_get_sys_handler();

    return (sys_hdl->flag_sm2 & flag);
}

void app_bt_get_remote_addr(btaddr_t *addr)
{
    app_handle_t sys_hdl = app_get_sys_handler();

    btaddr_copy(addr, &sys_hdl->remote_btaddr);
}

void *app_bt_get_handle(uint8_t handle_type)
{
    void *param = NULL;
    app_handle_t sys_hdl = app_get_sys_handler();

    switch (handle_type)
    {
        case 0:
            param = (void *)sys_hdl->unit;
            break;
    #ifdef CONFIG_TWS
        case 1:
            param = (void *)&sys_hdl->link_handle;
            break;
        case 2:
            param = (void *)&sys_hdl->stereo_btaddr;
            break;
	#endif
        default:
            break;
    }

    return param;
}

void app_bt_reset_policy_iocap(void)
{
#if(CONFIG_AS_SLAVE_ROLE == 0)
    app_handle_t sys_hdl = app_get_sys_handler();
    sys_hdl->unit->hci_link_policy &= ~HCI_LINK_POLICY_ENABLE_ROLE_SWITCH;
#endif
#ifdef CONFIG_BLUETOOTH_SSP
    bt_sec_set_io_caps(HCI_IO_CAPABILITY_NO_IO);
#endif
}
#ifdef CONFIG_TWS
void app_reset_bt_baseband(void)
{
    os_printf("app_reset_bt_baseband()\n");
    app_handle_t sys_hdl = app_get_sys_handler();

    if(sys_hdl->unit != NULL)
    {
    #ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
        app_bt_slave_linein_set(0);
        linein_sbc_alloc_free();
    #endif	
        set_tws_prim_sec(TWS_PRIM_SEC_UNDEFINED);
        sys_hdl->flag_sm1 &= ~(APP_AUDIO_FLAG_SET);
        jtask_stop(sys_hdl->app_auto_con_task);
        jtask_stop(sys_hdl->app_reset_task);
        //BK3000_stop_wdt();
        //aud_PAmute_oper(1);
        //aud_volume_mute(1);
        bt_unit_disable(sys_hdl->unit);
        hci_disable_task(sys_hdl->unit);

        //Delay(100);
        //app_reset();
        bt_unit_enable(sys_hdl->unit);
        app_set_chip_btaddr(app_get_local_btaddr());
	#ifdef CONFIG_TWS
        app_set_role_switch_enable(APP_ROLE_SWITCH_DISABLE);
	#endif
        //bt_flag1_operate(APP_FLAG_AUTO_CONNECTION, 0);
        //sys_hdl->flag_sm1 &= ~APP_FLAG_AUTO_CONNECTION;
        //sys_hdl->flag_sm1 |= APP_FLAG_RECONNCTION|APP_FLAG_AUTO_CONNECTION;
    }
}
#endif
void app_bt_acl_time_out_wrap(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();

    os_printf("acl time out\r\n");
    if(sys_hdl->unit != NULL)
    {
    #ifdef CONFIG_TWS
    #ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
        app_bt_slave_linein_set(0);
        linein_sbc_alloc_free();
    #else
        bt_flag2_operate(APP_FLAG2_STEREO_WORK_MODE, 0);
    #endif	
        //set_tws_prim_sec(TWS_PRIM_SEC_UNDEFINED);
        app_change_mode_flag_clear();
    #endif
        sys_hdl->flag_sm1 &= ~(APP_AUDIO_FLAG_SET);
        jtask_stop(sys_hdl->app_auto_con_task);
        jtask_stop(sys_hdl->app_reset_task);
        //aud_PAmute_oper(1);
	 //aud_volume_mute(1);
        bt_unit_disable(sys_hdl->unit);
        hci_disable_task(sys_hdl->unit);
        Delay(100);
        app_reset();
    #ifdef CONFIG_TWS
        app_set_role_switch_enable(APP_ROLE_SWITCH_DISABLE);
    #endif
        bt_flag1_operate(APP_FLAG_AUTO_CONNECTION, 0);
        //sys_hdl->flag_sm1 &= ~APP_FLAG_AUTO_CONNECTION;
       // sys_hdl->flag_sm1 |= APP_FLAG_RECONNCTION|APP_FLAG_AUTO_CONNECTION;
    }
}

int app_is_conn_be_accepted(void)
{
    int ret;
    app_handle_t sys_hdl = app_get_sys_handler();

    if((sys_hdl->flag_sm1 & APP_SDP_DISABLE_FLAG_SET) == APP_SDP_DISABLE_FLAG_SET)
    {
        ret = -1;
    }
    else
    {
        ret = 0;
    }

    return ret;
}

result_t app_bt_active_conn_profile(uint8 profileId, void *arg)
{
    char cmd[40];
    app_handle_t sys_hdl = app_get_sys_handler();

#ifdef CONFIG_BLUETOOTH_HFP
    app_env_handle_t env_h = app_env_get_handle();
#endif
    
    btaddr_t *remote_btaddr = (btaddr_t *)arg;

    if (NULL == sys_hdl->unit)
    {
        return UWE_NODEV;
    }

    if(NULL == remote_btaddr)
    {
        remote_btaddr = &sys_hdl->remote_btaddr;
    }

    memset(cmd, 0 , 40);
    sprintf(cmd,"%u " BTADDR_FORMAT,
                0,
                remote_btaddr->b[5],
                remote_btaddr->b[4],
                remote_btaddr->b[3],
                remote_btaddr->b[2],
                remote_btaddr->b[1],
                remote_btaddr->b[0]);

    os_printf("active:" BTADDR_FORMAT ",profileId:%d\r\n", BTADDR(remote_btaddr),profileId);

#ifdef CONFIG_BLUETOOTH_A2DP
    if(profileId == PROFILE_ID_A2DP)
    {
        return a2dp_cmd_connect(cmd, sizeof(cmd));
    }
#ifdef CONFIG_TWS
    else if( profileId == PROFILE_ID_A2DP_SRC )
    {
        return a2dp_src_cmd_connect( cmd, sizeof(cmd) );
    }
#endif
    else
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP
    if(profileId == PROFILE_ID_AVRCP)
        return avrcp_cmd_connect(cmd, sizeof(cmd));
#ifdef CONFIG_TWS
    else if( profileId == PROFILE_ID_AVRCP_CT )
        return avrcp_cmd_ct_connect( cmd, sizeof(cmd) );
#endif
    else
#endif
/*temp assign channel to test, may add sdp sender to get channel for connection sooner*/
#ifdef CONFIG_BLUETOOTH_HFP
    if(profileId == PROFILE_ID_HFP)
    {
    #if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
        extern uint32_t hfp_is_connecting_based_on_raddr(btaddr_t *);
        if (hfp_is_connecting_based_on_raddr(remote_btaddr))
    #else
        if (hfp_is_used())   // (g_hfp_hf_app.is_used)
    #endif
        {
            //os_printf("hfp exit1\r\n");
            return UWE_ALREADY;
        }

        return sdp_connect(&env_h->env_cfg.bt_para.device_addr, remote_btaddr);
    }
    else
#endif
#ifdef CONFIG_BLUETOOTH_HSP
    if(profileId == PROFILE_ID_HSP)
    {
        uint32 channel = (uint32)arg;

        if(channel == 0)
            channel = sys_hdl->hfp_rfcomm_channel - 1;
//        remote_btaddr = &sys_hdl->remote_btaddr;

        memset(cmd, 0 , 40);
        sprintf(cmd,"%u " BTADDR_FORMAT " %u",0, remote_btaddr->b[5],
                    remote_btaddr->b[4],remote_btaddr->b[3],
                    remote_btaddr->b[2], remote_btaddr->b[1],
                    remote_btaddr->b[0], channel);
        return hs_cmd_connect(cmd, sizeof(cmd));
    }
    else
#endif
#if defined(CONFIG_BLUETOOTH_HID)
    if((profileId == PROFILE_ID_HID)&&app_env_check_HID_profile_enable())
    {
        return hid_cmd_connect(cmd, sizeof(cmd));
    }
    else
#endif
        return 0;
}

void app_bt_sdp_connect(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();
    btaddr_t *remote_btaddr = &sys_hdl->remote_btaddr;
    btaddr_t *local_btaddr = &env_h->env_cfg.bt_para.device_addr;

    os_printf("app_bt_sdp_connect, local = " BTADDR_FORMAT ", remote = " BTADDR_FORMAT "\r\n", BTADDR(local_btaddr), BTADDR(remote_btaddr));

    sdp_connect(local_btaddr, remote_btaddr);
}

void app_bt_sdp_service_connect(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();
    btaddr_t *remote_btaddr = &sys_hdl->remote_btaddr;
    btaddr_t *local_btaddr = &env_h->env_cfg.bt_para.device_addr;

    os_printf("app_bt_sdp_service_connect, local = " BTADDR_FORMAT ", remote = " BTADDR_FORMAT "\r\n", BTADDR(local_btaddr), BTADDR(remote_btaddr));

    sdp_service_connect(local_btaddr, remote_btaddr, TID_A2DP_PROTOCOL_SSA_REQUEST);
}

void app_bt_sco_enable_wrap(uint8 enable)
{
#ifdef CONFIG_BLUETOOTH_HFP
    aud_mic_open(enable);
#endif
}

void app_bt_key_store_wrap(btaddr_t *raddr, uint8 *key, uint8 key_type)
{
    CLEAR_PWDOWN_TICK;
    CLEAR_SLEEP_TICK;
    if(key_type <= 0x06)   // combination key type used for standard pairing
    {
        app_env_store_autoconn_info(raddr, key);
    }
}


#if(CONFIG_AS_SLAVE_ROLE == 1)
extern void hci_cmd_role_switch(uint8_t role, btaddr_t *raddr);

uint8 Judge_role(btaddr_t *btaddr,bool_t connecting)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    uint8 role; 
    btaddr_t *bt_rmtaddr = NULL;

    if(btaddr == NULL)
    {
        bt_rmtaddr = hci_find_acl_slave_link(sys_hdl->unit);
        if((bt_rmtaddr != NULL) && connecting)
        {
            bt_unit_acl_disconnect(sys_hdl->unit, bt_rmtaddr);
            bt_flag2_operate(APP_FLAG2_RECONN_AFTER_DISCONNEC, 1);
            return 0;
        }        
    }
    role = hci_get_acl_link_role(sys_hdl->unit,btaddr);
    if(!app_check_bt_mode(BT_MODE_1V2)) // not 1v2 config
    {
        if(role == 0) // master
        {
            os_printf("===rmt addr0:%x,"BTADDR_FORMAT"\r\n", BTADDR(btaddr));
            hci_cmd_role_switch(HCI_ROLE_SLAVE,btaddr);
        }
        return 0;
    }
    else              // 1v2 config
    {
        if(hci_get_acl_link_count(sys_hdl->unit) > 1)  // have two links;
        {
            if((role == HCI_ROLE_SLAVE) && connecting) // slave role, disconnect this link and re-connect;
            {
                bt_unit_acl_disconnect(sys_hdl->unit, btaddr);
                bt_flag2_operate(APP_FLAG2_RECONN_AFTER_DISCONNEC, 1);
                if(app_check_bt_mode(BT_MODE_1V1))
                {
                	app_env_handle_t  env_h = app_env_get_handle();
                	btaddr_copy(&env_h->env_data.default_btaddr,&sys_hdl->remote_btaddr);
                	MSG_T msg;
                	msg.id = MSG_ENV_WRITE_ACTION;
                	msg.arg = 0x01;
                	msg.hdl = 0;
                	msg_lush_put(&msg);
                }
                return 1;
            }
            else        // check all links, once has one's link is slave role, then send hci cmd role swtich;
            {
                bt_rmtaddr = hci_find_acl_slave_link(sys_hdl->unit);
                if(bt_rmtaddr != NULL)
                {
                    // hci cmd role switch;
                    os_printf("===rmt addr1:%x,"BTADDR_FORMAT"\r\n", BTADDR(bt_rmtaddr));
                    hci_cmd_role_switch(HCI_ROLE_MASTER,bt_rmtaddr);
                }
                return 0;               
            }
        }
        else                                          // only one link;
        {
            if(role == 0) // master
            {
                os_printf("===rmt addr:%x,"BTADDR_FORMAT"\r\n", BTADDR(btaddr));
                hci_cmd_role_switch(HCI_ROLE_SLAVE,btaddr);
            }  
            return 0;
      
        }
        
    }
}
#endif
#if(CONFIG_AS_SLAVE_ROLE == 0)
uint8 Judge_role(void)
{//
//#if  defined(BT_ONLY_ONE_BT)  ||(!defined(BT_ONE_TO_MULTIPLE) && !defined(CONFIG_TWS))
	if(1 == Remote_BlueDev.init_flag)
	{
		app_handle_t sys_hdl = app_get_sys_handler();
        os_printf("=======Judge_role:%x========\r\n",Remote_BlueDev.init_flag);
        bt_unit_acl_disconnect(sys_hdl->unit, &(Remote_BlueDev.btaddr_def));
        bt_flag2_operate(APP_FLAG2_RECONN_AFTER_DISCONNEC, 1);
        if(app_check_bt_mode(BT_MODE_1V1))
        {
            app_env_handle_t  env_h = app_env_get_handle();
            btaddr_copy(&env_h->env_data.default_btaddr,&sys_hdl->remote_btaddr);
            MSG_T msg;
            msg.id = MSG_ENV_WRITE_ACTION;
            msg.arg = 0x01;
            msg.hdl = 0;
            msg_lush_put(&msg);
        }
        return 1;
	}
	return 0;
}
#endif
void Set_remoteDev_role_btaddr(btaddr_t *btaddr,uint32 role)
{
    memcpy((uint8*)&(Remote_BlueDev.btaddr_def),(uint8*)btaddr,sizeof(btaddr_t));
    Remote_BlueDev.init_flag = role;
    //os_printf("set remote devaddr role:role:%x\r\n",Remote_BlueDev.init_flag);
}

void bt_all_acl_disconnect(hci_unit_t *unit)
{
    btaddr_t *raddr[2] = {NULL};
    int i, count = 0;
    count = hci_get_acl_link_addr(unit, raddr);
    for(i = 0; i < count; i++)
    {
        bt_unit_acl_disconnect(unit, (const btaddr_t *)raddr[i]);
        Delay(100);
    }
}

#if defined(CONFIG_TWS) || (CONFIG_APP_RECONNECT_MATCH == 1)
void app_bt_auconn_stop( void )
{
    app_handle_t app_h = app_get_sys_handler();

    jtask_stop(app_h->app_auto_con_task);
    if( app_h->flag_sm1 & APP_FLAG_AUTO_CONNECTION )
        app_h->flag_sm1 &= ~APP_FLAG_AUTO_CONNECTION;
    if( app_h->flag_sm1 & APP_FLAG_RECONNCTION )
        app_h->flag_sm1 &= ~APP_FLAG_RECONNCTION;

    app_bt_reset_policy_iocap();
}
#endif
#ifdef CONFIG_TWS
char app_bt_update_inquiry_diac_lowest_byte(void) // LIB use this Function
{
    return DIAC_LOWEST_BYTE;
}
void app_write_supervision_timeout(btaddr_t *btaddr, uint16 to) // 8000-5s, 1638-1s, &app_h->stereo_btaddr--tws_slave
{
    app_handle_t app_h = app_get_sys_handler();
    hci_link_t *link = hci_link_lookup_btaddr(app_h->unit, btaddr, HCI_LINK_ACL);

    hci_write_link_supervision_timeout_cp cp;

    if (NULL == link)
    {
        return;
    }

    cp.con_handle = link->hl_handle;
    cp.timeout = to; 
    os_printf("app_write_supervision_timeout(%d)\r\n", get_tws_prim_sec());
    
    unit_send_cmd(app_h->unit, HCI_CMD_WRITE_LINK_SUPERVISION_TIMEOUT, (void *)&cp, sizeof(cp));
}
void app_bt_stereo_pairing_exit(void)
{
    os_printf("app_bt_stereo_pairing_exit()\r\n");
    app_handle_t app_h = app_get_sys_handler();
    app_bt_inquiry(0, 0 , APP_EXIT_PERIODIC_INQUIRY);
    bt_unit_write_inquiry_IAC(app_h->unit, 0);
    bt_flag2_operate(APP_FLAG2_STEREO_CONNECTTING, 0);
    unset_tws_flag(TWS_FLAG_LAUNCH_TWS_INQUIRY_AND_PAGE);
}

#if (CONFIG_TWS_SUPPORT_OLD_OPERATE == 1)
extern u_int32 SYSrand_Get_Rand(void);
void app_bt_stereo_judge_role_again(void)
{
    os_printf("app_bt_stereo_judge_role_again\r\n");

    app_handle_t app_h = app_get_sys_handler();

    if ((SYSrand_Get_Rand()&0x1) || bt_flag1_is_set(APP_FLAG_LINEIN))
    {
        tws_launch_diac_scan();
        jtask_schedule( app_h->app_auto_con_task,7000,(jthread_func)app_button_stereo,(void *)NULL);
    }
    else
    {
        tws_launch_diac_pairing();
        jtask_schedule( app_h->app_auto_con_task,10000,(jthread_func)app_button_stereo,(void *)NULL);
    }
}
#endif

extern void backend_unit_remove_name(void);

void app_bt_inquiry( uint8 seconds, uint8 max_responses, uint8 oper )
{
    app_handle_t app_h = app_get_sys_handler();

    app_h->inquiry_ticks = 0;

    if (APP_LAUNCH_PERIODIC_INQUIRY == oper)
    {
        //unit_send_cmd(app_h->unit, HCI_CMD_INQUIRY_CANCEL, NULL, 0);
        
        backend_unit_remove_name();
        
        hci_periodic_inquiry_cp inq;

        /* Specific Inquiry LAP is 0x9e8b11 */
        inq.lap[0] = DIAC_LOWEST_BYTE;
        inq.lap[1] = 0x8b;
        inq.lap[2] = 0x9e;
        inq.inquiry_length = seconds;//(uint8_t)(((uint32_t)seconds * 100) / 128);
        inq.min_period_length = inq.inquiry_length + 1;
        inq.max_period_length = inq.min_period_length + 1;//inq.inquiry_length;
        /* (N x 1.28) sec */
        inq.num_responses = max_responses;
        unit_send_cmd(app_h->unit, HCI_CMD_PERIODIC_INQUIRY, (void *)&inq, sizeof(inq));
    }
    else if (APP_EXIT_PERIODIC_INQUIRY == oper)
    {
        os_printf("inquiry end\r\n");
        unit_send_cmd(app_h->unit, HCI_CMD_EXIT_PERIODIC_INQUIRY, (void *)NULL, 0);
    }
}

void app_bt_stereo_auto_conn_stop( void )
{
    app_handle_t app_h = app_get_sys_handler();

    jtask_stop( app_h->app_stereo_task );
}

void app_bt_inquiry_active_conn(void *arg)
{
    app_handle_t app_h = app_get_sys_handler();

	if(!app_is_bt_mode()
	#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
		&& !app_is_linein_mode()
	#endif
		)
    {
        return;
    }
#if 0
    if (get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)
        return;
#endif
#if (CONFIG_CHARGE_EN == 1)
    if (bt_flag2_is_set(APP_FLAG2_CHARGE_POWERDOWN)&&(app_env_check_Charge_Mode_PwrDown()||(flag_power_charge==1)))
        return;
#endif

#if 1
    os_printf("app_bt_inquiry_active_conn: 0x%x, 0x%x-0x%x,rcnt:%d\r\n",  
                        bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE),
                        bt_flag2_is_set(APP_FLAG2_STEREO_AUTOCONN),
                        bt_flag2_is_set(APP_FLAG2_STEREO_INQUIRY_RES),retry_count);
#endif	

    if(!bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
    {
        if(bt_flag2_is_set(APP_FLAG2_STEREO_AUTOCONN))
        {
            if(retry_count > (app_env_get_autoconn_max() + 5))
            {
                os_printf("tws auto connect timeout...\r\n");
                retry_count = 0;
                return;
            }
	     app_set_role_switch_enable(APP_ROLE_SWITCH_ENABLE);
            os_printf("active:"BTADDR_FORMAT"\r\n", BTADDR(&app_h->stereo_btaddr));
            retry_count++;
            bt_unit_acl_connect(app_h->unit, &app_h->stereo_btaddr);
            bt_flag2_operate(APP_FLAG2_STEREO_CONNECTTING,  1);
            if (arg)
            {
                jtask_schedule(app_h->app_stereo_task, (uint32)arg, (jthread_func)app_bt_inquiry_active_conn, (void *)arg);
            }
        }
        else
        {
            jtask_stop(app_h->app_stereo_task);
            jtask_schedule(app_h->app_auto_con_task, 1000, (jthread_func)bt_auto_connect_start, (void *)NULL );
        }
    }
}

void app_bt_inquiry_result( struct mbuf *m )
{
#if (CONFIG_TWS_SUPPORT_OLD_OPERATE == 1)
    hci_inquiry_result_ep ep;
    hci_inquiry_result ir;
    app_handle_t app_h = app_get_sys_handler();

    m_copydata(m, 0, sizeof(ep), (void *)&ep);
    m_adj(m, sizeof(ep));

    os_printf("inquiry_result:0x%x, %d\r\n",  
                            bt_flag2_is_set(APP_FLAG2_STEREO_INQUIRY_RES),
                            ep.num_responses);

    if( !bt_flag2_is_set( APP_FLAG2_STEREO_INQUIRY_RES ) && ep.num_responses >= 1 )
    {
        jtask_stop(app_h->app_auto_con_task);
        m_copydata(m, 0, sizeof(ir), (void *)&ir);
        m_adj(m, sizeof(ir));
        os_printf("result: "BTADDR_FORMAT"\r\n", BTADDR(&ir.btaddr));
        memcpy((void *)&app_h->stereo_btaddr, (void *)&ir.btaddr, sizeof(btaddr_t));
        bt_flag2_operate( APP_FLAG2_STEREO_INQUIRY_RES, 1 );
        bt_flag2_operate(APP_FLAG2_STEREO_AUTOCONN, 1);
        app_bt_stereo_pairing_exit();
        app_bt_inquiry_active_conn((void *)5000);
    }
#endif	
}

void app_bt_eir_inquiry_result(struct mbuf *m)
{
#if (CONFIG_TWS_SUPPORT_OLD_OPERATE == 0)
    hci_extended_result_ep ep;
    int i = 0; 
    //os_printf("%s, %d\n", __func__, m->m_pkt_len);
    
    m_copydata(m, 0, sizeof(ep), (void *)&ep);    
    m_adj(m, sizeof(ep));
    
    os_printf("app_bt_eir_inquiry_result:%d\n",  ep.num_responses);

    os_printf(" eir_inquiry_result"BTADDR_FORMAT", uclass=0x%02x %02x %02x, rssi=%d\n\n",
                        BTADDR(&ep.btaddr)
                        , ep.uclass[0] , ep.uclass[1] , ep.uclass[2]
                        , ep.rssi);

	if (bt_flag1_is_set(APP_FLAG_DUT_MODE_ENABLE))
	{
		app_bt_stereo_pairing_exit();
		return;
	}


    for (i = 0; i < HCI_MAX_EXTENDED_INQ_RSP_LEN; )//HCI_MAX_EXTENDED_INQ_RSP_LEN
    {
        if (tws_hci_eir_type_beken(0) == ep.response[i + 1])
        {
            os_printf(">[TWS inquiry:0x%x, "BTADDR_FORMAT"\n\n", bt_flag2_is_set(APP_FLAG2_STEREO_INQUIRY_RES), BTADDR(&ep.btaddr));
            uint8 eir_beken_buff[HCI_EIR_TYPE_BEKEN_SIZE] = {0}, idx = 0;

            for (idx = 0; idx < HCI_EIR_TYPE_BEKEN_SIZE; idx++)
            {
                eir_beken_buff[idx] = ep.response[i + 1 + 1 + idx];
            }

            if (tws_hci_eir_type_beken(1) == eir_beken_buff[2])
            {// launch tws pairing procedure.
                //os_printf(">[TWS inquiry:0x%x, "BTADDR_FORMAT"\n\n", bt_flag2_is_set(APP_FLAG2_STEREO_INQUIRY_RES), BTADDR(&ep.btaddr));
                if(!bt_flag2_is_set(APP_FLAG2_STEREO_INQUIRY_RES))
                {
                    app_handle_t app_h = app_get_sys_handler();
					
                	bt_unit_set_scan_enable(app_h->unit,HCI_NO_SCAN_ENABLE);
                    jtask_stop(app_h->app_auto_con_task);

                    //os_printf("%s, result: "BTADDR_FORMAT"\r\n", __func__, BTADDR(&ir.btaddr));
                    memcpy((void *)&app_h->stereo_btaddr, (void *)&ep.btaddr, sizeof(btaddr_t));
                    bt_flag2_operate(APP_FLAG2_STEREO_INQUIRY_RES, 1);
                    bt_flag2_operate(APP_FLAG2_STEREO_AUTOCONN, 1);
                    #if(CONFIG_TWS_AUTOCONNECT == 1)
                    set_tws_prim_sec(TWS_PRIM_SEC_SECOND);
                    #elif(CONFIG_TWS_KEY_MASTER == 1)
                    set_tws_prim_sec(TWS_PRIM_SEC_PRIMARY);
                    #else
                    set_tws_prim_sec(TWS_PRIM_SEC_SECOND);    
                    #endif
                
                    app_bt_stereo_pairing_exit();
                    app_bt_inquiry_active_conn((void *)5000);
                }
            }
            
            break;
        }
        
        i += ep.response[i] + 1;
    }
#endif
}

void app_bt_inquiry_complete( struct mbuf *m )
{
    app_handle_t app_h = app_get_sys_handler();

    os_printf("app_bt_inquiry_complete()\r\n");

	if(bt_flag2_is_set(APP_FLAG2_STEREO_INQUIRY_RES))
		return;
	//针对inquiry指令可能被auto conn冲掉现象，增加按键响应重新执行机会
	//收到inquiry complete信息，则关掉按键响应重新执行机会
	
    app_h->inquiry_ticks++;
#if (CONFIG_TWS_SUPPORT_OLD_OPERATE == 0)
	if (!bt_flag1_is_set(APP_FLAG_ACL_CONNECTION|APP_FLAG_A2DP_CONNECTION|APP_FLAG_LINEIN)
		&& !bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
	{
		bt_unit_set_scan_enable(app_h->unit, HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
	}	
#endif

#if 0
	jtask_stop(app_h->app_auto_con_task);
#if TWS_CONN_MECHANISM_V2
    //if((get_tws_prim_sec() != TWS_PRIM_SEC_SECOND) || bt_flag1_is_set(APP_FLAG_ACL_CONNECTION))
    if(get_tws_prim_sec() == TWS_PRIM_SEC_UNDEFINED)
    {
     /* If I am  TWS_PRIM_SEC_UNDEFINED or TWS_PRIM_SEC_SECOND that disconnected from the TWS-MASTER, 
        launch the DIAC Inquiry. 
     */
        tws_launch_diac_pairing();
    }
    else
    {
        app_bt_stereo_pairing_exit();
    }
#else
//    app_button_stereo();
	app_bt_stereo_judge_role_again();
#endif
//    if( app_h->inquiry_ticks >= APP_BT_INQUIRY_TICKS )
//    {
//        os_printf("inq ok\r\n");
//        app_bt_inquiry( 0, 0, 1);
//        bt_unit_write_inquiry_IAC( app_h->unit, 0 );
//        //app_bt_inquiry( APP_BT_INQUIRY_DURATION, 1, 0);
//    }
#endif
}

#if (CONFIG_TWS_KEY_MASTER==1)
extern inline u_int8 get_piconet_role(const void *p_bd_addr);
#endif
void app_bt_stereo_con_compl_wrap(hci_link_t *link, const btaddr_t  *rbtaddr)
{
    app_handle_t app_h = app_get_sys_handler();
    hci_write_link_supervision_timeout_cp cp;
    
    if(app_change_mode_flow_ctrl_en())//when change mode, won't support tws connecting..
    {
        os_printf("change mode, won't support tws connecting..\r\n");
        app_bt_stereo_profile_disconn_wrap();
        bt_flag2_operate(APP_FLAG2_STEREO_AUTOCONN, 0);
        bt_flag2_operate(APP_FLAG2_STEREO_CONNECTTING,  0);
        //unset_tws_flag(TWS_FLAG_CONNECTING_SLAVE);
        return ;
    }
    //os_printf("%s, "BTADDR_FORMAT",%d \r\n", __func__, BTADDR(rbtaddr),get_tws_prim_sec());
    //app_bt_stereo_pairing_exit();
    bt_unit_set_scan_enable(app_h->unit, HCI_NO_SCAN_ENABLE);
    bt_flag2_operate(APP_FLAG2_STEREO_WORK_MODE, 1 );
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
    if(bt_flag1_is_set(APP_FLAG_LINEIN))
        linein_audio_close();
#endif

    if((NULL == link) || (NULL == rbtaddr))
    {
        os_printf("app_bt_stereo_con_compl_wrap, error, %p-%p\r\n",  link, rbtaddr);
        return;
    }

    CLEAR_PWDOWN_TICK;

    app_h->flag_sm1 |= APP_FLAG_ACL_CONNECTION;

    memcpy((uint8 *)&app_h->stereo_btaddr, (uint8 *)rbtaddr, sizeof(btaddr_t));
	
#if 1//CONFIG_TWS_AUTOCONNECT
    app_bt_stereo_pairing_exit();
#endif

    cp.con_handle = link->hl_handle;
    cp.timeout = 8000; // 5s
    if (TWS_PRIM_SEC_SECOND == get_tws_prim_sec())
    {
        cp.timeout = 8000 * 5; // 5s * 5  //1638; // 1s
    }
    os_printf("app_bt_stereo_con_compl_wrap:%d,%d, addr:"BTADDR_FORMAT"\r\n",get_tws_prim_sec(), cp.timeout,  BTADDR(rbtaddr));

    unit_send_cmd(app_h->unit, HCI_CMD_WRITE_LINK_SUPERVISION_TIMEOUT, (void *)&cp, sizeof(cp));

#ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    bt_sniff_alloc_st(1, link->hl_handle, &app_h->stereo_btaddr);
#endif

#if (CONFIG_TWS_KEY_MASTER==1)
    os_printf(">[TWS:role:0x%08x,0x%08x,%d\r\n",bt_flag2_is_set(0xffffffff),bt_flag1_is_set(0xffffffff),get_piconet_role(&app_h->stereo_btaddr));
    /*
        the first TWS connect,the inquiry role is TWS_PRIM_SEC_PRIMARY,
        and the inquiry scan role is TWS_PRIM_SEC_SECOND 
    */
    if (bt_flag2_is_set(APP_FLAG2_STEREO_INQUIRY_RES))
    {
        bt_flag2_operate(APP_FLAG2_STEREO_AUTOCONN, 0);
        set_tws_prim_sec(TWS_PRIM_SEC_PRIMARY);
    }
    else
    {
        if(get_tws_prim_sec() == TWS_PRIM_SEC_UNDEFINED)
            set_tws_prim_sec(TWS_PRIM_SEC_SECOND);
        //Last as TWS-MASTER.
        /*
        if(get_tws_env_stereo_role() == TWS_PRIM_SEC_PRIMARY) 
        {
            set_tws_prim_sec(TWS_PRIM_SEC_PRIMARY);        
        }
        else
        {
            set_tws_prim_sec(TWS_PRIM_SEC_SECOND);
        }  
        */
    }
    /*
        When the bt device power on,it will check link keys of env_data.

        If it has two connections history,one is TWS_PRIM_SEC_PRIMARY,and the other is mobile phone;
        This device will set APP_FLAG_AUTO_CONNECTION flag,and request connection to the mobile;
        So,we set this device as TWS_PRIM_SEC_PRIMARY role.

        If it has only the stereo addr and link key,this device must be TWS_PRIM_SEC_SECOND,and will
        set APP_FLAG2_STEREO_AUTOCONN flag;
        So,we set this device as TWS_PRIM_SEC_SECOND role.
    */
#if 0
    if(bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION))
        set_tws_prim_sec(TWS_PRIM_SEC_PRIMARY);
    else
    {
        /* if mobile phone has been connected by the device, it should be TWS_PRIM_SEC_PRIMARY */
        if(bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION|APP_FLAG_HFP_CONNECTION)) 
            set_tws_prim_sec(TWS_PRIM_SEC_PRIMARY);
    }
 	if (!bt_flag2_is_set(APP_FLAG2_STEREO_AUTOCONN))
       	set_tws_prim_sec(TWS_PRIM_SEC_PRIMARY);
#endif
        


#else
 	if (bt_flag2_is_set(APP_FLAG2_STEREO_AUTOCONN))
       	set_tws_prim_sec(TWS_PRIM_SEC_SECOND);
    else
        set_tws_prim_sec(TWS_PRIM_SEC_PRIMARY);	  
#endif
	
    if(get_tws_prim_sec() == TWS_PRIM_SEC_SECOND)
    {
        app_h->link_handle = link->hl_handle;
        //memcpy( (uint8 *)&app_h->remote_btaddr, (uint8 *)rbtaddr, sizeof(btaddr_t));
        app_bt_auconn_stop();
        bt_flag2_operate(APP_FLAG2_STEREO_WORK_MODE, 1 );
    }
    else
    {
        //app_bt_active_conn_profile(PROFILE_ID_A2DP_SRC, (void *)&app_h->stereo_btaddr);
        set_tws_flag(TWS_FLAG_CONNECTING_SLAVE);
    }
}


/* Only for TWS-MASTER stereo A2DP connected call_back. */
void app_bt_stereo_profile_conn_wrap( void )
{
    app_handle_t app_h = app_get_sys_handler();
	
    os_printf("app_bt_stereo_profile_conn_wrap, "BTADDR_FORMAT" \r\n",  BTADDR(&app_h->stereo_btaddr));
	
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
	//if(bt_flag1_is_set(APP_FLAG_LINEIN))
	//	sdadc_enable(0);
#endif
    //jtask_stop( app_h->app_auto_con_task );
    app_bt_stereo_auto_conn_stop();
    app_bt_stereo_pairing_exit();
    bt_unit_set_scan_enable(app_h->unit, HCI_NO_SCAN_ENABLE);
    bt_flag2_operate(APP_FLAG2_STEREO_WORK_MODE, 1);
    bt_flag2_operate(APP_FLAG2_MATCH_BUTTON|APP_FLAG2_STEREO_MATCH_BUTTON|APP_FLAG2_STEREO_AUTOCONN_RETRY, 0);
    bt_flag2_operate(APP_FLAG2_STEREO_BUTTON_PRESS|APP_FLAG2_STEREO_CONNECTTING|APP_FLAG2_STEREO_AUTOCONN|APP_FLAG2_STEREO_INQUIRY_RES, 0);		
    if(get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)
    {
        hci_link_t *tmp_link = NULL;
        hci_write_default_pkt_type_cp cp;
    	tmp_link = hci_link_lookup_btaddr(app_h->unit, &(app_h->stereo_btaddr), HCI_LINK_ACL);
        cp.pswd = 0x9527;
        cp.handle = tmp_link->hl_handle;
        cp.pkt_type = 1;        // poll pkt;
        cp.poll_interval = 40;  // 40 * 0.625
        hci_link_set_default_pkt_type(&cp);
    }		
#ifndef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE	
    if(SYS_WM_BT_MODE != app_h->sys_work_mode)
    {
        app_bt_stereo_profile_disconn_wrap();
    }
    else
#endif	
        app_wave_file_play_start(APP_WAVE_FILE_ID_STEREO_CONN);		
}

extern void LMinq_Inquiry_clear_device_scan_state(void);
void app_bt_stereo_profile_conn_option(void *arg)
{
    MSG_T msg;
    app_handle_t app_h = app_get_sys_handler();
    os_printf("app_bt_stereo_profile_conn_option()\r\n");

    msg.id  = MSG_ENV_WRITE_ACTION;
    msg.arg = 0x01;
    msg.hdl = 0;
    msg_lush_put(&msg);

	if(!bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION|APP_FLAG_HFP_CONNECTION)
		&&(SYS_WM_BT_MODE == app_h->sys_work_mode)
	)
    {
		app_set_role_switch_enable(APP_ROLE_SWITCH_DISABLE);
        app_bt_check_inquiry_set(0);
		LMinq_Inquiry_clear_device_scan_state();
        if (app_get_env_key_num())
        {
            bt_flag1_operate(APP_FLAG_AUTO_CONNECTION, 1);
            bt_auto_connect_start();
        }
    }	
    else
    {
        app_set_led_event_action(LED_EVENT_CONN);
    }
#ifndef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE	
    if (SYS_WM_BT_MODE != app_h->sys_work_mode)
    {
        app_bt_stereo_profile_disconn_wrap();
    }
#endif	
}

/* Only for TWS-MASTER stereo A2DP disconnected call_back. */
void app_bt_stereo_profile_disconn_wrap(void)
{   
    app_handle_t app_h = app_get_sys_handler();
    os_printf("app_bt_stereo_profile_disconn_wrap()\r\n");

    avrcp_cmd_ct_disconnect();
    bt_unit_acl_disconnect(app_h->unit, &app_h->stereo_btaddr);
    unset_tws_flag(TWS_FLAG_CONNECTING_SLAVE);
    unset_tws_flag(TWS_FLAG_MASTER_STARTED_RE_SYNC);
    unset_tws_flag(TWS_FLAG_MASTER_SLAVE_A2DP_STREAM_CONNECTED);
#if 0//TWS_CONN_MECHANISM_V2
    if((get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)
        && bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION))  // TWS_Master still connected with the phone.
    {
    	// Open the scan in order to let TWS-SLAVE connecting it.
        tws_launch_diac_scan();
    }
#endif
}
void bt_unit_twscontrol_scan(uint8 enable)	
{
    app_handle_t app_h = app_get_sys_handler();

    if(bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE)&&(get_tws_prim_sec()==TWS_PRIM_SEC_SECOND))
    {
        retry_count = 0; 
    }

    os_printf("bt_unit_twscontrol_scan %d\r\n",enable);

    if ((HCI_NO_SCAN_ENABLE == enable) 
	#if (CONFIG_TWS_AUTOCONNECT==0)
		|| app_bt_stereo_addr_env_empty()
	#endif	
		)
    {
        bt_unit_set_scan_enable(app_h->unit, HCI_NO_SCAN_ENABLE);
    }
/************************************
    else if (bt_flag1_is_set(APP_FLAG_A2DP_CONNECTION)
        && avrcp_is_connection())
****************************************/
    else
    {
    #if CONFIG_TWS_AUTOCONNECT
        if(!bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
        {
            bt_unit_set_scan_enable(app_h->unit, HCI_PAGE_SCAN_ENABLE|HCI_INQUIRY_SCAN_ENABLE);
        }
        else
    #endif
        {
            bt_unit_set_scan_enable(app_h->unit, HCI_PAGE_SCAN_ENABLE);
        }
    }
}
#endif

#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
/* Link flow clrl by handle,where:
 * <flow_ctrl> = 0: no flow control;
 * <flow_ctrl> = 1(bit0): Controller flow control,saved air bandwith;
 * <flow_ctrl> = 2(bit1): Host layer flow control,saved sbc node buffer;
 */
void bt_set_ag_flow_ctrl_by_handle(hci_link_t *link)
{
	uint8 tc_cmd[8] = {0x01,0xe0,0xfc,0x04,0x83,0x00,0x00,0x01};
	tc_cmd[5] =  (uint8)(link->hl_handle & 0xff);
	tc_cmd[6] =  (uint8)((link->hl_handle>>8) & 0xff);
	tc_cmd[7] = (link->flow_ctrl & 0x03);
	uart_send_poll(tc_cmd,sizeof(tc_cmd));
    jtask_schedule(link->hl_expire, 500, bt_set_ag_flow_ctrl_timeout, link);
}
void bt_set_ag_flow_ctrl_timeout(void *arg)
{
    hci_link_t *link = (hci_link_t *)arg;
    bt_set_ag_flow_ctrl_by_handle(link);
}
#endif

void bt_exchange_hf_active_by_handle(uint16_t handle)
{
	uint8 tc_cmd[7] = {0x01,0xe0,0xfc,0x03,0x84,0x00,0x00};
	tc_cmd[5] =  (uint8)(handle & 0xff);
	tc_cmd[6] =  (uint8)((handle>>8) & 0xff);
	uart_send_poll(tc_cmd,sizeof(tc_cmd));
}
#if defined(CONFIG_TWS)&&defined(CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE)
void app_sniff_debug(void)
{
    uint8_t idx = 0;
    for(idx=0;idx<NEED_SNIFF_DEVICE_COUNT;idx++)
    {
        os_printf("idx:%d,bt_addr:"BTADDR_FORMAT"\r\n",idx,BTADDR(&(g_sniff_st[idx].remote_btaddr)));
        os_printf("used:%d,active:%d,policy:%d,hd:%p\r\n",g_sniff_st[idx].is_used,g_sniff_st[idx].is_active,g_sniff_st[idx].is_policy,g_sniff_st[idx].link_handle);
    }
}
#endif

void app_bt_pinkey_missing(btaddr_t* addr)
{
    app_env_handle_t  env_h = app_env_get_handle();
    app_handle_t sys_hdl = app_get_sys_handler();

    os_printf("pin or key missing!Won't reconnect again!\r\n");
#ifdef CONFIG_TWS
    if (btaddr_same(addr, (const btaddr_t *)app_bt_get_handle(2)))
    {
        bt_unit_delete_stored_link_key(sys_hdl->unit, addr);
        app_env_keypair_stereo_delete(addr);  
    }  
    else
#endif
    {
        bt_unit_delete_stored_link_key(sys_hdl->unit, NULL);
        app_env_keypair_used_delete(addr);
        memcpy( (uint8 *)&env_h->env_data.default_btaddr, (uint8 *)addr, sizeof(btaddr_t));
    }

#ifdef BT_ONE_TO_MULTIPLE
    MSG_T msg;
#ifdef BT_ONLY_ONE_BT
    msg.id = MSG_ENV_WRITE_ACTION;
    msg.arg = 0x00;
    msg.hdl = 0;
    msg_lush_put(&msg);
#else
{
    int ret = 0;
    ret = app_env_write_action(&(env_h->env_data.default_btaddr),2); // 2 means write flash whatever
    if(ret)
    {
        msg.id = MSG_ENV_WRITE_ACTION;
        msg.arg = 0x00;
        msg.hdl = 0;
        msg_lush_put(&msg);
    }
}
#endif
#endif
}

//EOF
