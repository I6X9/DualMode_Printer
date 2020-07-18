#ifndef _OTAS_TASK_H_
#define _OTAS_TASK_H_

#include "rwprf_config.h"
#if (BLE_OTAS_SERVER)
#include "types.h"
#include "prf_types.h"
#include "rwip_task.h" // Task definitions

///Maximum number of  Server task instances
#define OTAS_IDX_MAX  0x01
///Maximal number of BRACES that can be added in the DB
#define  OTAS_RX_LEN  550
#define  OTAS_TX_LEN  50

/// Possible states of the braces task
enum otas_state
{
    /// Idle state
    OTAS_IDLE,
    /// busy state
    OTAS_BUSY,
    /// Number of defined states.
    OTAS_STATE_MAX
};

/// Messages for BRACELET Server
enum otas_msg_id
{
	/// Start the Bracelet profile - at connection used 
	OTAS_RX = TASK_FIRST_MSG(TASK_ID_OTAS),

    OTAS_TX,
};

/// Features Flag Masks
enum otas_features
{
    ///  Characteristic doesn't support notifications
    OTAS_NTF_NOT_SUP,
    
    OTAS_NTF_SUP,
};

/// Parameters for the database creation
struct otas_db_cfg
{
    /// Number of  to add
    uint8_t ota_nb;
    /// Features of each  instance
    uint8_t features;
};

///Parameters of the @ref OTAS_rX_PDU format
struct otas_rx_pdu
{	
	uint16_t length;
    /// data
    uint8_t data[OTAS_RX_LEN];
};

///Parameters of the @ref OTAS_TX_PDU format
struct otas_tx_pdu
{	
	uint16_t length;
    /// data
    uint8_t data[OTAS_TX_LEN];
};

extern const struct ke_state_handler otas_default_handler;
#endif // BLE_BRACELET_REPORTER

#endif /* _BRACES_TASK_H_ */
