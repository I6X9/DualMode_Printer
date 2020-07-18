/**
 ****************************************************************************************
 *
 * @file braces.c
 *
 * @brief barcelet Server Implementation.
 *
 * Copyright (C) beken 2009-2015
 *
 *
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_OTAS_SERVER)
#include "attm.h"
#include "otas.h"
#include "otas_task.h"
#include "ke_mem.h"
#include "prf_utils.h"
#include "prf.h"
#include "co_utils.h"
#include "driver_flash.h"
#include "uart.h"
#include "ke.h"
#include "sys_irq.h"
#include "uw_errno.h"
#include "att.h"
#include "lmp_utils.h"
#include "app_beken_includes.h"

const struct attm_desc otas_att_db[OTAS_IDX_NB] =
{
	// Service Declaration
	[OTAS_IDX_SVC]     = {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},
    // Characteristic Declaration
	[OTAS_IDX_RX_CHAR] = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},

	[OTAS_IDX_RX_VAL]  = {ATT_USER_SERVER_CHAR_RX, PERM(WRITE_COMMAND, ENABLE), PERM(UUID_LEN, UUID_128)|0x10, OTAS_RX_LEN},

	[OTAS_IDX_TX_CHAR] = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Characteristic Declaration
	[OTAS_IDX_TX_VAL]  = {ATT_USER_SERVER_CHAR_TX, PERM(NTF, ENABLE), PERM(UUID_LEN, UUID_128)|0x10, OTAS_TX_LEN},

	[OTAS_IDX_TX_CFG]  = {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, 0},
};

static uint8_t otas_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  struct otas_db_cfg* params)
{
    // Service content flag
    uint32_t cfg_flag = 0xFFFFFFFF;
    // DB Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;
	
    struct otas_env_tag* otas_env = (struct otas_env_tag*)ke_malloc(sizeof(struct otas_env_tag), KE_MEM_ATT_DB);

    // allocate BRACE required environment variable
    env->env = (prf_env_t*) otas_env;
	
  	status = attm_svc_create_db128( start_hdl, ATT_USER_SERVER_SVC_OAD, (uint8_t *)&cfg_flag,
                                    OTAS_IDX_NB, NULL, env->task, &otas_att_db[0],
                                    (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))
                                    | PERM(SVC_MI, DISABLE) );	

	UART_PRINTF("create_db status = %x\r\n",status);
    otas_env->otas_start_hdl = *start_hdl;
    *start_hdl += OTAS_IDX_NB;
		        
    if(status == ATT_ERR_NO_ERROR)
    {	
		// set start handle to first allocated service value
		*start_hdl = otas_env->otas_start_hdl;

		otas_env->features          = params->features;
		otas_env->prf_env.app_task  = app_task|(PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
		otas_env->prf_env.prf_task  = env->task|PERM(PRF_MI, DISABLE);

		// initialize environment variable
		env->id                     = TASK_ID_OTAS;
		env->desc.idx_max           = OTAS_IDX_MAX;
		env->desc.state             = otas_env->state;
		env->desc.default_handler   = &otas_default_handler;

		// service is ready, go into an Idle state
		ke_state_set(env->task, OTAS_IDLE);
    }
    return (status);
}

static void otas_destroy(struct prf_task_env* env)
{
    struct otas_env_tag* otas_env = (struct otas_env_tag*) env->env;

    // clear on-going operation
    if(otas_env->operation != NULL)
    {
        ke_free(otas_env->operation);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(otas_env);
}

static void otas_create(struct prf_task_env* env, uint8_t conidx)
{
    struct otas_env_tag* otas_env = (struct otas_env_tag*) env->env;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    // force notification config to zero when peer device is connected
	otas_env->tx_ntf_cfg[conidx] = 0;
}

static void otas_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct otas_env_tag* otas_env = (struct otas_env_tag*) env->env;

    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);
    // force notification config to zero when peer device is disconnected
	otas_env->tx_ntf_cfg[conidx] = 0;
}

// BRACE Task interface required by profile manager
const struct prf_task_cbs otas_itf =
{
    (prf_init_fnct) otas_init,
    otas_destroy,
    otas_create,
    otas_cleanup,
};

const struct prf_task_cbs* otas_prf_itf_get(void)
{
   return &otas_itf;
}

#endif // (BLE_BRACELET_REPORTER)
