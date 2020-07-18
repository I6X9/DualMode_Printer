/**
 ****************************************************************************************
 *
 * @file   fcc0s_task.c
 *
 * @brief FCC0 Server Role Task Implementation.
 *
 * Copyright (C) Beken 2009-2015
 *
 *
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_FCC0_SERVER)
#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "atts.h"
#include "co_utils.h"
#include "fcc0s.h"
#include "fcc0s_task.h"
#include "uart.h"
#include "prf_utils.h"

static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid, struct gattc_att_info_req_ind *param,
                                                   ke_task_id_t const dest_id, ke_task_id_t const src_id)
{

    struct gattc_att_info_cfm * cfm;
    uint8_t  att_idx = 0;
    // retrieve handle information
    uint8_t status = fcc0s_get_att_idx(param->handle, &att_idx);

    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
    cfm->handle = param->handle;

    if(status == GAP_ERR_NO_ERROR)
    {
        // check if it's a client configuration char
        if(att_idx == FCC0S_IDX_FCC2_LVL_NTF_CFG)
        {
            // CCC attribute length = 2
            cfm->length = 2;
        }
        else // not expected request
        {
            cfm->length = 0;
            status = ATT_ERR_WRITE_NOT_PERMITTED;
        }
    }

    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

static int gattc_write_req_ind_handler(ke_msg_id_t const msgid, struct gattc_write_req_ind const *param,
                                                ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    struct gattc_write_cfm * cfm;
    uint8_t att_idx = 0;
    uint8_t conidx = KE_IDX_GET(src_id);
    uint8_t status = fcc0s_get_att_idx(param->handle, &att_idx);
    
    // If the attribute has been found, status is GAP_ERR_NO_ERROR
    if (status == GAP_ERR_NO_ERROR)
    {
        struct fcc0s_env_tag* fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);
        // Extract value before check
        uint16_t ntf_cfg = co_read16p(&param->value[0]);
        // Only update configuration if value for stop or notification enable
        if ((att_idx == FCC0S_IDX_FCC2_LVL_NTF_CFG) && ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF)))
        {
            // Conserve information in environment
            if (ntf_cfg == PRF_CLI_START_NTF)
            {
                // Ntf cfg bit set to 1
                fcc0s_env->ntf_cfg[conidx] |= (FCC0_FCC2_LVL_NTF_SUP );
            }
            else
            {
                // Ntf cfg bit set to 0
                fcc0s_env->ntf_cfg[conidx] &= ~(FCC0_FCC2_LVL_NTF_SUP );
            }

            // Inform APP of configuration change
            struct fcc0s_fcc2_level_ntf_cfg_ind * ind = KE_MSG_ALLOC(FCC0S_FCC2_LEVEL_NTF_CFG_IND, prf_dst_task_get(&(fcc0s_env->prf_env), conidx), 
                                                                      dest_id, fcc0s_fcc2_level_ntf_cfg_ind);
            ind->conidx = conidx;
            ind->ntf_cfg = fcc0s_env->ntf_cfg[conidx];
						
            ke_msg_send(ind);			
        }
		 else if ((att_idx == FCC0S_IDX_FCC3_LVL_NTF_CFG) && ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF)))
        {
            // Conserve information in environment
            if (ntf_cfg == PRF_CLI_START_NTF)
            {
                // Ntf cfg bit set to 1
                fcc0s_env->ntf_cfg[conidx] |= (FCC0_FCC2_LVL_NTF_SUP );
            }
            else
            {
                // Ntf cfg bit set to 0
                fcc0s_env->ntf_cfg[conidx] &= ~(FCC0_FCC2_LVL_NTF_SUP );
            }

            // Inform APP of configuration change
            struct fcc0s_fcc2_level_ntf_cfg_ind * ind = KE_MSG_ALLOC(FCC0S_FCC2_LEVEL_NTF_CFG_IND, prf_dst_task_get(&(fcc0s_env->prf_env), conidx), 
                                                                      dest_id, fcc0s_fcc2_level_ntf_cfg_ind);
            ind->conidx = conidx;
            ind->ntf_cfg = fcc0s_env->ntf_cfg[conidx];
						
            ke_msg_send(ind);			
        }
		else if (att_idx == FCC0S_IDX_FCC1_LVL_VAL)
		{
			// Allocate the alert value change indication
			struct fcc0s_fcc1_writer_ind *ind = KE_MSG_ALLOC(FCC0S_FCC1_WRITER_REQ_IND, prf_dst_task_get(&(fcc0s_env->prf_env), conidx),
			                                                  dest_id, fcc0s_fcc1_writer_ind);
			
			// Fill in the parameter structure	
			memcpy(ind->fcc1_value, &param->value[0], param->length);
			ind->conidx = conidx;
			ind->length = param->length;
			
			// Send the message
			ke_msg_send(ind);
		}
        else
        {
            status = PRF_APP_ERROR;
        }
    }

    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
    cfm->handle = param->handle;
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}   

static int gattc_read_req_ind_handler(ke_msg_id_t const msgid, struct gattc_read_req_ind const *param,
                                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    struct gattc_read_cfm * cfm;
    uint8_t  att_idx = 0;
    uint8_t conidx = KE_IDX_GET(src_id);
    // retrieve handle information
    uint8_t status = fcc0s_get_att_idx(param->handle, &att_idx);
    uint16_t length = 0;
    struct fcc0s_env_tag* fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);
    
    // If the attribute has been found, status is GAP_ERR_NO_ERROR
    if (status == GAP_ERR_NO_ERROR)
    {
        // read notification information
        if (att_idx == FCC0S_IDX_FCC1_LVL_VAL)
        {
            length = FCC0_FCC1_DATA_LEN * sizeof(uint8_t);
        }
        // read notification information
        else if (att_idx == FCC0S_IDX_FCC2_LVL_NTF_CFG)
        {
            length = sizeof(uint16_t);
        }
        else if (att_idx == FCC0S_IDX_FCC2_LVL_USER_DESC)
        {
			length = FCC0_FCC2_DESC_LEN;
        }
		else if (att_idx == FCC0S_IDX_FCC3_LVL_USER_DESC)
        {
			length = FCC0_FCC3_DESC_LEN;
        }
        else
        {
            status = PRF_APP_ERROR;
        }
    }

    //Send write response
    cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, length);
    cfm->handle = param->handle;
    cfm->status = status;
    cfm->length = length;
   
    if (status == GAP_ERR_NO_ERROR)
    {
        // read notification information
        if (att_idx == FCC0S_IDX_FCC1_LVL_VAL)
        {
            cfm->value[0] = fcc0s_env->fcc1_value[0];
        }
        // retrieve notification config
        else if (att_idx == FCC0S_IDX_FCC2_LVL_NTF_CFG)
        {
            uint16_t ntf_cfg = (fcc0s_env->ntf_cfg[conidx] & FCC0_FCC2_LVL_NTF_SUP) ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;
            co_write16p(cfm->value, ntf_cfg);
        }
        else if (att_idx == FCC0S_IDX_FCC2_LVL_USER_DESC)
        {
			memcpy(cfm->value, FCC0_FCC2_DESC, cfm->length);
        }
		else if (att_idx == FCC0S_IDX_FCC3_LVL_USER_DESC)
        {
			memcpy(cfm->value, FCC0_FCC3_DESC, cfm->length);
        }
    }

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}   

static int gattc_cmp_evt_handler(ke_msg_id_t const msgid, struct gattc_cmp_evt const *param,
                                         ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    if(param->operation == GATTC_NOTIFY)
    {	
        UART_PRINTF("gattc_cmp_evt_handler!\r\n");
    }

	// go back in to idle mode
    ke_state_set(dest_id, ke_state_get(dest_id) & ~FCC0S_BUSY);
	
    return (KE_MSG_CONSUMED);
}

static int fcc0s_fcc2_level_ntf_ind_handler(ke_msg_id_t const msgid, struct fcc0s_fcc2_level_ntf_ind const *param,
                                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    int msg_status = KE_MSG_SAVED;
    
    if(ke_state_get(dest_id) == FCC0S_IDLE)
    {
        struct fcc0s_env_tag* fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);
        // Allocate the GATT notification message
        struct gattc_send_evt_cmd *fcc2_lvl = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD, KE_BUILD_ID(TASK_GATTC, 0), prf_src_task_get(&(fcc0s_env->prf_env), 0),
                                                                gattc_send_evt_cmd, sizeof(uint8_t)*(param->length));

        if(fcc2_lvl == NULL)
        {
            while(1) UART_PRINTF("KE_MSG_ALLOC_DYN GATTC_SEND_EVT_CMD  == NULL \r\n");
        }

        // Fill in the parameter structure
        fcc2_lvl->operation = GATTC_NOTIFY;
        fcc2_lvl->handle = fcc0s_get_att_handle(FCC0S_IDX_FCC2_LVL_VAL);
        fcc2_lvl->seq_num = fcc2_lvl->handle;
        // pack measured value in database
        fcc2_lvl->length = param->length;
        memcpy(&fcc2_lvl->value[0], &param->fcc2_value[0], param->length);
        // send notification to peer device
        ke_msg_send(fcc2_lvl);

        ke_state_set(dest_id, FCC0S_BUSY);
        msg_status = KE_MSG_CONSUMED;
    }
    else
    {
        UART_PRINTF("fcc2_level_ntf_ind_handler state busy!!!!\r\n");
    }

    return (msg_status);
}


static int fcc0s_fcc3_level_ntf_ind_handler(ke_msg_id_t const msgid, struct fcc0s_fcc3_level_ntf_ind const *param,
													ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
  int msg_status = KE_MSG_SAVED;
  
  if(ke_state_get(dest_id) == FCC0S_IDLE)
  {
	  struct fcc0s_env_tag* fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);
	  // Allocate the GATT notification message
	  struct gattc_send_evt_cmd *fcc3_lvl = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD, KE_BUILD_ID(TASK_GATTC, 0), prf_src_task_get(&(fcc0s_env->prf_env), 0),
															  gattc_send_evt_cmd, sizeof(uint8_t)*(param->length));

	  if(fcc3_lvl == NULL)
	  {
		  while(1) UART_PRINTF("KE_MSG_ALLOC_DYN GATTC_SEND_EVT_CMD  == NULL \r\n");
	  }

	  // Fill in the parameter structure
	  fcc3_lvl->operation = GATTC_NOTIFY;
	  fcc3_lvl->handle = fcc0s_get_att_handle(FCC0S_IDX_FCC3_LVL_VAL);
	  fcc3_lvl->seq_num = fcc3_lvl->handle;
	  // pack measured value in database
	  fcc3_lvl->length = param->length;
	  memcpy(&fcc3_lvl->value[0], &param->fcc3_value[0], param->length);
	  // send notification to peer device
	  ke_msg_send(fcc3_lvl);

	  ke_state_set(dest_id, FCC0S_BUSY);
	  msg_status = KE_MSG_CONSUMED;
  }
  else
  {
	  UART_PRINTF("fcc3_level_ntf_ind_handler state busy!!!!\r\n");
  }

  return (msg_status);
}


/// Default State handlers definition
const struct ke_msg_handler fcc0s_default_state[] =
{
    {GATTC_ATT_INFO_REQ_IND,   (ke_msg_func_t) gattc_att_info_req_ind_handler},
    {GATTC_WRITE_REQ_IND,      (ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_READ_REQ_IND,       (ke_msg_func_t) gattc_read_req_ind_handler},
    {GATTC_CMP_EVT,            (ke_msg_func_t) gattc_cmp_evt_handler},
    {FCC0S_FCC2_LEVEL_NTF_IND, (ke_msg_func_t) fcc0s_fcc2_level_ntf_ind_handler},
	{FCC0S_FCC3_LEVEL_NTF_IND, (ke_msg_func_t) fcc0s_fcc3_level_ntf_ind_handler}
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler fcc0s_default_handler = KE_STATE_HANDLER(fcc0s_default_state);

#endif /* #if (BLE_FCC0_SERVER) */
