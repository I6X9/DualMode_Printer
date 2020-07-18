/**
 ****************************************************************************************
 *
 * @file fff0s.c
 *
 * @brief fff0 Server Implementation.
 *
 * Copyright (C) beken 2009-2015
 *
 *
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_FCC0_SERVER)
#include "attm.h"
#include "fcc0s.h"
#include "fcc0s_task.h"
#include "prf_utils.h"
#include "prf.h"
#include "ke_mem.h"
#include "attm_util128.h"
#include "uart.h"

/*
 * FFF0 ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// Full FFF0 Database Description - Used to add attributes into the database
const struct attm_desc fcc0_att_db[FCC0S_IDX_NB] =
{
    // FFF0 Service Declaration
    [FCC0S_IDX_SVC]               = {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},
    
	// FCC2 Level Characteristic Declaration
	[FCC0S_IDX_FCC2_LVL_CHAR]      = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // FCC2 Level Characteristic Value
    [FCC0S_IDX_FCC2_LVL_VAL]       = {ATT_USER_SERVER_CHAR_FCC2, PERM(RD, ENABLE)|PERM(NTF, ENABLE), PERM(UUID_LEN, UUID_128)|PERM(RI, ENABLE)|ATT_UUID_128_LEN, FCC0_FCC2_DATA_LEN*sizeof(uint8_t)},
	// FCC2 Level Characteristic Configuration
	[FCC0S_IDX_FCC2_LVL_NTF_CFG]   = {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, 0},
    // FCC2 Level Characteristic Description
    [FCC0S_IDX_FCC2_LVL_USER_DESC] = {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), PERM(RI, ENABLE), FCC0_FCC2_DESC_LEN},


    // FCC1 Level Characteristic Declaration
	[FCC0S_IDX_FCC1_LVL_CHAR]     = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // FCC1 Level Characteristic Value
    [FCC0S_IDX_FCC1_LVL_VAL]       = {ATT_USER_SERVER_CHAR_FCC1, PERM(WRITE_COMMAND, ENABLE), PERM(UUID_LEN, UUID_128)|0x10, FCC0_FCC1_DATA_LEN*sizeof(uint8_t)},




	
	// FCC2 Level Characteristic Declaration
	[FCC0S_IDX_FCC3_LVL_CHAR]      = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // FCC2 Level Characteristic Value
    [FCC0S_IDX_FCC3_LVL_VAL]       = {ATT_USER_SERVER_CHAR_FCC3, PERM(RD, ENABLE)|PERM(NTF, ENABLE), PERM(UUID_LEN, UUID_128)|PERM(RI, ENABLE)|ATT_UUID_128_LEN, FCC0_FCC3_DATA_LEN*sizeof(uint8_t)},
	// FCC2 Level Characteristic Configuration
	[FCC0S_IDX_FCC3_LVL_NTF_CFG]   = {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, 0},
    // FCC2 Level Characteristic Description
    [FCC0S_IDX_FCC3_LVL_USER_DESC] = {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), PERM(RI, ENABLE), FCC0_FCC3_DESC_LEN},
    
};/// Macro used to retrieve permission value from access and rights on attribute.

static uint8_t fcc0s_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  struct fcc0s_db_cfg* params)
{
    uint16_t shdl;
    struct fcc0s_env_tag* fcc0s_env = NULL;
    uint8_t status = GAP_ERR_NO_ERROR;
    
    //-------------------- allocate memory required for the profile  ---------------------
    fcc0s_env = (struct fcc0s_env_tag* ) ke_malloc(sizeof(struct fcc0s_env_tag), KE_MEM_ATT_DB);
    memset(fcc0s_env, 0 , sizeof(struct fcc0s_env_tag));

    // Service content flag
    uint32_t cfg_flag = FCC0S_CFG_FLAG_MANDATORY_MASK;

    // Save database configuration
    fcc0s_env->features |= (params->features) ;
   
    shdl = *start_hdl;

    //------------------ create the attribute database for the profile -------------------
	status = attm_svc_create_db(&(shdl), ATT_USER_SERVER_FCC0, (uint8_t *)&cfg_flag, FCC0S_IDX_NB, NULL, env->task,
	                                     &fcc0_att_db[0], (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)));	

    //-------------------- Update profile task information  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // allocate BASS required environment variable
        env->env = (prf_env_t*) fcc0s_env;
        *start_hdl = shdl;
        fcc0s_env->start_hdl = *start_hdl;
        fcc0s_env->prf_env.app_task = app_task | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        fcc0s_env->prf_env.prf_task = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_FCC0S;
        env->desc.idx_max           = FCC0S_IDX_MAX;
        env->desc.state             = fcc0s_env->state;
        env->desc.default_handler   = &fcc0s_default_handler;

        // service is ready, go into an Idle state
        ke_state_set(env->task, FCC0S_IDLE);
    }
    else if(fcc0s_env != NULL)
    {
        ke_free(fcc0s_env);
    }
     
    return (status);
}

static void fcc0s_destroy(struct prf_task_env* env)
{
    struct fcc0s_env_tag* fcc0s_env = (struct fcc0s_env_tag*) env->env;

    // clear on-going operation
    if(fcc0s_env->operation != NULL)
    {
        ke_free(fcc0s_env->operation);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(fcc0s_env);
}

static void fcc0s_create(struct prf_task_env* env, uint8_t conidx)
{
    struct fcc0s_env_tag* fff0s_env = (struct fcc0s_env_tag*) env->env;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    // force notification config to zero when peer device is connected
    fff0s_env->ntf_cfg[conidx] = 0;
}

static void fcc0s_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct fcc0s_env_tag* fcc0s_env = (struct fcc0s_env_tag*) env->env;

    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);
    // force notification config to zero when peer device is disconnected
    fcc0s_env->ntf_cfg[conidx] = 0;
}

/// BASS Task interface required by profile manager
const struct prf_task_cbs fcc0s_itf =
{
    (prf_init_fnct) fcc0s_init,
    fcc0s_destroy,
    fcc0s_create,
    fcc0s_cleanup,
};

const struct prf_task_cbs* fcc0s_prf_itf_get(void)
{
    return &fcc0s_itf;
}

uint16_t fcc0s_get_att_handle( uint8_t att_idx)
{	
    struct fcc0s_env_tag *fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);
    uint16_t handle = ATT_INVALID_HDL;
   
    handle = fcc0s_env->start_hdl;

    // increment index according to expected index
    if(att_idx < FCC0S_IDX_FCC3_LVL_NTF_CFG)
    {
        handle += att_idx;
    }
    // FCC2 notification
    else if((att_idx == FCC0S_IDX_FCC3_LVL_NTF_CFG) && (((fcc0s_env->features ) & 0x01) == FCC0_FCC2_LVL_NTF_SUP))
    {
        handle += FCC0S_IDX_FCC3_LVL_NTF_CFG;			
    }		      
    else
    {
        handle = ATT_INVALID_HDL;
    }
    
    return handle;
}

uint8_t fcc0s_get_att_idx(uint16_t handle, uint8_t *att_idx)
{
    struct fcc0s_env_tag* fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);
    uint16_t hdl_cursor = fcc0s_env->start_hdl;
    uint8_t status = PRF_APP_ERROR;

    // Browse list of services
    // handle must be greater than current index 
    // check if it's a mandatory index
    if(handle <= (hdl_cursor + FCC0S_IDX_FCC3_LVL_USER_DESC))
    {
        *att_idx = handle - hdl_cursor;
        status = GAP_ERR_NO_ERROR;  
    }
    hdl_cursor += FCC0S_IDX_FCC3_LVL_VAL;

    // check if it's a notify index
    if(((fcc0s_env->features ) & 0x01) == FCC0_FCC2_LVL_NTF_SUP)
    {
        hdl_cursor++;
        if(handle == hdl_cursor)
        {
            *att_idx = FCC0S_IDX_FCC3_LVL_NTF_CFG;
            status = GAP_ERR_NO_ERROR;
        }
    }
    hdl_cursor++;
    
    return (status);
}

#endif // (BLE_ffC0_SERVER)
