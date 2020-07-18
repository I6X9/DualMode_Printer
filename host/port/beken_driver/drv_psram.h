/**
 **************************************************************************************
 * @file    drv_psram.h
 * @brief   Driver API for PSRAM
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2018 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __DRV_PSRAM_H__
#define __DRV_PSRAM_H__

#include "types.h"
//#include "stdint.h"

typedef enum
{
    PSRAM_PIN_SPEC,
    PSRAM_PIN_GPIO,
}PSRAM_PIN;

typedef enum
{
    PSRAM_CLK_PLL0_DIV1,
    PSRAM_CLK_PLL0_DIV2,
    PSRAM_CLK_PLL0_DIV4,
	PSRAM_CLK_PLL0_DIV8,
    PSRAM_CLK_PLL0_DIV16,
    PSRAM_CLK_PLL0_DIV32,
    PSRAM_CLK_PLL0_DIV64,
	PSRAM_CLK_PLL0_DIV128,
	PSRAM_CLK_PLL1_DIV1,
	PSRAM_CLK_PLL1_DIV2,
	PSRAM_CLK_PLL1_DIV4,
	PSRAM_CLK_PLL1_DIV8,
	PSRAM_CLK_PLL1_DIV16,
	PSRAM_CLK_PLL1_DIV32,
	PSRAM_CLK_PLL1_DIV64,
	PSRAM_CLK_PLL1_DIV128,
}PSRAM_CLK;

typedef enum
{
    PSRAM_MODE_LINE_1,
    PSRAM_MODE_LINE_4,
}PSRAM_MODE;

void psram_init(uint32_t pin, uint32_t clk, uint32_t mode);

#endif//__DRV_PSRAM_H__
