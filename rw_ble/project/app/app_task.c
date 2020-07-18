/**
 ****************************************************************************************
 *
 * @file appm_task.c
 *
 * @brief RW APP Task implementation
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"          // SW configuration

#if (BLE_APP_PRESENT)
#include <string.h>
#include "app_task.h"              // Application Manager Task API
#include "app.h"                      // Application Manager Definition
#include "app_wlist.h"
#include "gapc_task.h"            // GAP Controller Task API
#include "gapm_task.h"          // GAP Manager Task API
#include "gattc_task.h"
#include "arch.h"                    // Platform Definitions
#include "ke_timer.h"             // Kernel timer
#include "app_sec.h"              // Security Module Definition
#include "app_dis.h"              // Device Information Module Definition
#include "diss_task.h"
#include "app_batt.h"             // Battery Module Definition
#include "bass_task.h"
#include "app_hid.h"              // HID Module Definition
#include "hogpd_task.h"
#include "app_fcc0.h"              // fff0 Module Definition
#include "fcc0s_task.h"
#include "app_ota.h"             
#include "otas_task.h"              
#include "uart.h"
#include "reg_ble_em_cs.h"
#include "lld.h"
#include "user_config.h"


//this flag use indicate adv timeout
static bool adv_timeout_flag = false;


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static uint8_t appm_get_handler(const struct ke_state_handler *handler_list,
                                ke_msg_id_t msgid,
                                void *param,
                                ke_task_id_t src_id)
{
    // Counter
    uint8_t counter;

    // Get the message handler function by parsing the message table
    for (counter = handler_list->msg_cnt; 0 < counter; counter--)
    {
			
        struct ke_msg_handler handler = (*(handler_list->msg_table + counter - 1));
			
        if ((handler.id == msgid) ||
            (handler.id == KE_MSG_DEFAULT_HANDLER))
        {
            // If handler is NULL, message should not have been received in this state
            ASSERT_ERR(handler.func);

            return (uint8_t)(handler.func(msgid, param, TASK_APP, src_id));
        }
    }

    // If we are here no handler has been found, drop the message
    return (KE_MSG_CONSUMED);
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_adv_timeout_handler(ke_msg_id_t const msgid,
                                   void const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    UART_PRINTF("%s\r\n", __func__);

    adv_timeout_flag = true;

    // Stop advertising
    appm_stop_advertising();

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles ready indication from the GAP. - Reset the stack
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_device_ready_ind_handler(ke_msg_id_t const msgid,
                                         void const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Application has not been initialized
    ASSERT_ERR(ke_state_get(dest_id) == APPM_INIT);

    // Reset the stack
    struct gapm_reset_cmd* cmd = KE_MSG_ALLOC(GAPM_RESET_CMD,
                                              TASK_GAPM, TASK_APP,
                                              gapm_reset_cmd);

    cmd->operation = GAPM_RESET;

    ke_msg_send(cmd);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAP manager command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapm_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
	//UART_PRINTF("param->operation = 0x%x, param->status = 0x%x \r\n", param->operation, param->status);
    switch(param->operation)
    {
        // Reset completed
        case (GAPM_RESET):
        {
            if(param->status == GAP_ERR_NO_ERROR)
            {
                // Set Device configuration
                struct gapm_set_dev_config_cmd* cmd = KE_MSG_ALLOC(GAPM_SET_DEV_CONFIG_CMD,
	                                                                   TASK_GAPM, TASK_APP,
                                                                   gapm_set_dev_config_cmd);
                // Set the operation
                cmd->operation = GAPM_SET_DEV_CONFIG;
                // Set the device role - Peripheral
                cmd->role      = GAP_ROLE_PERIPHERAL;
                // Set Data length parameters
                cmd->sugg_max_tx_octets = BLE_MAX_OCTETS;
                cmd->sugg_max_tx_time   = BLE_MAX_TIME_4_2;
								
		 		cmd->max_mtu = 512;//BLE_MIN_OCTETS;
                //Do not support secure connections
                cmd->pairing_mode = GAPM_PAIRING_LEGACY;
                // Enable Slave Preferred Connection Parameters present 
                cmd->att_cfg = GAPM_MASK_ATT_SLV_PREF_CON_PAR_EN;
                // load IRK
                memcpy(cmd->irk.key, app_env.loc_irk, KEY_LEN);

                // Send message
                ke_msg_send(cmd);
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
        break;
        case (GAPM_PROFILE_TASK_ADD):
        {
            // Add the next requested service
            if (!appm_add_svc())
            {
                // Go to the ready state
                ke_state_set(TASK_APP, APPM_READY);
							
				//device not bonded, start general adv
				appm_start_advertising();
            }
        }
        break;
        // Device Configuration updated
        case (GAPM_SET_DEV_CONFIG):
        {
            ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation, param->status);

            // Go to the create db state
            ke_state_set(TASK_APP, APPM_CREATE_DB);
            // Add the first required service in the database
            // and wait for the PROFILE_ADDED_IND
            appm_add_svc();
        }
        break;	

        case (GAPM_ADV_NON_CONN):
        case (GAPM_ADV_UNDIRECT):
        case (GAPM_ADV_DIRECT):
		case (GAPM_UPDATE_ADVERTISE_DATA):
        case (GAPM_ADV_DIRECT_LDC):
		{
			//if device cancel advtising is adv timeout,then reset this flag
			if((param->status == GAP_ERR_CANCELED) && (adv_timeout_flag == true))
			{
				adv_timeout_flag = false;
				
			}
		}
        break;

        default:
        {
            // Drop the message
        }
        break;
    }

    return (KE_MSG_CONSUMED);
}

static int gapc_get_dev_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gapc_get_dev_info_req_ind const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    switch(param->req)
    {
        case GAPC_DEV_NAME:
        {
            struct gapc_get_dev_info_cfm * cfm = KE_MSG_ALLOC_DYN(GAPC_GET_DEV_INFO_CFM,
                                                    src_id, dest_id,
                                                    gapc_get_dev_info_cfm, APP_DEVICE_NAME_MAX_LEN);
            cfm->req = param->req;
            cfm->info.name.length = appm_get_dev_name(cfm->info.name.value);

            // Send message
            ke_msg_send(cfm);
        } break;

        case GAPC_DEV_APPEARANCE:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                                                    src_id, dest_id,
                                                    gapc_get_dev_info_cfm);
            cfm->req = param->req;
            // Set the device appearance
            cfm->info.appearance = 961;
            
            // Send message
            ke_msg_send(cfm);
        } break;

        case GAPC_DEV_SLV_PREF_PARAMS:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                    								src_id, dest_id,
                                                    gapc_get_dev_info_cfm);
            cfm->req = param->req;
            // Slave preferred Connection interval Min
            cfm->info.slv_params.con_intv_min = 8;
            // Slave preferred Connection interval Max
            cfm->info.slv_params.con_intv_max = 10;
            // Slave preferred Connection latency
            cfm->info.slv_params.slave_latency = 180;
            // Slave preferred Link supervision timeout
            cfm->info.slv_params.conn_timeout  = 600;  // 6s (600*10ms)

            // Send message
            ke_msg_send(cfm);
        } break;

        default: /* Do Nothing */
			break;
    }


    return (KE_MSG_CONSUMED);
}
/**
 ****************************************************************************************
 * @brief Handles GAPC_SET_DEV_INFO_REQ_IND message.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_set_dev_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gapc_set_dev_info_req_ind const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
	// Set Device configuration
	struct gapc_set_dev_info_cfm* cfm = KE_MSG_ALLOC(GAPC_SET_DEV_INFO_CFM, src_id, dest_id,
                                                 gapc_set_dev_info_cfm);
	// Reject to change parameters
	cfm->status = GAP_ERR_REJECTED;
	cfm->req = param->req;
	// Send message
	ke_msg_send(cfm);

	return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles connection complete event from the GAP. Enable all required profiles
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_connection_req_ind_handler(ke_msg_id_t const msgid,
                                           struct gapc_connection_req_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{	
	UART_PRINTF("%s\r\n", __func__);
	adv_timeout_flag = false;
	
    app_env.conidx = KE_IDX_GET(src_id);
    // Check if the received Connection Handle was valid
    if (app_env.conidx != GAP_INVALID_CONIDX)
    {
        uint8_t i;
        // Retrieve the connection info from the parameters
        app_env.conhdl = param->conhdl;

        // Clear the advertising timeout timer
        if (ke_timer_active(APP_ADV_TIMEOUT_TIMER, TASK_APP))
        {
            ke_timer_clear(APP_ADV_TIMEOUT_TIMER, TASK_APP);
        }

		struct gapc_connection_cfm *cfm = KE_MSG_ALLOC(GAPC_CONNECTION_CFM,
    							KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
    							gapc_connection_cfm);

    	cfm->auth = GAP_AUTH_REQ_NO_MITM_NO_BOND; 
    	
    	//Send the message
    	ke_msg_send(cfm);			

        /*--------------------------------------------------------------
         * ENABLE REQUIRED PROFILES
         *--------------------------------------------------------------*/
        #if BLE_BATT_SERVER
        // Enable Battery Service
        app_batt_enable_prf(app_env.conhdl);
        #endif

		UART_PRINTF("peer_addr_type = 0x%x\r\n",param->peer_addr_type);
		UART_PRINTF("peer_addr = ");
		for(i = 0; i < sizeof(bd_addr_t);i ++)
		{
			UART_PRINTF("0x%02x ",param->peer_addr.addr[i]);
		}
		UART_PRINTF("\r\n");
				
        // We are now in connected State
        ke_state_set(dest_id, APPM_CONNECTED);

		//if device not bond with local, send bond request
		//ke_timer_set(APP_SEND_SECURITY_REQ,TASK_APP,10);  
		
		#if UPDATE_CONNENCT_PARAM
		ke_timer_set(APP_PARAM_UPDATE_REQ_IND,TASK_APP,100); 
		#endif	
		
    }
    else
    {
		appm_start_advertising();
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAP controller command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
	UART_PRINTF("gapc_cmp_evt_handler operation = %x, status = %x\r\n",param->operation, param->status);
	switch(param->operation)
	{
    	case (GAPC_UPDATE_PARAMS):
    	{
			if (param->status != GAP_ERR_NO_ERROR)
        	{
            	//appm_disconnect();
			}
			UART_PRINTF("gapc update paras state: 0x%x\r\n", param->status);
    	} break;

		case (GAPC_DISCONNECT):
		{
			UART_PRINTF("GAPC_DISCONNECT: 0x%x\r\n", param->status);
		}break;

    	default:
    	  break;
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles disconnection complete event from the GAP.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                      struct gapc_disconnect_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	UART_PRINTF("disconnect link reason = 0x%x\r\n",param->reason);
#if UPDATE_CONNENCT_PARAM
	if(ke_timer_active(APP_PARAM_UPDATE_REQ_IND, TASK_APP))
	{
		ke_timer_clear(APP_PARAM_UPDATE_REQ_IND, TASK_APP);
	}
#endif

	// Go to the ready state
	ke_state_set(TASK_APP, APPM_READY);

    adv_timeout_flag = false;
	
	ke_state_set(TASK_APP, APPM_READY);
	appm_start_advertising();
	
    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles profile add indication from the GAP.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_profile_added_ind_handler(ke_msg_id_t const msgid,
                                          struct gapm_profile_added_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Current State
    uint8_t state = ke_state_get(dest_id);

    if (state == APPM_CREATE_DB)
    {
        switch (param->prf_task_id)
        {
            default: 
			break;
        }
    }
    else
    {
        ASSERT_INFO(0, state, src_id);
    }

    return KE_MSG_CONSUMED;
}



/**
 ****************************************************************************************
 * @brief Handles reception of all messages sent from the lower layers to the application
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int appm_msg_handler(ke_msg_id_t const msgid,
                            void *param,
                            ke_task_id_t const dest_id,
                            ke_task_id_t const src_id)
{
    // Retrieve identifier of the task from received message
    ke_task_id_t src_task_id = MSG_T(msgid);
    // Message policy
    uint8_t msg_pol          = KE_MSG_CONSUMED;


    switch (src_task_id)
    {
        case (TASK_ID_GAPC):
        {
            if ((msgid >= GAPC_BOND_CMD) &&
                (msgid <= GAPC_SECURITY_IND))
            {
                // Call the Security Module
                msg_pol = appm_get_handler(&app_sec_table_handler, msgid, param, src_id);
            }
            // else drop the message
        } break;

        case (TASK_ID_GATTC):
        {
            // Service Changed - Drop
        } break;		

		case (TASK_ID_FCC0S):
        {
            // Call the Health Thermometer Module
            msg_pol = appm_get_handler(&app_fcc0_table_handler, msgid, param, src_id);
        } break;		
#if BLE_DIS_SERVER	
        case (TASK_ID_DISS):
        {
            // Call the Device Information Module
            msg_pol = appm_get_handler(&app_dis_table_handler, msgid, param, src_id);
        } break;
#endif
#if BLE_BATT_SERVER
        case (TASK_ID_BASS):
        {
            // Call the Battery Module
            msg_pol = appm_get_handler(&app_batt_table_handler, msgid, param, src_id);
        } break;
#endif
#ifdef BEKEN_OTA_BLE
        case (TASK_ID_OTAS):
        {
            // Call the Beken OTA Module
            msg_pol = appm_get_handler(&app_otas_table_handler, msgid, param, src_id);
        } break;
#endif
        default:
        break;
    }

    return (msg_pol);
}


/*******************************************************************************
 * Function: gapc_update_conn_param_req_ind_handler
 * Description: Update request command processing from slaver connection parameters
 * Input: msgid   -Id of the message received.
 *		  param   -Pointer to the parameters of the message.
 *		  dest_id -ID of the receiving task instance
 *		  src_id  -ID of the sending task instance.
 * Return: If the message was consumed or not.
 * Others: void
*******************************************************************************/
static int gapc_update_conn_param_req_ind_handler (ke_msg_id_t const msgid, 
									const struct gapc_param_update_req_ind  *param,
                 					ke_task_id_t const dest_id, ke_task_id_t const src_id)
{

	UART_PRINTF("slave send param_update_req\r\n");
	
	appm_update_param(12, 12, 0, 600);
	
	return KE_MSG_CONSUMED;
}

 
/*******************************************************************************
 * Function: gapc_le_pkt_size_ind_handler
 * Description: GAPC_LE_PKT_SIZE_IND
 * Input: msgid   -Id of the message received.
 *		  param   -Pointer to the parameters of the message.
 *		  dest_id -ID of the receiving task instance
 *		  src_id  -ID of the sending task instance.
 * Return: If the message was consumed or not.
 * Others: void
*******************************************************************************/
static int gapc_le_pkt_size_ind_handler (ke_msg_id_t const msgid, 
									const struct gapc_le_pkt_size_ind  *param,
                 					ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
   	UART_PRINTF("%s \r\n", __func__);
	
	UART_PRINTF("1max_rx_octets = %d\r\n",param->max_rx_octets);
	UART_PRINTF("1max_rx_time = %d\r\n",param->max_rx_time);
	UART_PRINTF("1max_tx_octets = %d\r\n",param->max_tx_octets);
	UART_PRINTF("1max_tx_time = %d\r\n",param->max_tx_time);
	
	return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief  GAPC_PARAM_UPDATED_IND
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_param_updated_ind_handler (ke_msg_id_t const msgid, 
									const struct gapc_param_updated_ind  *param,
                 					ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    UART_PRINTF("%s \r\n", __func__);
	
	UART_PRINTF("con_interval = %d\r\n",param->con_interval);
	UART_PRINTF("con_latency = %d\r\n",param->con_latency);
	UART_PRINTF("sup_to = %d\r\n",param->sup_to);
	
	return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief  APP_SEND_SECURITY_REQ
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_send_security_req_handler(ke_msg_id_t const msgid, void const *param,
        ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    appm_send_seurity_req();
	
    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief  GATTC_MTU_CHANGED_IND
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_mtu_changed_ind_handler(ke_msg_id_t const msgid,
                                     struct gattc_mtu_changed_ind const *ind,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
	UART_PRINTF("%s \r\n",__func__);
	UART_PRINTF("ind->mtu = %d.seq = %d\r\n",ind->mtu,ind->seq_num);
	
 	return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief   GAPC_PARAM_UPDATE_REQ_IND
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_param_update_req_ind_handler(ke_msg_id_t const msgid,
                                struct gapc_param_update_req_ind const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
	UART_PRINTF("%s \r\n", __func__);
	// Prepare the GAPC_PARAM_UPDATE_CFM message
    struct gapc_param_update_cfm *cfm = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CFM,
                                             src_id, dest_id,
                                             gapc_param_update_cfm);
	 
	//cfm->accept = false;
	cfm->ce_len_max = 0xffff;
	cfm->ce_len_min = 0xffff;
	cfm->accept = true; 

	// Send message
    ke_msg_send(cfm);
	 
	return (KE_MSG_CONSUMED);
}


/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

/* Default State handlers definition. */
const struct ke_msg_handler appm_default_state[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,    	(ke_msg_func_t)appm_msg_handler},
    {APP_ADV_TIMEOUT_TIMER,     	(ke_msg_func_t)app_adv_timeout_handler},
    {GAPM_DEVICE_READY_IND,     	(ke_msg_func_t)gapm_device_ready_ind_handler},
    {GAPM_CMP_EVT,             		(ke_msg_func_t)gapm_cmp_evt_handler},
    {GAPC_GET_DEV_INFO_REQ_IND, 	(ke_msg_func_t)gapc_get_dev_info_req_ind_handler},
    {GAPC_SET_DEV_INFO_REQ_IND, 	(ke_msg_func_t)gapc_set_dev_info_req_ind_handler},
    {GAPC_CONNECTION_REQ_IND,   	(ke_msg_func_t)gapc_connection_req_ind_handler},
    {GAPC_CMP_EVT,             		(ke_msg_func_t)gapc_cmp_evt_handler},
    {GAPC_DISCONNECT_IND,       	(ke_msg_func_t)gapc_disconnect_ind_handler},
    {GAPM_PROFILE_ADDED_IND,    	(ke_msg_func_t)gapm_profile_added_ind_handler},
    {GAPC_LE_PKT_SIZE_IND,			(ke_msg_func_t)gapc_le_pkt_size_ind_handler},
    {GAPC_PARAM_UPDATED_IND,		(ke_msg_func_t)gapc_param_updated_ind_handler},
    {GATTC_MTU_CHANGED_IND,			(ke_msg_func_t)gattc_mtu_changed_ind_handler},	
    {GAPC_PARAM_UPDATE_REQ_IND, 	(ke_msg_func_t)gapc_param_update_req_ind_handler},
    {APP_PARAM_UPDATE_REQ_IND, 		(ke_msg_func_t)gapc_update_conn_param_req_ind_handler},
    {APP_SEND_SECURITY_REQ,     	(ke_msg_func_t)gapc_send_security_req_handler},
};

/* Specifies the message handlers that are common to all states. */
const struct ke_state_handler appm_default_handler = KE_STATE_HANDLER(appm_default_state);

/* Defines the place holder for the states of all the task instances. */
ke_state_t appm_state[APP_IDX_MAX];

#endif //(BLE_APP_PRESENT)

/// @} APPTASK
