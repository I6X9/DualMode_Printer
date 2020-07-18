/**
 ****************************************************************************************
 *
 * @file app_fff0.c
 *
 * @brief fff0 Application Module entry point
 *
 * @auth  gang.cheng
 *
 * @date  2016.05.31
 *
 * Copyright (C) Beken 2009-2016
 *
 *
 ****************************************************************************************
 */
#include "rwip_config.h"     // SW configuration
#if (BLE_FCC0_SERVER)
#include <string.h>
#include "app_fcc0.h"              // Battery Application Module Definitions
#include "app.h"                    // Application Definitions
#include "app_task.h"             // application task definitions
#include "fcc0s_task.h"           // health thermometer functions
#include "co_bt.h"
#include "prf_types.h"             // Profile common types definition
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "fcc0s.h"
#include "ke_timer.h"
#include "uart.h"
#include "Co_utils.h"

/// fff0 Application Module Environment Structure
struct app_fcc0_env_tag app_fcc0_env;

void app_fcc0_init(void)
{
    // Reset the environment
    memset(&app_fcc0_env, 0, sizeof(struct app_fcc0_env_tag));
}

void app_fcc0_add_fcc0s(void)
{
   struct fcc0s_db_cfg *db_cfg;	
   struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD, TASK_GAPM, TASK_APP,
                                                             gapm_profile_task_add_cmd, sizeof(struct fcc0s_db_cfg));
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = 0;
    req->prf_task_id = TASK_ID_FCC0S;
    req->app_task = TASK_APP;
    req->start_hdl = 0; 

    // Set parameters
    db_cfg = (struct fcc0s_db_cfg*)req->param;
	 
    // Sending of notifications is supported
    db_cfg->features = FCC0_FCC2_LVL_NTF_SUP;
    // Send the message
    ke_msg_send(req);
}

static int app_fcc0_fcc2_level_ntf_cfg_ind_handler(ke_msg_id_t const msgid, struct fcc0s_fcc2_level_ntf_cfg_ind const *param,
                                                                ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
	UART_PRINTF("fcc2 param->ntf_cfg = %x\r\n",param->ntf_cfg);
	if(param->ntf_cfg == PRF_CLI_STOP_NTFIND)
	{
		//ke_timer_clear(FCC0S_FCC1_LEVEL_PERIOD_NTF, dest_id);
	}
    else
	{
		//ke_timer_set(FCC0S_FCC1_LEVEL_PERIOD_NTF, dest_id, 1);
	}
    
    return (KE_MSG_CONSUMED);
}
void app_ff01_send_data(uint8_t* buf, uint8_t len){


		struct fcc0s_fcc2_level_ntf_ind * ind = KE_MSG_ALLOC(FCC0S_FCC2_LEVEL_NTF_IND, prf_get_task_from_id(TASK_ID_FCC0S),
															  TASK_APP, fcc0s_fcc2_level_ntf_ind);
		// Fill in the parameter structure
		ind->conidx =0;
		ind->length =len;
		memcpy(ind->fcc2_value, buf, len);

		// Send the message
		ke_msg_send(ind);

}

void app_ff03_send_data(uint8_t* buf, uint8_t len){


		struct fcc0s_fcc3_level_ntf_ind * ind = KE_MSG_ALLOC(FCC0S_FCC3_LEVEL_NTF_IND, prf_get_task_from_id(TASK_ID_FCC0S),
															  TASK_APP, fcc0s_fcc3_level_ntf_ind);
		// Fill in the parameter structure
		ind->conidx =0;
		ind->length =len;
		memcpy(ind->fcc3_value, buf, len);

		// Send the message
		ke_msg_send(ind);

}
extern void uart_send (unsigned char *buff, unsigned int len) ;

static int app_fcc0_fcc1_level_upd_handler(ke_msg_id_t const msgid, struct fcc0s_fcc1_writer_ind const *param,
                                                       ke_task_id_t const dest_id, ke_task_id_t const src_id)
{

	uart_send(param->fcc1_value,param->length);
	
    return (KE_MSG_CONSUMED);
}



                                              

                                   
static int app_fcc0_msg_dflt_handler(ke_msg_id_t const msgid, void const *param,
                                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Drop the message
    return (KE_MSG_CONSUMED);
}


// Default State handlers definition
const struct ke_msg_handler app_fcc0_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,       (ke_msg_func_t)app_fcc0_msg_dflt_handler},
    {FCC0S_FCC1_WRITER_REQ_IND,    (ke_msg_func_t)app_fcc0_fcc1_level_upd_handler},
    {FCC0S_FCC2_LEVEL_NTF_CFG_IND, (ke_msg_func_t)app_fcc0_fcc2_level_ntf_cfg_ind_handler},
};

const struct ke_state_handler app_fcc0_table_handler = {&app_fcc0_msg_handler_list[0], (sizeof(app_fcc0_msg_handler_list)/sizeof(struct ke_msg_handler))};
#endif
