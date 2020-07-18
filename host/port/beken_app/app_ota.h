#ifndef _APP_OTA_H_
#define _APP_OTA_H_

#include "config.h"
#include "types.h"

#ifdef BEKEN_OTA
/* ota flash operation addr */
#define OTA_NVDS_ADDR            0x002000
#define OTA_MCU_ADDR             0x005000
#define OTA_DSP_ADDR             0x0C2000
#define OTA_BACKUP_ADDR          0x142000
#define OTA_END_ADDR             0x200000

/* ota location mark */
#define OTA_MARK_INIT            0xFFFF
#define OTA_MARK_ONGO            0xFF55
#define OTA_MARK_FAIL            0x0000
#define OTA_MARK_SUCC            0x5555

/* image flag */
enum image_flag
{
    IMAGE_MCU        =   0x01,
    IMAGE_MCU_ADDR   =   0x02,
    IMAGE_DSP        =   0x03,
};

void app_ota_erase_flash(void);
void app_ota_write_flash(void);
#ifdef BEKEN_OTA_BLE
void app_otas_init(void);
void app_ota_add_otas(void);
extern const struct ke_state_handler app_otas_table_handler;
#endif
#ifdef BEKEN_OTA_SPP
void app_ota_spp_pkt_reframe(uint8 *pValue, uint16_t length);
#endif
void app_ota_update_size_req(uint16 size);
#endif
uint8 app_ota_is_ongoing(void);
uint8 app_ota_tx_arqn_nak_flag_get(void);
void app_ota_tx_arqn_nak_flag_set(uint8 value);

#endif
