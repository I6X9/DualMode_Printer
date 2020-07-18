/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include <jos.h>
#include <bluetooth.h>
#include <bt_api.h>
#include <bt_spp.h>
#include <bt_sdp.h>
#include "bt_app_internal.h"
#include "bt_rfcomm_channel.h"
#include "app_beken_includes.h"

#ifdef CONFIG_BLUETOOTH_SPP
/* SPP Service-level Notifications */
static void spp_connected(bt_spp_session_h session, bt_app_ctx_h app_ctx);
static void spp_disconnected(bt_spp_session_h session, result_t status,
    bt_app_ctx_h app_ctx);

static bt_spp_conn_cbs_t spp_conn_cbs = {
    spp_connected,
    spp_disconnected
};

static void spp_newconn(bt_conn_req_h conn_req, const btaddr_t *laddr,
    const btaddr_t *raddr, bt_link_modes_mask_t mode,
    bt_app_ctx_h app_server_ctx, uint8_t channel);

void spp_result_input(bt_spp_session_h session, char *result,uint16_t length);

//void spp_result_input(bt_spp_session_h session, char *result,
//    bt_app_ctx_h app_ctx);

result_t spp_send( char *buff, uint8_t len );


static const bt_spp_cbs_t spp_cbs = {
    spp_result_input
};

typedef struct {
    uint32_t                    svc_id;
    uint32_t                    server_sdp;
    bt_spp_server_h             server;
    bt_spp_session_h            session;

    btaddr_t                    laddr;
    btaddr_t                    raddr;
    bt_conn_req_h               conn_req;
    bt_link_modes_mask_t        conn_req_mode;

    bool_t                      is_used;
} spp_app_t;

/* XXX: Currently, only single service is supported by this sample */
static spp_app_t g_spp_app;
unsigned char spp_is_connected(void)
{
    spp_app_t *app = &g_spp_app;
    return app->is_used;
}

static void spp_connected(bt_spp_session_h session, bt_app_ctx_h app_ctx)
{
    spp_app_t *app = &g_spp_app;

    msg_put(MSG_ENV_WRITE_ACTION);
    os_printf("spp connected: %lu\r\n", app->svc_id);
    int i,j;
    char addr[14]={0}, linkkey[34]={0};
    app_env_handle_t  env_h = app_env_get_handle();

    for (i = 0; i < MAX_KEY_STORE; i++)
    {
        for (j = 0; j < 6; j++)
            sprintf( &addr[j*2], "%02x", env_h->env_data.key_pair[i].btaddr.b[5-j] );
        for( j=0; j < 16; j++)
            sprintf( &linkkey[j*2], "%02x", env_h->env_data.key_pair[i].linkkey[j] );
        if( env_h->env_data.key_pair[i].used != 0xff)
            os_printf("key_pair[%d] used: %02x, addr: 0x%s, linkkey: 0x%s\r\n",
                i, env_h->env_data.key_pair[i].used, addr, linkkey);
    }
}

static void spp_disconnected(bt_spp_session_h session, result_t err,
    bt_app_ctx_h app_ctx)
{
    spp_app_t *app = &g_spp_app;

    os_printf("spp disconnected: %lu\r\n", app->svc_id);

    app->svc_id++;
    app->is_used = 0;
}

static void spp_newconn(bt_conn_req_h conn_req, const btaddr_t *laddr,
    const btaddr_t *raddr, bt_link_modes_mask_t mode,
    bt_app_ctx_h app_server_ctx, uint8_t channel)
{
    spp_app_t *app = &g_spp_app;

    DECLARE_FNAME("spp_newconn");

    if(app->is_used || app->conn_req)
    {
        DBG_E(DBT_HFP, ("%s: already connected - rejecting\n", FNAME));
        bt_spp_conn_reject(conn_req);
        return;
    }

    j_memcpy((void *)&app->laddr, (void *)laddr, sizeof(btaddr_t));
    j_memcpy((void *)&app->raddr, (void *)raddr, sizeof(btaddr_t));

    bt_frontend_notification("spp newconn %lu", app->svc_id);

    if(bt_spp_conn_accept(app->session, conn_req, mode))
    {
        bt_frontend_notification("spp connect %lu failed", app->svc_id);
        return;
    }

    ////app_set_led_event_action(LED_EVENT_HFP_CONNETCTING); 
    app->is_used = 1;
    bt_frontend_notification("spp connecting %lu", app->svc_id);

}

extern void uart_send (unsigned char *buff, unsigned int len) ;
void spp_result_input(bt_spp_session_h session, char *result,uint16_t length)
//    bt_app_ctx_h app_ctx)
{
    //os_printf("%s\r\n", result );
#ifdef BEKEN_OTA_SPP
    app_ota_spp_pkt_reframe((uint8*)&result[0], length);
    return;
#endif
    uart_send((unsigned char *)&result[0], length);

}

static enum {
    UNINIT = 0,
    SPP_REGISTER,
    SERVER_START,
    BACKEND_REGISTER,
    CONN_REGISTER
} spp_sample_init_stage = UNINIT;

void spp_backend_uninit(void);
result_t spp_backend_init(void)
{
    result_t err;
    
    os_printf("spp_backend_init\r\n");

    j_memset(&g_spp_app, 0, sizeof(g_spp_app));
    
    err = bt_spp_register(&spp_cbs);
    if(err)
        goto Exit;
    spp_sample_init_stage = SPP_REGISTER;

    err = bt_spp_server_start(&g_spp_app.server, BTADDR_ANY,
        RFCOMM_CHANNEL_SPP, spp_newconn, NULL);
    if(err)
        goto Exit;
    spp_sample_init_stage = SERVER_START;

#ifdef CONFIG_BLUETOOTH_COMM_INTERFACE
    err = backend_section_register(&hf_section);
    if(err)
        goto Exit;
    spp_sample_init_stage = BACKEND_REGISTER;
#endif

    err = bt_spp_conn_create(&g_spp_app.session, &spp_conn_cbs,
        NULL);
    if(err)
        goto Exit;
    spp_sample_init_stage = CONN_REGISTER;

Exit:
    DBG_RC_I(err, DBT_APP, ("%s: done, %s, stage: %d\n", FNAME, uwe_str(err),
        spp_sample_init_stage));

    if(err)
        spp_backend_uninit();
    
    return err;
}

void spp_backend_uninit(void)
{
    if(spp_sample_init_stage == CONN_REGISTER)
    {
        if(g_spp_app.conn_req)
            (void)bt_spp_conn_reject(g_spp_app.conn_req);
        
        bt_spp_conn_destroy(&g_spp_app.session);
        spp_sample_init_stage = BACKEND_REGISTER;
    }

#ifdef CONFIG_BLUETOOTH_COMM_INTERFACE
    if(spp_sample_init_stage == BACKEND_REGISTER)
    {
        backend_section_unregister(&hf_section);
        spp_sample_init_stage = SERVER_START;
    }
#endif

    if(spp_sample_init_stage == SERVER_START)
    {
        bt_spp_server_stop(&g_spp_app.server);
        spp_sample_init_stage = SPP_REGISTER;
    }

    if(spp_sample_init_stage == SPP_REGISTER)
    {
        bt_spp_unregister();
        spp_sample_init_stage = UNINIT;
    }
}

result_t spp_connect_rfcomm(btaddr_t raddr, uint8_t channel)
{
	result_t err;
	btaddr_t laddr;
	uint32_t unit_id = 0;
	spp_app_t *app = &g_spp_app;
	
	err = backend_unit_get_addr(unit_id, &laddr);
	if(err)
	{
		err = UWE_NODEV;
		goto Exit;
	}
	
	app->laddr = laddr;
	app->raddr = raddr;
	err = bt_spp_conn_connect(app->session, &laddr, &raddr, channel);
	if(err)
		goto Exit;
	
	bt_frontend_notification("spp connecting %lu", app->svc_id);
	
Exit:
	DBG_RC_I(err, DBT_APP, ("%s: Done (%s)\n", FNAME, uwe_str(err)));
	return err;
}

result_t spp_send(char *buff, uint8_t len)
{
    spp_app_t *app = &g_spp_app;


    return bt_spp_conn_send(app->session, buff, len);
}
#endif
