/**
 ****************************************************************************************
 *
 * @file arch_main.c
 *
 * @brief Main loop of the application.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ******** ********************************************************************************
 */

 
/*
 * INCLUDES
 ****************************************************************************************
 */
 
#include "rwip_config.h" // RW SW configuration

#include "arch.h"      // architectural platform definitions
#include <stdlib.h>    // standard lib functions
#include <stddef.h>    // standard definitions
#include "types.h"    // standard integer definition
#include <stdbool.h>   // boolean definition
#include "boot.h"      // boot definition
#include "rwip.h"      // RW SW initialization
#include "uart.h"      	// UART initialization
#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
#include "rf.h"        // RF initialization
#endif // BLE_EMB_PRESENT || BT_EMB_PRESENT

#if (BLE_APP_PRESENT)
#include "app.h"       // application functions
#endif // BLE_APP_PRESENT

#include "nvds.h"         // NVDS definitions

#include "reg_assert_mgr.h"
#include "RomCallFlash.h"
#include "app_task.h"
#include "otas.h"
#include "driver_flash.h"
#include "app_beken_includes.h"

/***************CEVA_INCLUDE,added by yangyang,2018/6/7*************/
#include "config.h"
#include "bk3000_mcu.h"
/*******************************************************************/


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// Creation of uart external interface api
struct rwip_eif_api uart_api;


/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

static void Stack_Integrity_Check(void);


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
#if ((UART_PRINTF_EN) &&(UART_DRIVER))
void assert_err(const char *condition, const char * file, int line)
{
	os_printf("%s,condition %s,file %s,line = %d\r\n",__func__,condition,file,line);
  
}

void assert_param(int param0, int param1, const char * file, int line)
{
	os_printf("%s,param0 = %d,param1 = %d,file = %s,line = %d\r\n",__func__,param0,param1,file,line);
  
}

void assert_warn(int param0, int param1, const char * file, int line)
{
	 os_printf("%s,param0 = %d,param1 = %d,file = %s,line = %d\r\n",__func__,param0,param1,file,line);
 
}

void dump_data(uint8_t* data, uint16_t length)
{
	os_printf("%s,data = %d,length = %d,file = %s,line = %d\r\n",__func__,data,length);
 
}
#else
void assert_err(const char *condition, const char * file, int line)
{
  
}

void assert_param(int param0, int param1, const char * file, int line)
{
  
}

void assert_warn(int param0, int param1, const char * file, int line)
{
 
}

void dump_data(uint8_t* data, uint16_t length)
{
 
}
#endif //UART_PRINTF_EN

/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void platform_reset(uint32_t error)
{
    //void (*pReset)(void);
	UART_PRINTF("BLE---error = 0x%x\r\n", error);

    // Disable interrupts
    GLOBAL_INT_STOP();

    #if UART_PRINTF_EN
    // Wait UART transfer finished
    uart_finish_transfers();
    #endif //UART_PRINTF_EN


    if(error == RESET_AND_LOAD_FW || error == RESET_TO_ROM)
    {
        // Not yet supported
    }
    else
    {
        //Restart FW
        //pReset = (void * )(0x0);
        //pReset();
        //wdt_enable(10);
        //while(1);
    }
}

void emi_init(void)
{
	unsigned long *p=(unsigned long *)0x814000;
	int i;
	for(i=0;i<1000;i++)
	{
		*p++ =  0;					
	}
}

extern uint8_t system_sleep_flag;
void system_sleep_init(void)
{
	#if SYSTEM_SLEEP
	system_sleep_flag = 0x1;
	#else
	system_sleep_flag = 0x0;
	#endif
}

/**
 *******************************************************************************
 * @brief RW main function.
 *
 * This function is called right after the booting process has completed.
 *
 * @return status   exit status
 *******************************************************************************
 */

extern struct rom_env_tag rom_env;
void rwip_eif_api_init(void)
{
	uart_api.read = &uart_read;
	uart_api.write = &uart_write;
	uart_api.flow_on = &uart_flow_on;
	uart_api.flow_off = &uart_flow_off;
}

const struct rwip_eif_api* rwip_eif_get(uint8_t type)
{
    const struct rwip_eif_api* ret = NULL;
    switch(type)
    {
        case RWIP_EIF_AHI:
        {
            ret = &uart_api;
        }
        break;
        #if (BLE_EMB_PRESENT) || (BT_EMB_PRESENT)
        case RWIP_EIF_HCIC:
        {
            ret = &uart_api;
        }
        break;
        #elif !(BLE_EMB_PRESENT) || !(BT_EMB_PRESENT)
        case RWIP_EIF_HCIH:
        {
            ret = &uart_api;
        }
        break;
        #endif 
        default:
        {
            ASSERT_INFO(0, type, 0);
        }
        break;
    }
    return ret;
}

static void Stack_Integrity_Check(void)
{
	if ((REG_PL_RD(STACK_BASE_UNUSED)!= BOOT_PATTERN_UNUSED))
	{
		while(1)
		{
			uart_putchar("Stack_Integrity_Check STACK_BASE_UNUSED fail!\r\n");
		}
	}
	
	if ((REG_PL_RD(STACK_BASE_SVC)!= BOOT_PATTERN_SVC)) 
	{
		while(1)
		{
			uart_putchar("Stack_Integrity_Check STACK_BASE_SVC fail!\r\n");
		}
	}
	
	if ((REG_PL_RD(STACK_BASE_FIQ)!= BOOT_PATTERN_FIQ))
	{
		while(1)
		{
			uart_putchar("Stack_Integrity_Check STACK_BASE_FIQ fail!\r\n");
		}
	}
	
	if ((REG_PL_RD(STACK_BASE_IRQ)!= BOOT_PATTERN_IRQ))
	{
		while(1)
		{
			uart_putchar("Stack_Integrity_Check STACK_BASE_IRQ fail!\r\n");
		}
	}
	
}


void rom_env_init(struct rom_env_tag *api)
{
	memset(&rom_env,0,sizeof(struct rom_env_tag));
	rom_env.prf_get_id_from_task = prf_get_id_from_task;
	rom_env.prf_get_task_from_id = prf_get_task_from_id;
	rom_env.prf_init = prf_init;	
	rom_env.prf_create = prf_create;
	rom_env.prf_cleanup = prf_cleanup;
	rom_env.prf_add_profile = prf_add_profile;
	rom_env.rwble_hl_reset = rwble_hl_reset;
	rom_env.rwip_reset = rwip_reset;
#if SYSTEM_SLEEP		
	rom_env.rwip_prevent_sleep_set = rwip_prevent_sleep_set;
       rom_env.rwip_prevent_sleep_clear = rwip_prevent_sleep_clear;
	rom_env.rwip_sleep_lpcycles_2_us = rwip_sleep_lpcycles_2_us;
	rom_env.rwip_us_2_lpcycles = rwip_us_2_lpcycles;
	rom_env.rwip_wakeup_delay_set = rwip_wakeup_delay_set;
#endif	
	rom_env.platform_reset = platform_reset;
	rom_env.assert_err = assert_err;
	rom_env.assert_param = assert_param;
	rom_env.Read_Uart_Buf = Read_Uart_Buf;
	rom_env.uart_clear_rxfifo = uart_clear_rxfifo;
	
}

#ifdef BT_DUALMODE_RW
void rw_ble_init(void)
{
    // Initialize random process
    struct bd_addr ble_addr;
    struct bd_name ble_name;


    srand(1);
    
    //get System sleep flag
    system_sleep_init();

    rwip_eif_api_init();

    // Initialize NVDS module
    struct nvds_env_tag env;
    env.flash_read = &flash_read;
    env.flash_write = &flash_write;
    env.flash_erase = &flash_erase;
    nvds_init(env);
    memcpy((uint8_t *)&ble_addr,(uint8_t *)app_env_local_bd_addr(),6);
    memcpy((uint8_t *)&(ble_name.name[0]),(uint8_t *)app_env_local_bd_name(),32);
	
	ble_name.namelen = strlen((char *)&(ble_name.name[0]));

    nvds_put(NVDS_TAG_BD_ADDRESS,6,(uint8_t *)&ble_addr);
    nvds_put(NVDS_TAG_DEVICE_NAME,ble_name.namelen,(uint8_t *)&(ble_name.name[0]));
    rom_env_init(&rom_env);
    
    // Initialize RW SW stack
    rwip_init(0);
}

void rw_ble_schedule(void)
{
#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#endif

    rwip_schedule();
    
#if SYSTEM_SLEEP
    if(1)
    {	
        uint8_t sleep_type = 0;

        // Check if the processor clock can be gated
        GLOBAL_INT_DISABLE();
        sleep_type = rwip_sleep();
        GLOBAL_INT_RESTORE();

        if((sleep_type & RW_MCU_DEEP_SLEEP) == RW_MCU_DEEP_SLEEP)
        {	
        //os_printf("RW_MCU_DEEP_SLEEP!\r\n");
        }
        else if((sleep_type & RW_MCU_IDLE_SLEEP) == RW_MCU_IDLE_SLEEP)
        {
        //os_printf("RW_MCU_IDLE_SLEEP!\r\n");
        }
        else if((sleep_type & RW_NO_SLEEP) == RW_NO_SLEEP)
        {
        //os_printf("RW_NO_SLEEP!\r\n");
        }
    }	
#endif

#if 0//(CONFIG_CPU_CLK_OPTIMIZATION == 1)
    if(app_is_bt_mode())
        BK3000_set_clock(CPU_CLK_SEL,CPU_OPTIMIZATION_DIV);
#endif
}
#endif
/// @} DRIVERS
