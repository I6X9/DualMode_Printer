/**
 **************************************************************************************
 * @file    drv_tl420.c
 * @brief   Driver for the DSP CEVA-TL420 which is embeded @ BK3268
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2017 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#include "types.h"
#include "bk3000_reg.h"
#include "drv_tl420.h"

#define USE_PSRAM_AS_DSP_EXT_DATA	(0)
#if     USE_PSRAM_AS_DSP_EXT_DATA
#include "drv_psram.h"
#endif

int32 os_printf(const char *fmt, ...);

void dsp_code_data_clear(uint32_t* adr, uint32_t len)
{
	uint32_t i;

    for(i = 0; i < len; i++)
    {
        *adr++ = 0;
    }
}

void dsp_code_data_copy(uint32_t* dst, uint32_t* src, uint32_t len)
{
	uint32_t i;

    for(i = 0; i < len; i++)
    {
        *dst++ = *src++;
    }
}

#if 0

#include "tl420_code_data.h"

void tl420_boot(uint32_t type)
{
    //os_printf("[MCU]: DSP addr(CODE0) = 0x%08X, sizeof(CODE0) = %d\r\n", tl420_code0, sizeof(tl420_code0));
    //os_printf("[MCU]: DSP addr(CODE1) = 0x%08X, sizeof(CODE1) = %d\r\n", tl420_code1, sizeof(tl420_code1));
    //os_printf("[MCU]: DSP addr(DATA0) = 0x%08X, sizeof(DATA0) = %d\r\n", tl420_data0, sizeof(tl420_data0));
    //os_printf("[MCU]: DSP addr(DATA1) = 0x%08X, sizeof(DATA1) = %d\r\n", tl420_data1, sizeof(tl420_data1));

    REG_DSP_POWER = 1;

	REG_DSP_CLK = (DSP_CLK_PLL_0 << SFT_DSP_CLK_SELECT) |
				  (DSP_CLK_DIV_1 << SFT_DSP_CLK_DIVID);

    //新版BK3268不能GATE时钟
	BK3000_PMU_GATE_CFG |= (1 << 20);

	if(type == 0)
	{
		REG_DSP_CFG = (1 << SFT_DSP_CFG_RESET) |
					  (1 << SFT_DSP_CFG_ORESET) |
					  (1 << SFT_DSP_CFG_BOOT) |
					  (1 << SFT_DSP_CFG_EXT_WAIT) |
					  (1 << SFT_DSP_CFG_VCNTX_EN) |
					  (0 << SFT_DSP_CFG_CACHE_INV) |
					  (0 << SFT_DSP_CFG_PMSS_MODE) |
					  (1 << SFT_DSP_CFG_HRST);
		REG_DSP_VECTOR = 0;
	}
	else
	{
		REG_DSP_CFG = (1 << SFT_DSP_CFG_RESET) |
					  (1 << SFT_DSP_CFG_ORESET) |
					  (1 << SFT_DSP_CFG_BOOT) |
					  (1 << SFT_DSP_CFG_EXT_WAIT) |
					  (1 << SFT_DSP_CFG_VCNTX_EN) |
					  (1 << SFT_DSP_CFG_CACHE_INV) |
					  (1 << SFT_DSP_CFG_PMSS_MODE) |
					  (1 << SFT_DSP_CFG_HRST);
		REG_DSP_VECTOR = (uint32_t)TL420_FLASH_BOOT_ADDRESS;

		if(TL420_FLASH_BOOT_ADDRESS != (uint32_t)tl420_code1 / 2)
		{
			os_printf("[MCU]: DSP flash code is not located @ the right address ()\r\n", TL420_FLASH_BOOT_ADDRESS, (uint32_t)tl420_code1 / 2);
		}
	}

	//REG_DSP_INT    = 0;
	//REG_DSP_POWER  = 1;

	//os_printf("[MCU]: Download DSP code and data\n");

	if(type == 0)
	dsp_code_data_copy((uint32_t*)ADR_DSP_CODE_AREA, (uint32_t*)tl420_code0, sizeof(tl420_code0) / 4);
	dsp_code_data_copy((uint32_t*)ADR_DSP_DATA_AREA, (uint32_t*)tl420_data0, sizeof(tl420_data0) / 4);

	os_printf("[MCU]: Start running DSP\r\n");

	REG_DSP_CFG &= ~((1 << SFT_DSP_CFG_EXT_WAIT));
}

#else

void tl420_boot(uint32_t type)
{
    uint32_t dsp_boot_addr = *(uint32_t*)0x4;

    if(dsp_boot_addr)
    {
        uint32_t* dsp_code_data_info = (uint32_t*)(dsp_boot_addr + 0x120);

        REG_DSP_POWER = 1;

        REG_DSP_CLK = (DSP_CLK_PLL_1 << SFT_DSP_CLK_SELECT) |
                      (DSP_CLK_DIV_1 << SFT_DSP_CLK_DIVID);

        //新版BK3268不能GATE时钟
        BK3000_PMU_GATE_CFG |= (1 << 20);

        type = dsp_code_data_info[4];

        if(type == 0)
        {
            REG_DSP_CFG = (1 << SFT_DSP_CFG_RESET) |
                          (1 << SFT_DSP_CFG_ORESET) |
                          (1 << SFT_DSP_CFG_BOOT) |
                          (1 << SFT_DSP_CFG_EXT_WAIT) |
                          (1 << SFT_DSP_CFG_VCNTX_EN) |
                          (0 << SFT_DSP_CFG_CACHE_INV) |
                          (0 << SFT_DSP_CFG_PMSS_MODE) |
                          (1 << SFT_DSP_CFG_HRST);
            REG_DSP_VECTOR = 0;
        }
        else
        {
            REG_DSP_CFG = (1 << SFT_DSP_CFG_RESET) |
                          (1 << SFT_DSP_CFG_ORESET) |
                          (1 << SFT_DSP_CFG_BOOT) |
                          (1 << SFT_DSP_CFG_EXT_WAIT) |
                          (1 << SFT_DSP_CFG_VCNTX_EN) |
                          (1 << SFT_DSP_CFG_CACHE_INV) |
                          (1 << SFT_DSP_CFG_PMSS_MODE) |
                          (1 << SFT_DSP_CFG_HRST);
            REG_DSP_VECTOR = 0;//dsp_code_data_info[4];

            if(dsp_code_data_info[4] != dsp_boot_addr / 2)
            {
                os_printf("[MCU]: DSP flash code is not located @ the right address ()\r\n", dsp_code_data_info[4], dsp_boot_addr / 2);
            }
        }

        //REG_DSP_INT    = 0;
        //REG_DSP_POWER  = 1;

        //os_printf("[MCU]: Download DSP code and data\n");

        //dsp_code_data_clear((uint32_t*)ADR_DSP_DATA_AREA, 64 * 1024 / 4);
        //dsp_code_data_clear((uint32_t*)ADR_DSP_DAT1_AREA, 64 * 1024 / 4);

        if(dsp_code_data_info[9])
        dsp_code_data_copy((uint32_t*)ADR_DSP_CODE_AREA, (uint32_t*)(dsp_boot_addr + dsp_code_data_info[8]), dsp_code_data_info[9] / 4);
        if(dsp_code_data_info[10])
        dsp_code_data_copy((uint32_t*)ADR_DSP_DATA_AREA, (uint32_t*)(dsp_boot_addr + dsp_code_data_info[8] + dsp_code_data_info[9]), dsp_code_data_info[10] / 4);
		#if USE_PSRAM_AS_DSP_EXT_DATA == 0
        if(dsp_code_data_info[11])
        dsp_code_data_copy((uint32_t*)ADR_DSP_DAT1_AREA, (uint32_t*)(dsp_boot_addr + dsp_code_data_info[8] + dsp_code_data_info[9] + dsp_code_data_info[10]), dsp_code_data_info[11] / 4);
		#else
        psram_init(PSRAM_PIN_GPIO, PSRAM_CLK_PLL0_DIV2, PSRAM_MODE_LINE_4);
        if(dsp_code_data_info[11])
        dsp_code_data_copy((uint32_t*)0x01000000, (uint32_t*)(dsp_boot_addr + dsp_code_data_info[8] + dsp_code_data_info[9] + dsp_code_data_info[10]), dsp_code_data_info[11] / 4);
        if(1)
        {
        	uint32_t* dst = (uint32_t*)0x01000000;
        	uint32_t* src = (uint32_t*)(dsp_boot_addr + dsp_code_data_info[8] + dsp_code_data_info[9] + dsp_code_data_info[10]);
        	uint32_t  len = dsp_code_data_info[11] / 4;
        	int i;
        	os_printf("[MCU]: psram double check\n");
        	for(i = 0; i < len; i++)
        	{
        		if(dst[i] != src[i])
        		{
        			os_printf("[%d]: %08X - %08X\n", i, dst[i], src[i]);
        		}
        	}
        }
        #endif

        os_printf("[MCU]: Start running DSP\r\n");
    #if 1 // set dac mic with dsp
        extern void aud_dac_mic_update_init(void);
        aud_dac_mic_update_init();
    #endif
        REG_DSP_CFG &= ~((1 << SFT_DSP_CFG_EXT_WAIT));
    }
    else
    {
        os_printf("[MCU]: No DSP firmware found\r\n");
    }
}
static uint8_t s_dsp_is_poweron = 0;
void start_dsp(void)
{
    //CLEAR_WDT;
    //os_delay_ms(1000);
    if(s_dsp_is_poweron) return;
    BK3000_PMU_PERI_PWDS &= ~((1 << 0) | (1 << 11)); /* MCU open DMA & AUD clk,because DSP can't controll this register */

   //CLEAR_WDT;
    tl420_boot(0);
    s_dsp_is_poweron = 1;

    //CLEAR_WDT;
    //os_delay_ms(100);
    //while(1) CLEAR_WDT;
}
void shutdown_dsp(void)
{
    if(!s_dsp_is_poweron) return;
    REG_DSP_CFG = 0;
    REG_DSP_POWER = 0; 
    s_dsp_is_poweron = 0;
   
}
void dsp_halt_clk(void)
{
    REG_DSP_HALT |= MSK_DSP_CLK_HALT;    
}
void dsp_restart_clk(void)
{
    REG_DSP_HALT &= ~MSK_DSP_CLK_HALT;
}

#endif
