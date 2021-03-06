/**
 ****************************************************************************************
 *
 * @file app.c
 *
 * @brief Application entry point
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration

#if (BLE_APP_PRESENT)
#include <string.h>
#include "app_task.h"                // Application task Definition
#include "app.h"                     // Application Definition
#include "gap.h"                     // GAP Definition
#include "gapm_task.h"               // GAP Manager Task API
#include "gapc_task.h"               // GAP Controller Task API

#include "co_bt.h"                   // Common BT Definition
#include "co_math.h"                 // Common Maths Definition
#include "ke_timer.h"
#include "app_wlist.h"
#include "app_sec.h"                 // Application security Definition
#include "app_dis.h"                 // Device Information Service Application Definitions
#include "app_batt.h"                // Battery Application Definitions
#include "app_hid.h"                 // HID Application Definitions
#include "app_ota.h"                 // Application oads Definition
#include "nvds.h"                    // NVDS Definitions
#include "rf.h"
#include "uart.h"
#include "app_fcc0.h"      
//#include "adc.h"
//#include "wdt.h"
//#include "gpio.h"
/*
 * DEFINES
 ****************************************************************************************
 */
#define APP_DEVICE_NAME_LENGTH_MAX      (32+3)



/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef void (*appm_add_svc_func_t)(void);


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of service to add in the database
enum appm_svc_list
{
    //APPM_SVC_DIS,
    //APPM_SVC_BATT,
	//APPM_SVC_HIDS,
#ifdef BEKEN_OTA_BLE
    APPM_SVC_OTAS,
#endif
	APPM_SVC_FCC0,
    APPM_SVC_LIST_STOP ,
};

/*
 * LOCAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */

/// Application Task Descriptor
static const struct ke_task_desc TASK_DESC_APP = {NULL, &appm_default_handler,
                                                  appm_state, APPM_STATE_MAX, APP_IDX_MAX};

/// List of functions used to create the database
static const appm_add_svc_func_t appm_add_svc_func_list[APPM_SVC_LIST_STOP] =
{
    //(appm_add_svc_func_t)app_dis_add_dis,
    //(appm_add_svc_func_t)app_batt_add_bas,
	//(appm_add_svc_func_t)app_hid_add_hids,
#ifdef BEKEN_OTA_BLE
	(appm_add_svc_func_t)app_ota_add_otas,
#endif
    (appm_add_svc_func_t)app_fcc0_add_fcc0s,
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Environment Structure
struct app_env_tag app_env;

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void appm_init()
{	
	uint8_t peer_irk_len = NVDS_LEN_PEER_IRK;
    uint8_t key_len = KEY_LEN;

    // Reset the application manager environment
    memset(&app_env, 0, sizeof(app_env));

    // Create APP task
    ke_task_create(TASK_APP, &TASK_DESC_APP);

    // Initialize Task state
    ke_state_set(TASK_APP, APPM_INIT);

    if (nvds_get(NVDS_TAG_LOC_IRK, &key_len, app_env.loc_irk) != NVDS_OK)
    {
        uint8_t counter;

        // generate a new IRK
        for (counter = 0; counter < KEY_LEN; counter++)
        {
            app_env.loc_irk[counter]    = (uint8_t)co_rand_word();
        }

        // Store the generated value in NVDS
        if (nvds_put(NVDS_TAG_LOC_IRK, KEY_LEN, (uint8_t *)&app_env.loc_irk) != NVDS_OK)
        {
            ASSERT_INFO(0, NVDS_TAG_LOC_IRK, 0);
        }
    }
    // Store peer identity in NVDS
    if (nvds_get(NVDS_TAG_PEER_IRK, &peer_irk_len, (uint8_t *)&app_env.peer_irk.irk.key) != NVDS_OK)
    {
    	UART_PRINTF("not NVDS_TAG_PEER_IRK\r\n");				 
    }
    else
	{
	    int i;
		UART_PRINTF("appm irk.key = ");
		for(i = 0;i<sizeof(struct gap_sec_key);i++)
		{
			UART_PRINTF("0x%x ",app_env.peer_irk.irk.key[i]);
		}
		UART_PRINTF("\r\n");
					
		UART_PRINTF("appm addr type = %x\r\n",app_env.peer_irk.addr.addr_type);
		UART_PRINTF("appm addr.addr = ");
		for(i = 0;i<sizeof(struct bd_addr);i++)
		{
			UART_PRINTF("0x%x ", app_env.peer_irk.addr.addr.addr[i]);
		}	
		UART_PRINTF("\r\n");
	}
					 			
    /*------------------------------------------------------
     * INITIALIZE ALL MODULES
     *------------------------------------------------------*/ 
	// Security Module
    app_sec_init(); 
	
#if BLE_DIS_SERVER
    app_dis_init();    // Device Information Module
#endif

#if BLE_BATT_SERVER
    app_batt_init();   // Battery Module
#endif

#ifdef BEKEN_OTA_BLE		
    app_otas_init();   // Beken ota Module	
#endif

#if (BLE_FCC0_SERVER)
    app_fcc0_init();   // User develop Module
#endif
}

bool appm_add_svc(void)
{
    // Indicate if more services need to be added in the database
    bool more_svc = false;

    // Check if another should be added in the database
    if (app_env.next_svc != APPM_SVC_LIST_STOP)
    {
        ASSERT_INFO(appm_add_svc_func_list[app_env.next_svc] != NULL, app_env.next_svc, 1);

        // Call the function used to add the required service
        appm_add_svc_func_list[app_env.next_svc]();

        // Select following service to add
        app_env.next_svc++;
        more_svc = true;
    }

    return more_svc;
}


/*设备主动断开连接函数*/
void appm_disconnect(void)
{
    struct gapc_disconnect_cmd *cmd = KE_MSG_ALLOC(GAPC_DISCONNECT_CMD,
                                                   KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                                                   gapc_disconnect_cmd);

    cmd->operation = GAPC_DISCONNECT;
    cmd->reason    = CO_ERROR_REMOTE_USER_TERM_CON;

    // Send the message
    ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * Advertising Functions
 ****************************************************************************************
 */

/* 设备发起广播函数*/
void appm_start_advertising(void)
{
    // Check if the advertising procedure is already is progress
    if (ke_state_get(TASK_APP) == APPM_READY)
    {				
        // Prepare the GAPM_START_ADVERTISE_CMD message
        struct gapm_start_advertise_cmd *cmd = KE_MSG_ALLOC(GAPM_START_ADVERTISE_CMD,
                                                            TASK_GAPM, TASK_APP,
                                                            gapm_start_advertise_cmd);

        cmd->op.addr_src    = GAPM_STATIC_ADDR;
        cmd->channel_map    = APP_ADV_CHMAP;
        cmd->intv_min 		= APP_ADV_INT_MIN;
        cmd->intv_max 		= APP_ADV_INT_MAX;	
        cmd->op.code        = GAPM_ADV_UNDIRECT;
		
        cmd->info.host.mode = GAP_GEN_DISCOVERABLE;

 		/*-----------------------------------------------------------------------------------
         * Set the Advertising Data and the Scan Response Data
         *---------------------------------------------------------------------------------*/
        // Flag value is set by the GAP
        cmd->info.host.adv_data_len       = ADV_DATA_LEN;
        cmd->info.host.scan_rsp_data_len  = SCAN_RSP_DATA_LEN;

        // Advertising Data
        if(nvds_get(NVDS_TAG_APP_BLE_ADV_DATA, &cmd->info.host.adv_data_len,
                    &cmd->info.host.adv_data[0]) != NVDS_OK)
        {
            cmd->info.host.adv_data[0] = 2;// Length of ad type flags
            cmd->info.host.adv_data[1] = GAP_AD_TYPE_FLAGS;
            cmd->info.host.adv_data[2] = GAP_SIMUL_BR_EDR_LE_HOST|GAP_SIMUL_BR_EDR_LE_CONTROLLER;
            // set mode in ad_type
            switch(cmd->info.host.mode)
            {
                // General discoverable mode
                case GAP_GEN_DISCOVERABLE:
                {
                    cmd->info.host.adv_data[2] |= GAP_LE_GEN_DISCOVERABLE_FLG;
                }
                break;
                // Limited discoverable mode
                case GAP_LIM_DISCOVERABLE:
                {
                    cmd->info.host.adv_data[2] |= GAP_LE_LIM_DISCOVERABLE_FLG;
                }
                break;
                default: break; // do nothing
            }
            cmd->info.host.adv_data_len = 3;
           
            //Add list of UUID and appearance
            //memcpy(&cmd->info.host.adv_data[cmd->info.host.adv_data_len], APP_HID_ADV_DATA_UUID, APP_HID_ADV_DATA_UUID_LEN);
            //cmd->info.host.adv_data_len += APP_HID_ADV_DATA_UUID_LEN;
        }

		
        //  Device Name Length
        uint8_t device_name_length;
        uint8_t device_name_avail_space;
        uint8_t device_name_temp_buf[APP_DEVICE_NAME_LENGTH_MAX];


        // Get remaining space in the Advertising Data - 2 bytes are used for name length/flag
        device_name_avail_space = ADV_DATA_LEN  - cmd->info.host.adv_data_len - 2;
        // Check if data can be added to the Advertising data
        if (device_name_avail_space > 2)
        {
            device_name_length = APP_DEVICE_NAME_LENGTH_MAX;//NVDS_LEN_DEVICE_NAME;
            if (nvds_get(NVDS_TAG_DEVICE_NAME, &device_name_length,
                         &device_name_temp_buf[0]) != NVDS_OK)
            {
                device_name_length = strlen(APP_HID_DEVICE_NAME);
                // Get default Device Name (No name if not enough space)
                memcpy(&device_name_temp_buf[0], APP_HID_DEVICE_NAME, device_name_length);
            }
				
	     	if(device_name_length > 0)
            {
                // Check available space
                device_name_length = co_min(device_name_length, device_name_avail_space);
                cmd->info.host.adv_data[cmd->info.host.adv_data_len]     = device_name_length + 1;
								 
                // Fill Device Name Flag
                cmd->info.host.adv_data[cmd->info.host.adv_data_len + 1] = '\x09';      // complete local name
                // Copy device name
                memcpy(&cmd->info.host.adv_data[cmd->info.host.adv_data_len + 2],
                device_name_temp_buf, device_name_length);

                // Update Advertising Data Length
                cmd->info.host.adv_data_len += (device_name_length + 2);
            }
          
        }

        // Scan Response Data
        if(nvds_get(NVDS_TAG_APP_BLE_SCAN_RESP_DATA, &cmd->info.host.scan_rsp_data_len,
                    &cmd->info.host.scan_rsp_data[0]) != NVDS_OK)
        {
            if(device_name_length > 0)
            {
                device_name_length = co_min(device_name_length, SCAN_RSP_DATA_LEN);
                cmd->info.host.scan_rsp_data_len = 0;
                cmd->info.host.scan_rsp_data[cmd->info.host.scan_rsp_data_len] = device_name_length + 1;
                cmd->info.host.scan_rsp_data_len ++;
                cmd->info.host.scan_rsp_data[cmd->info.host.scan_rsp_data_len] = '\x08';  // shorten local name
                cmd->info.host.scan_rsp_data_len ++;
    			memcpy(&cmd->info.host.scan_rsp_data[cmd->info.host.scan_rsp_data_len],
                device_name_temp_buf, device_name_length);
                cmd->info.host.scan_rsp_data_len += device_name_length;
            }
            else
            {
                cmd->info.host.scan_rsp_data_len = 0;
    			memcpy(&cmd->info.host.scan_rsp_data[cmd->info.host.scan_rsp_data_len],
                       APP_SCNRSP_DATA, APP_SCNRSP_DATA_LEN);
                cmd->info.host.scan_rsp_data_len += APP_SCNRSP_DATA_LEN;
            }
            
        }

        // Send the message
        ke_msg_send(cmd);
	 	UART_PRINTF("appm start advertising\r\n");
		
        // Set the state.
        ke_state_set(TASK_APP, APPM_ADVERTISING);	
    }
	
    // else ignore the request
}


/* 设备主动停止广播函数*/
void appm_stop_advertising(void)
{
    if (ke_state_get(TASK_APP) == APPM_ADVERTISING)
    {
        // Stop the advertising timer if needed
        if (ke_timer_active(APP_ADV_TIMEOUT_TIMER, TASK_APP))
        {
            ke_timer_clear(APP_ADV_TIMEOUT_TIMER, TASK_APP);
        }

        // Go in ready state
        ke_state_set(TASK_APP, APPM_READY);

        // Prepare the GAPM_CANCEL_CMD message
        struct gapm_cancel_cmd *cmd = KE_MSG_ALLOC(GAPM_CANCEL_CMD,
                                                   TASK_GAPM, TASK_APP,
                                                   gapm_cancel_cmd);
        cmd->operation = GAPM_CANCEL;

        // Send the message
        ke_msg_send(cmd);
		UART_PRINTF("appm stop advertising\r\n");

		//wdt_disable_flag = 1;
		
    }
    // else ignore the request
}

void appm_update_param(uint16_t intv_min, uint16_t intv_max, uint16_t latency, uint16_t time_out)
{
    // Prepare the GAPC_PARAM_UPDATE_CMD message
    struct gapc_param_update_cmd *cmd = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CMD,
                                                     KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                                                     gapc_param_update_cmd);

    cmd->operation  = GAPC_UPDATE_PARAMS;
    cmd->intv_min   = intv_min;
    cmd->intv_max   = intv_max;
    cmd->latency    = latency;
    cmd->time_out   = time_out;

    // not used by a slave device
    cmd->ce_len_min = 0xFFFF;
    cmd->ce_len_max = 0xFFFF;
		
    UART_PRINTF("intv_min = %d,intv_max = %d,latency = %d,time_out = %d\r\n",
		cmd->intv_min,cmd->intv_max,cmd->latency,cmd->time_out);
	
    // Send the message
    ke_msg_send(cmd);
}

void appm_send_seurity_req(void)
{
	UART_PRINTF("%s \r\n",__func__);
    app_sec_send_security_req(app_env.conidx);
}

uint8_t appm_get_dev_name(uint8_t* name)
{
    // copy name to provided pointer
    memcpy(name, app_env.dev_name, app_env.dev_name_len);
    // return name length
    return app_env.dev_name_len;
}

bool appm_ble_is_connection(void)
{
    if(ke_state_get(TASK_APP) == APPM_CONNECTED)
        return 1;
    else
        return 0;
}

#endif //(BLE_APP_PRESENT)


