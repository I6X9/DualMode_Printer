/**
 ****************************************************************************************
 *
 * @file   braces_task.c
 *
 * @brief barcelet Server Role Task Implementation.
 *
 * Copyright (C) Beken 2009-2015
 *
 *
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_OTAS_SERVER)
#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "atts.h"
#include "co_utils.h"
#include "otas.h"
#include "otas_task.h"
#include "prf_utils.h"
#include "uart.h"
#include "app_ota.h"

static int ota_tx_handler(ke_msg_id_t const msgid, struct otas_tx_pdu const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    int msg_status = KE_MSG_SAVED;

    if(ke_state_get(dest_id) == OTAS_IDLE)
    {
        struct otas_env_tag* otas_env = PRF_ENV_GET(OTAS, otas);
        struct gattc_send_evt_cmd *cmd = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD, KE_BUILD_ID(TASK_GATTC, 0), prf_src_task_get(&(otas_env->prf_env),0),
                                                          gattc_send_evt_cmd, sizeof(uint8_t) * param->length);

        if(cmd == NULL)
        {
            while(1) UART_PRINTF("KE_MSG_ALLOC_DYN GATTC_SEND_EVT_CMD  == NULL \r\n");
        }

        // Fill in the parameter structure
        cmd->operation = GATTC_NOTIFY;
        cmd->handle = otas_env->otas_start_hdl + OTAS_IDX_TX_VAL;
        cmd->seq_num = cmd->handle;
        // pack measured value in database
        cmd->length = param->length;
        memcpy(&cmd->value[0], param->data, param->length);

        // send notification to peer device
        ke_msg_send(cmd);

        ke_state_set(dest_id, OTAS_BUSY);
        msg_status = KE_MSG_CONSUMED;


    }
    else
    {
        UART_PRINTF("ota_tx_handler state busy!!!!\r\n");
    }

    return (msg_status);
}

static int gattc_write_req_ind_handler(ke_msg_id_t const msgid, struct gattc_write_req_ind const *param,
                                                ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // UART_PRINTF("oad %s \r\n",__func__);
    uint8_t status = ATT_ERR_NO_ERROR;
    int msg_status = KE_MSG_CONSUMED;
    uint8_t conidx = KE_IDX_GET(src_id);
		
    // If the attribute has been found, status is ATT_ERR_NO_ERROR
    if(ke_state_get(dest_id) == OTAS_IDLE)
    {	
        struct otas_env_tag* otas_env = PRF_ENV_GET(OTAS, otas);
        
		if(otas_env == NULL)
		{
			while(1){ UART_PRINTF("otas_env == null\r\n");};
		}
		uint16_t ntf_cfg = co_read16p(&param->value[0]);
		if(((otas_env->features & 0x01) == OTAS_NTF_SUP) && ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF)))
		{
			
		    if(param->handle == (otas_env->otas_start_hdl + OTAS_IDX_TX_CFG))
			{
			 	//UART_PRINTF("OADS_IDX_FFC2_LVL_NTF_CFG ntf_cfg = %d\r\n",ntf_cfg);
				if (ntf_cfg == PRF_CLI_START_NTF)
				{
				    // Ntf cfg bit set to 1
				    otas_env->tx_ntf_cfg[conidx] |= (OTAS_NTF_SUP);
				}
				else
				{
				    // Ntf cfg bit set to 0
				    otas_env->tx_ntf_cfg[conidx] &= ~(OTAS_NTF_SUP);
				}
			}
		}	
				
		if(param->handle == (otas_env->otas_start_hdl + OTAS_IDX_RX_VAL))
		{   
#ifdef BEKEN_OTA_BLE
            struct otas_rx_pdu *rx_pdu = KE_MSG_ALLOC(OTAS_RX, TASK_APP, dest_id, otas_rx_pdu);
            
            rx_pdu->length = param->length;
            memcpy(rx_pdu->data, &param->value[0], param->length);
            ke_msg_send(rx_pdu);
#endif
		}
			
		struct gattc_write_cfm *cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
		cfm->handle = param->handle;
		cfm->status = status;
	  	ke_msg_send(cfm);
    }
	else if(ke_state_get(dest_id) == OTAS_BUSY)
    {
		UART_PRINTF("OTAS_BUSY\r\n");
      	msg_status = KE_MSG_SAVED;
    }
    return (msg_status);
}   

static int gattc_read_req_ind_handler(ke_msg_id_t const msgid, struct gattc_read_req_ind const *param,
                                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    //UART_PRINTF("oads %s \r\n",__func__);
	uint8_t status = ATT_ERR_NO_ERROR;
	int msg_status = KE_MSG_CONSUMED;
	struct gattc_read_cfm * cfm;
	struct otas_env_tag* otas_env = PRF_ENV_GET(OTAS, otas);
	uint16_t length = 0;
	uint8_t value[20] = {0};

	if(ke_state_get(dest_id) == OTAS_IDLE)
	{
        if(param->handle == (otas_env->otas_start_hdl + OTAS_IDX_TX_CFG))
		{
			length = sizeof(uint16_t);
			memcpy(value,&otas_env->tx_ntf_cfg[0],length);
		}
		else
		{
		    //UART_PRINTF("Read status ATT_ERR_REQUEST_NOT_SUPPORTED \r\n");
		    status = ATT_ERR_REQUEST_NOT_SUPPORTED;
		}
			
		cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, length);
		cfm->length = length;
		memcpy(cfm->value, value, length);
		cfm->handle = param->handle;
		cfm->status = status;
		
		// Send value to peer device.
      	ke_msg_send(cfm);				
	}
	else if(ke_state_get(dest_id) == OTAS_BUSY)
	{
		msg_status = KE_MSG_SAVED;
	}

    return (msg_status);
}   

static int gattc_cmp_evt_handler(ke_msg_id_t const msgid, struct gattc_cmp_evt const *param,
                                         ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if(param->operation == GATTC_NOTIFY)
    {
        if(param->status == GAP_ERR_NO_ERROR)
        {
            ke_state_set(dest_id, OTAS_IDLE);
        }
    }
    return (KE_MSG_CONSUMED);
}

/// Default State handlers definition
const struct ke_msg_handler otas_default_state[] =
{
    {OTAS_TX,                       (ke_msg_func_t) ota_tx_handler},
    {GATTC_WRITE_REQ_IND,    	    (ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_READ_REQ_IND,            (ke_msg_func_t) gattc_read_req_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t) gattc_cmp_evt_handler},
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler otas_default_handler = KE_STATE_HANDLER(otas_default_state);

#endif /* #if (BLE_OTAS_SERVER) */
