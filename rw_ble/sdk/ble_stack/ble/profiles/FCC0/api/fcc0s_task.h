/**
 ****************************************************************************************
 *
 * @file ffC0s_task.h
 *
 * @brief Header file - Battery Service Server Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */
#ifndef _FCC0S_TASK_H_
#define _FCC0S_TASK_H_

#include "rwprf_config.h"
#if (BLE_FCC0_SERVER)
#include "types.h"
#include "rwip_task.h" // Task definitions

///Maximum number of FFF0 Server task instances
#define FCC0S_IDX_MAX     0x01
///Maximal number of FFF0 that can be added in the DB
#define  FCC0_FCC1_DATA_LEN  200
#define  FCC0_FCC2_DATA_LEN  200
#define  FCC0_FCC3_DATA_LEN  200

///Description
#define  FCC0_FCC2_DESC      "Beken_notify_01"
#define  FCC0_FCC2_DESC_LEN  sizeof(FCC0_FCC2_DESC)

#define  FCC0_FCC3_DESC      "Beken_notify_03"
#define  FCC0_FCC3_DESC_LEN  sizeof(FCC0_FCC3_DESC)

/// Possible states of the FFF0S task
enum fcc0s_state
{
    /// Idle state
    FCC0S_IDLE,
    /// busy state
    FCC0S_BUSY,
    /// Number of defined states.
    FCC0S_STATE_MAX
};

/// Messages for FFF0 Server
enum fcc0s_msg_id
{
    /// Start the FFF0 Server - at connection used to restore bond data
	FCC0S_CREATE_DB_REQ   = TASK_FIRST_MSG(TASK_ID_FCC0S),
		
	FCC0S_FCC1_WRITER_REQ_IND,

	FCC0S_FCC2_LEVEL_NTF_IND,

	FCC0S_FCC3_LEVEL_NTF_IND,
	
    FCC0S_FCC2_LEVEL_NTF_CFG_IND,
    
   	
};

/// Features Flag Masks
enum fcc0s_features
{
    /// FCC1 Level Characteristic doesn't support notifications
    FCC0_FCC1_LVL_NTF_NOT_SUP,
    /// FCC2 Level Characteristic support notifications
    FCC0_FCC2_LVL_NTF_SUP,
    
};

/// Parameters for the database creation
struct fcc0s_db_cfg
{
    /// Number of FFF0 to add
    uint8_t fcc0_nb;
    /// Features of each FFF0 instance
    uint8_t features;
};

/// Parameters of the @ref FFF0S_ENABLE_REQ message
struct fcc0s_enable_req
{
    /// connection index
    uint8_t  conidx;
    /// Notification Configuration
    uint8_t  ntf_cfg;
    /// Old FFF1 Level used to decide if notification should be triggered
    uint8_t  old_fcc1_lvl;
};

/// Parameters of the @ref FFF0S_FFF2_WRITER_REQ_IND message
struct fcc0s_fcc1_writer_ind
{
    /// Connection index
    uint8_t conidx;

    uint8_t fcc1_value[FCC0_FCC1_DATA_LEN];
	
	uint8_t length;
};

///Parameters of the @ref FFF0S_BATT_LEVEL_UPD_REQ message
struct fcc0s_fcc2_level_ntf_ind
{
    /// Connection index
    uint8_t conidx;
	
	uint8_t length;
    
    uint8_t fcc2_value[FCC0_FCC2_DATA_LEN];
};

struct fcc0s_fcc3_level_ntf_ind
{
    /// Connection index
    uint8_t conidx;
	
	uint8_t length;
    
    uint8_t fcc3_value[FCC0_FCC3_DATA_LEN];
};


///Parameters of the @ref BASS_BATT_LEVEL_NTF_CFG_IND message
struct fcc0s_fcc2_level_ntf_cfg_ind
{
    /// connection index
    uint8_t  conidx;
    ///Notification Configuration
    uint8_t  ntf_cfg;
};

extern const struct ke_state_handler fcc0s_default_handler;
#endif // BLE_FFF0_SERVER

#endif /* _FFF0S_TASK_H_ */
