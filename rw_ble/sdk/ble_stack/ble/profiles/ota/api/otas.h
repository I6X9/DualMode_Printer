#ifndef _OTAS_H_
#define _OTAS_H_

#include "rwip_config.h"

#if (BLE_OTAS_SERVER)
#include "otas_task.h"
#include "atts.h"
#include "prf_types.h"
#include "prf.h"
#include "rwprf_config.h"

#define OTA_SERVICE_UUID       0xFFC0
#define OTA_RX_UUID            0xFFC1
#define OTA_TX_UUID            0xFFC2

enum
{
    ATT_USER_SERVER_SVC_OAD			= ATT_UUID_16(OTA_SERVICE_UUID),
    ATT_USER_SERVER_CHAR_RX			= ATT_UUID_16(OTA_RX_UUID),
    ATT_USER_SERVER_CHAR_TX			= ATT_UUID_16(OTA_TX_UUID),	
};

enum
{
    OTAS_IDX_SVC,       

	OTAS_IDX_RX_CHAR, 
	OTAS_IDX_RX_VAL,
	
	OTAS_IDX_TX_CHAR,  
	OTAS_IDX_TX_VAL,
	OTAS_IDX_TX_CFG,

	OTAS_IDX_NB,
};

struct otas_env_tag
{
	/// profile environment
	prf_env_t prf_env;
	/// On-going operation
	struct ke_msg * operation;
	///  Services Start Handle
	uint16_t otas_start_hdl; 
	/// OADS task state
	ke_state_t state[OTAS_IDX_MAX];
	/// Notification configuration of peer devices.
	uint16_t tx_ntf_cfg[BLE_CONNECTION_MAX];
	/// Database features
	uint8_t features;

	uint8_t cursor;
};

const struct prf_task_cbs* otas_prf_itf_get(void);

#endif /* #if (BLE_OTAS_SERVER) */
#endif /*  _OTAS_H_ */
