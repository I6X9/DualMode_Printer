/**
 ****************************************************************************************
 *
 * @file app_ffc0.c
 *
 * @brief findt Application Module entry point
 *
 * @auth  HAI.XU
 *
 * @date  2016.05.31
 *
 * Copyright (C) Beken 2009-2016
 *
 *
 ****************************************************************************************
 */
#ifndef APP_FCC0_H_
#define APP_FCC0_H_
#if (BLE_FCC0_SERVER)
#include "rwip_config.h"      // SW configuration
#include "types.h"            //Integer Definition
#include "ke_task.h"          // Kernel Task Definition

/// fff0s Application Module Environment Structure
struct app_fcc0_env_tag
{
    /// Connection handle
    uint8_t conidx;
};

/// fff0s Application environment
extern struct app_fcc0_env_tag app_fcc0_env;
/// Table of message handlers
extern const struct ke_state_handler app_fcc0_table_handler;

void app_fcc0_init(void);
void app_fcc0_add_fcc0s(void);
void app_ff01_send_data(uint8_t* buf, uint8_t len);
void app_ff03_send_data(uint8_t* buf, uint8_t len);

#endif
#endif // APP_BATT_H_
