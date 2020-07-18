/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include <jos.h>
#include <jos/jos_mbuf.h>
#include "bt_sco_backend_utils.h"
#include <bt_sco.h>
#include <audio_out_interface.h>
#include "app_beken_includes.h"
#include "beken_external.h"
#include "hw_lc.h"
#include "bt_hfp_hf.h"

#define SCO_BANDWIDTH        8000
#define SCO_WIDEBANDWIDTH    16000
#define SCO_MAX_LATENCY      0xFFFF // older value = 7,esco link/2EV3 packet not success!!!  ,>=0x0A
#define SCO_SAMPLE_SIZE      HCI_PCM_SAMPLE_SIZE_16BIT
#if (CONFIG_DEBUG_PCM_TO_UART == 1)
extern void app_print_linkkey_to_uart(void);
extern void uart_send_ppp(unsigned char *buff, unsigned char fid,unsigned short len);
extern void *TC_Get_Local_Baseband_Monitors(uint16_t *len);
#endif
/*
重复声明
#ifdef BT_ONE_TO_MULTIPLE
extern void set_hf_flag_1toN(btaddr_t *btaddr,uint32 flag);
extern void clear_hf_flag_1toN(btaddr_t *btaddr,uint32 flag);
#endif

void hf_volume_init( uint8_t aud_volume);
int8_t get_current_hfp_volume(void);

*/

extern void set_voice_recog_status(boolean status);

#if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
uint32_t hfp_all_apps_is_unused(void);
#endif
typedef struct {
    sco_h    ep;
    jtask_h  send_out_task;
    bool_t   connected;
    void *audio_out;
} sco_utils_t;

btaddr_t *bt_raddr = NULL;

static void sco_utils_newconn(void *app_ctx, sco_link_type_t link_type,
                              uint16_t pkt_type)
{
    sco_utils_t *ctx = (sco_utils_t *)app_ctx;
    sco_params_t sco_params;
    result_t rc;
    os_printf("sco_utils_newconn\r\n");

    sco_params.tx_bandwidth = SCO_BANDWIDTH;
    sco_params.rx_bandwidth = SCO_BANDWIDTH;
    sco_params.max_latency = SCO_MAX_LATENCY;
    sco_params.retransmit_effort = SCO_RETRANSMIT_EFFORT_LINK_QUALITY;

    /* Currently, application imposes no restrictions on the SCO packet type.
     * The actual packet type will be negotiated between the local controller
     * and the remote entity.
     */
    sco_params.pkt_type = HCI_PKT_ESCO_ALL;

    /* 2. Voice settings */
    sco_params.pcm_settings.input_coding = HCI_PCM_INPUT_CODING_LINEAR;
    sco_params.pcm_settings.input_format = HCI_PCM_INPUT_FORMAT_2COMP;
    sco_params.pcm_settings.input_sample_size = SCO_SAMPLE_SIZE;
    sco_params.pcm_settings.linear_bit_position = 0;
#if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
{
    hfp_hf_app_t *hfp_app_ptr = NULL;
    hfp_app_ptr = (hfp_hf_app_t *)hfp_get_app_from_priv_ext((sco_utils_h)app_ctx);
    if(hfp_app_ptr->freq == 16000)  /* mSBC */
    {
        sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_TRANSPARENT;
    }
    else
    {
        sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_CVSD;
    }
}
#else
{
	if(get_current_hfp_freq() == 16000) /* mSBC */
    {
        sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_TRANSPARENT;
    }
    else
    {
        sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_CVSD;
    }
}
#endif

    rc = sco_accept(ctx->ep, link_type, &sco_params);

#if 0 // move to hci_sco_newconn	
    if(a2dp_has_connection())
        rc = sco_accept(ctx->ep, link_type, &sco_params);
    else
        rc = sco_reject(ctx->ep);
#endif	
    DBG_RC_I(rc, DBT_TEST, ("%s: done, %s\n", FNAME, uwe_str(rc)));
    (void)rc;
}

#ifdef ADC_FIRST_DELAY
extern uint16 adc_first_discard_cnt;
extern uint8 adc_first_discard_flag;
#endif
extern int hfp_current_is_calling(void);
static void sco_utils_connected(void *app_ctx)
{
    result_t rc = 0;
    sco_utils_t *ctx = (sco_utils_t *)app_ctx;
    int8_t volume;
    uint32_t freq;
    app_env_handle_t env_h = app_env_get_handle();
#if CONFIG_TWS_AUTOCONNECT
	app_handle_t app_h = app_get_sys_handler();
	bt_unit_set_scan_enable(app_h->unit, HCI_NO_SCAN_ENABLE);
#endif

    jtask_stop(ctx->send_out_task);
    
    bt_raddr = get_pcb_raddr(ctx->ep);
#ifdef CONFIG_BLUETOOTH_COEXIST
    app_coexist_play_pause(0);
#endif
	
#if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
{
    /*
    void *hfp_app_ptr = NULL;
    hfp_app_ptr = hfp_get_app_from_priv_ext((sco_utils_h)app_ctx);
	if(hfp_app_ptr)
		bt_select_current_hfp(hfp_app_ptr);     //ensure safe while the AT cmd come after sco_connected
    */
    set_hf_flag_1toN(bt_raddr,APP_FLAG_SCO_CONNECTION);
#if ( BT_MODE==BT_MODE_1V2)
    if(!get_2hfps_sco_and_incoming_flag())
#endif
        hf_exchange_sco_active_flag(bt_raddr,SCO_CONNECTED);
#if ( BT_MODE==BT_MODE_1V2)
    check_2hfps_sco_and_incoming_status();

    if(bt_flag1_is_set(APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
        bt_flag2_operate(APP_FLAG2_RECONN_AFTER_CALL,1);
#endif
}
#else
{
    set_current_hfp_flag(APP_FLAG_SCO_CONNECTION, 1);    
}
#endif  
 
#if 1
    sbc_mem_free();
#endif

#if TWS_HFP_ENABLE
    app_wave_file_play_clean();
#endif

    if (!bt_flag1_is_set(APP_FLAG_SCO_CONNECTION))
    {
        bt_flag1_operate(APP_FLAG_SCO_CONNECTION, 1);
	#ifdef CONFIG_APP_AEC
		app_env_rf_pwr_set(1);
		if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_AEC_ENABLE))
		{
			app_aec_init(get_current_hfp_freq());
		}
	#endif
    }
    
    volume = get_current_hfp_volume();
    freq = get_current_hfp_freq();
#ifdef CONFIG_TWS
    if (get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)  
    {/* Let the TWS-SLAVE stoping the music immediately. */
        tws_set_call_exist(TRUE);
        
        #if MASTER_DECODE_MSBC_IN_TASK
        tws_hfp_msbc_decode_init();
        #endif

        #if TWS_HFP_ENABLE
        send_sco_connected_to_tws_slave((freq > 8000));
        #endif
    }    
#endif
    os_printf("sco_utils_connected:%d,%d\r\n",volume,freq);
    //aud_PAmute_oper(1);

    if(hfp_has_connection())
    {
    #if ( BT_MODE==BT_MODE_1V2)
        if(get_2hfps_sco_and_incoming_flag())
        {
        #ifdef CONFIG_DRIVER_I2S
            app_wave_file_aud_notify(48000, 1, volume);
        #else
            app_wave_file_aud_notify(freq, 1, volume);
        #endif
        }
        else
    #endif
        {        
            if(!bt_flag1_is_set(APP_FLAG_WAVE_PLAYING))
            {
            	aud_PAmute_oper(1);
                aud_close();
            #ifdef CONFIG_DRIVER_I2S
                rc = aud_initial(48000, 1, 16);
                aud_open();
            #else
                rc = aud_initial(freq, 1, 16);
            #endif
                if(rc)
                    goto Exit;
                //aud_mic_open(1);
                aud_open();
            }

        #ifdef CONFIG_DRIVER_I2S
            app_wave_file_aud_notify(48000, 1, volume);
        #else
            app_wave_file_aud_notify(freq, 1, volume);
        #endif
            //app_button_type_set(BUTTON_TYPE_HFP);
        #if TWS_SCO_AVOID_POP_ENABLE
            tws_sco_avoid_pop_sound();            
        #else
            hf_volume_init(volume);
        #endif

        #ifdef CONFIG_APP_HALFDUPLEX
            app_hfp_echo_erase_init();
        #endif

        #ifdef ADC_FIRST_DELAY
            adc_first_discard_flag = 0; //clear the flag
            adc_first_discard_cnt = 0;  //clear the count
        #endif
            app_bt_sco_enable_wrap(1);
        }
    }
    else if(bt_flag1_is_set(APP_FLAG_HSP_CONNECTION))
    {
        if(!bt_flag1_is_set(APP_FLAG_WAVE_PLAYING))
        {
        #ifdef CONFIG_DRIVER_I2S
            rc = aud_initial(48000, 1, 16);
        #else
                rc = aud_initial(8000, 1, 16);
        #endif
            if(rc)
                goto Exit;
        }

    #ifdef CONFIG_DRIVER_I2S
        app_wave_file_aud_notify(48000, 1,
                                 env_h->env_cfg.system_para.vol_hfp);
    #else
        app_wave_file_aud_notify(freq, 1,
                                 env_h->env_cfg.system_para.vol_hfp);
    #endif
    }

    ctx->connected = 1;

    
    #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    #if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
        uint16_t handle;
        handle = bt_sniff_get_handle_from_raddr(bt_raddr);
        app_bt_write_sniff_link_policy(handle, 0);
    #endif
    #endif
    
 Exit:
    /* if only sco connected, but no any at cmd and at result code,we don't set flags about hfp */
    /**
    if((!hfp_current_is_calling())&&(!bt_flag1_is_set(APP_FLAG_HFP_OUTGOING)) &&(!bt_flag2_is_set(APP_FLAG2_HFP_INCOMING))) // BlueSoleil软件/ 蓝牙测试盒
    {
        bt_flag1_operate(APP_FLAG_HFP_CALLSETUP, 1);
    }
    **/
    DBG_RC_I(rc, DBT_SCO, ("%s: done %s\n", FNAME, uwe_str(rc)));

#ifdef NO_SCAN_WHEN_WORKING
    app_handle_t sys_hdl = app_get_sys_handler();
    if(hci_get_acl_link_count(sys_hdl->unit) < BT_MAX_AG_COUNT)
        bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);
#endif

#ifdef BT_ONE_TO_MULTIPLE
    /* if an incoming call online,but not this hfp, we need to change the curent_hfp to another for accept or reject */

    /*
    if(has_hfp_flag_1toN(APP_FLAG2_HFP_INCOMING ) && (!get_hfp_flag_1toN(bt_raddr,APP_FLAG2_HFP_INCOMING)))
    {
        os_printf("===hfp curr chg-1\r\n");
    	change_cur_hfp_to_another_incoming_call(bt_raddr);
    }
    */
#endif
#if(CONFIG_AUD_FADE_IN_OUT == 1)
    if(!bt_flag1_is_set(APP_FLAG_WAVE_PLAYING))
        set_aud_fade_in_out_state(AUD_FADE_IN);
#endif

#if 0//TWS_HFP_ENABLE
    if(get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)  
    {
        aud_open();
    }    
#endif


}
#if TWS_HFP_ENABLE
void send_sco_connected_to_tws_slave(uint8 freq)
{
    uint8_t buff1[2] = {0};
    buff1[0] = TWS_HFP_CMD_SCO_CONNECTED;
    buff1[1] = freq;  // 0 = 8KHz,1 = 16KHz;
    tws_hfp_send_cmd(buff1, 2);
    
    tws_msbc_decode_prapare();
}

void send_sco_disconnted_to_tws_slave(void)
{
    //uint8_t buff[1] = {0};
    //tansparent_l2cap_send_data_in_controller(buff, 1, DATA5_TWS_M_SCO_DISCONN);
    uint8_t buff1[1] = {TWS_HFP_CMD_SCO_DISCONNECTED};
    tws_hfp_send_cmd(buff1, 1);
}
#endif
#ifdef SBC_FIRST_DISCARD_ENABLE 
extern uint8 sbc_first_discrad_flag;
extern uint8 sbc_first_discard_count;
extern uint8 sbc_first_discard_data;
#endif
static void sco_utils_disconnected(void *app_ctx, result_t err)
{
    app_handle_t app_h = app_get_sys_handler();

#if CONFIG_TWS_AUTOCONNECT
	if(!bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
		bt_unit_set_scan_enable(app_h->unit, HCI_PAGE_SCAN_ENABLE|HCI_INQUIRY_SCAN_ENABLE);
#endif	

    app_h->mic_volume_store &= ~0x80;

    set_voice_recog_status(FALSE);//here should set voice_recog_status FALSE.

    sco_utils_t *ctx = (sco_utils_t *)app_ctx;
    jtask_stop(ctx->send_out_task);
    app_bt_sco_enable_wrap(0);

    os_printf("sco_utils_disconnected\r\n");

    if(bt_flag1_is_set(APP_FLAG_HSP_CONNECTION))
    {
    }
    ctx->audio_out = NULL;
    ctx->connected = 0;

	set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE,0);
    //get_cur_hfp_raddr(&raddr);
#if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
    btaddr_t *raddr;
    raddr = get_pcb_raddr(ctx->ep);
    hf_exchange_sco_active_flag(raddr,SCO_DISCONNECTED);
    clear_hf_flag_1toN(raddr,APP_FLAG_SCO_CONNECTION);
#if ( BT_MODE==BT_MODE_1V2)
    set_2hfps_sco_and_incoming_flag(0);
#endif
#else
    set_current_hfp_flag(APP_FLAG_SCO_CONNECTION, 0);
    bt_raddr = NULL;
#endif

#if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
    if(!hfp_has_sco_conn())
#endif
    {
        bt_flag1_operate(APP_FLAG_SCO_CONNECTION, 0);
        /* app_button_type_set(BUTTON_TYPE_NON_HFP); ?????????????*/
    #ifdef CONFIG_APP_AEC
            app_env_rf_pwr_set(0);
            if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_AEC_ENABLE))
                app_aec_uninit();
    #endif
    }

    if(!(a2dp_has_music()))
    {

        if(!bt_flag1_is_set(APP_FLAG_WAVE_PLAYING)
        #if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)       
        	&&!hfp_has_sco_conn()
        #endif
        	)
        {
            aud_PAmute_oper(1);
            aud_volume_mute(1);
            aud_close();
        }
        else
        {
        #if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
            if(hfp_has_sco_conn())
            {
             	//change_cur_hfp_to_another_sco_conn(raddr);	
            #ifdef CONFIG_DRIVER_I2S
                app_wave_file_aud_notify(48000, 1,get_current_hfp_volume());
            #else
                app_wave_file_aud_notify(8000, 1,get_current_hfp_volume());
            #endif
             }
        #endif
             app_audio_restore();
        }

    }
    else
    {
        if(!bt_flag1_is_set(APP_FLAG_WAVE_PLAYING))
        {
            /*avoid the "pop" due to the flow ctrl recover(2DH-1->2DH-5) */
        #ifdef SBC_FIRST_DISCARD_ENABLE 
            sbc_first_discrad_flag = 1;
            sbc_first_discard_count = 0;
            sbc_first_discard_data = 3; //empirical value by measured
        #endif
            aud_PAmute_oper(1);
            app_audio_restore();
        }
        else
        {
            aud_PAmute_oper(1);
            if(bt_flag1_is_set(APP_FLAG_SCO_CONNECTION))
                hf_audio_handler(0);
            else
                a2dp_audio_handler(0);
		    aud_open();
        }
    }

#if (CONFIG_DEBUG_PCM_TO_UART == 1)
    uint16_t len;
    uint8_t *ptr = NULL;
    ptr = TC_Get_Local_Baseband_Monitors(&len);
    uart_send_ppp(ptr,0x20,len);
    app_print_linkkey_to_uart();
#endif

#ifdef BT_ONE_TO_MULTIPLE
#ifdef NO_SCAN_WHEN_WORKING
    app_handle_t sys_hdl = app_get_sys_handler();
    if(hci_get_acl_link_count(sys_hdl->unit) < BT_MAX_AG_COUNT)
    {
        if(app_env_check_inquiry_always())
            bt_unit_set_scan_enable(sys_hdl->unit,
        				HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);
        else
            bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
    }
#endif
#if (BT_MODE==BT_MODE_1V2)
    if(app_check_bt_mode(BT_MODE_1V2))
        app_bt_reconn_after_callEnd();
#endif
#endif
#ifdef CONFIG_TWS
    if(get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)  
    {
        if (!bt_flag1_is_set(APP_FLAG_CALL_ESTABLISHED))
        {
            tws_set_call_exist(FALSE);
        }
    #if TWS_HFP_ENABLE
        if(get_tws_flag(TWS_FLAG_W4_SLAVE_CONNECT))
        {
            unset_tws_flag(TWS_FLAG_W4_SLAVE_CONNECT);
            set_tws_hfp_flag(TWS_HFP_FLAG_W4_SCO_CONNECT);
            bt_unit_set_scan_enable(app_h->unit, HCI_PAGE_SCAN_ENABLE);
            jtask_schedule(app_h->tws_slave_check_task, TWS_W4_SLAVE_CONNECT_TIMEOUT, (jthread_func)tws_hfp_w4_slave_reconnect, NULL);
        }
        send_sco_disconnted_to_tws_slave();
    #endif

	#if MASTER_DECODE_MSBC_IN_TASK
        tws_hfp_msbc_decode_deinit();
	#endif

    }
#endif    

#if TWS_HFP_ENABLE
    tws_hfp_set_msbc_cnt(0);
	hf_cmd_cind_read();
#endif

#ifdef CONFIG_BLUETOOTH_COEXIST
    if(!bt_flag1_is_set(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
        app_coexist_play_pause(1);
#endif
}

static void sco_utils_complete(void *app_ctx, int32_t count, void *arg)
{
}

static void sco_utils_input(void *app_ctx, struct mbuf *m)
{
    result_t rc=0;
    uint32_t length = m_length(m);
    sco_utils_t *ctx = (sco_utils_t *)app_ctx;
    DECLARE_FNAME("sco_utils_input");

    if(bt_flag1_is_set(APP_FLAG_HFP_CALLSETUP))
        rc = audio_out_write(ctx->audio_out, mtod(m, uint8_t *), length);
    DBG_RC_I(rc, DBT_SCO, ("%s: done, %s\n", FNAME, uwe_str(rc)));
    (void)rc;
}

static scoproto_t sco_utils_cbs = {
    sco_utils_connected,
    sco_utils_disconnected,
    sco_utils_newconn,
    sco_utils_complete,
    sco_utils_input
};

void util_sco_close(sco_utils_h priv)
{
    sco_utils_t *ctx = (sco_utils_t *)priv;

    if(!priv)
    {
        return;
    }

    if(ctx->audio_out)
    {
        audio_out_close(&ctx->audio_out);
    }

    if(ctx->ep)
    {
        sco_close(ctx->ep);
    }

    if(ctx->send_out_task)
    {
        jtask_uninit(ctx->send_out_task);
        ctx->send_out_task = NULL;
    }

    if (ctx)
    {
        jfree(ctx);
        ctx = NULL;
    }
}

result_t util_sco_open(const btaddr_t *laddr, const btaddr_t *raddr,
                       sco_utils_h *priv)
{
    result_t rc;
    sco_utils_t *ctx = NULL;

    ctx = jmalloc(sizeof(sco_utils_t), M_ZERO);
    rc = jtask_init(&ctx->send_out_task, J_TASK_PNP);
    if(rc)
        goto Error;

    rc = sco_open(laddr, raddr, &sco_utils_cbs, ctx, &ctx->ep);
    if(rc)
        goto Error;

    *priv = ctx;

    return UWE_OK;

 Error:
    util_sco_close(ctx);
    return rc;
}

result_t util_sco_connect(sco_utils_h priv)
{
    sco_params_t sco_params;
    sco_utils_t *ctx = (sco_utils_t *)priv;
    int ret = 0;
    sco_params.tx_bandwidth = SCO_BANDWIDTH;
    sco_params.rx_bandwidth = SCO_BANDWIDTH;
    sco_params.max_latency = SCO_MAX_LATENCY;
    sco_params.retransmit_effort = SCO_RETRANSMIT_EFFORT_LINK_QUALITY;

    sco_params.pkt_type = HCI_PKT_ESCO_ALL;

    sco_params.pcm_settings.input_coding = HCI_PCM_INPUT_CODING_LINEAR;
    sco_params.pcm_settings.input_format = HCI_PCM_INPUT_FORMAT_2COMP;
    sco_params.pcm_settings.input_sample_size = SCO_SAMPLE_SIZE;
    sco_params.pcm_settings.linear_bit_position = 0;
#if (CONFIG_HFP17_MSBC_SUPPORTED == 1)
    /* Reference to HFP spec 1.7.0, @Page.40 */

    /********************************************
    For all HF initiated audio connection establishments for which both sides
    support the Codec Negotiation feature, the HF shall trigger the AG to establish
    a Codec Connection. This is necessary because only the AG knows about the codec
    selection and settings of the network.
    *********************************************/
    /*********************************************
    AT+BCC (Bluetooth Codec Connection)
    Syntax: AT+BCC
    Description:
    This command is used by the HF to request the AG to start the codec connection procedure.
    *********************************************/

    #if defined(BT_ONE_TO_MULTIPLE)||defined(CONFIG_TWS)
    {
        hfp_hf_app_t *hfp_app_ptr = NULL;
        hfp_app_ptr =(hfp_hf_app_t *) hfp_get_app_from_priv_ext((sco_utils_h)ctx);
        if(hf_check_flag(hfp_app_ptr,APP_HFP_PRIVATE_FLAG_CODEC_NEGOTIATION))
        {
            ret = bt_hfp_hf_establishment_codec(hfp_app_ptr->session);
        }
        else
        {
            sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_CVSD;
            ret = sco_connect(ctx->ep, SCO_LINK_TYPE_ESCO, &sco_params);
        }
    }
    #else
    {
    	if(get_current_hfp_flag(APP_HFP_PRIVATE_FLAG_CODEC_NEGOTIATION))
        {
            hfp_hf_app_t *hfp_app_ptr = NULL;
            hfp_app_ptr =(hfp_hf_app_t *)get_current_hfp_ptr();
            ret = bt_hfp_hf_establishment_codec(hfp_app_ptr->session);
        }
        else
        {
            sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_CVSD;
            ret = sco_connect(ctx->ep, SCO_LINK_TYPE_ESCO, &sco_params);
        }
    }
    #endif
#else
    sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_CVSD;
    ret = sco_connect(ctx->ep, SCO_LINK_TYPE_ESCO, &sco_params);
#endif
    os_printf("===sco connect:%d\r\n",ret);
    return ret;
}

#if PTS_TESTING
/***********************************************************
* in pts testing, just can only use [sco] type connect.
***********************************************************/
result_t util_sco_connect_pts(sco_utils_h priv)
{
    sco_params_t sco_params;
    sco_utils_t *ctx = (sco_utils_t *)priv;
    int ret = 0;
    
    sco_params.tx_bandwidth = SCO_BANDWIDTH;
    sco_params.rx_bandwidth = SCO_BANDWIDTH;
    sco_params.max_latency = SCO_MAX_LATENCY;
    sco_params.retransmit_effort = SCO_RETRANSMIT_EFFORT_POWER_CONSUMPTION;

    sco_params.pkt_type = HCI_PKT_SCO_ALL;

    sco_params.pcm_settings.input_coding = HCI_PCM_INPUT_CODING_LINEAR;
    sco_params.pcm_settings.input_format = HCI_PCM_INPUT_FORMAT_2COMP;
    sco_params.pcm_settings.input_sample_size = SCO_SAMPLE_SIZE;
    sco_params.pcm_settings.linear_bit_position = 0;
    sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_CVSD;

    ret = sco_connect(ctx->ep, SCO_LINK_TYPE_SCO, &sco_params);
    os_printf("util_sco_connect_pts, only [sco] type, 0x%x, %d \r\n", priv, ret);
    return ret;
}
#endif

result_t util_sco_disconnect(sco_utils_h priv)
{
    sco_utils_t *ctx = (sco_utils_t *)priv;

    return sco_disconnect(ctx->ep, 1);
}

result_t start_sco_test_call(sco_utils_h priv)
{
    sco_utils_t *ctx = (sco_utils_t *)priv;

    if(ctx->connected == 0)
    {
        jtask_schedule(ctx->send_out_task,
                       200,
                       (jthread_func)util_sco_connect,
                       (void *)priv);
    }

    return 0;
}

void stop_sco_test_call(sco_utils_h priv)
{
    sco_utils_t *ctx = (sco_utils_t *)priv;

    app_bt_sco_enable_wrap(0);
    app_button_type_set(BUTTON_TYPE_NON_HFP);

    jtask_stop(ctx->send_out_task);
}

uint32_t get_sco_conn_status(btaddr_t *raddr)
{
    uint32_t flag = 0;
    //在已有SCO连接，且为不同手机的蓝牙地址
    if((bt_raddr)
       && (!btaddr_same(bt_raddr,raddr)))
    {
        flag = 1;
    }

    return flag;
}
// EOF
