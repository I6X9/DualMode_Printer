/**
 **************************************************************************************
 * @file    drv_tl420.h
 * @brief   Driver for the DSP CEVA-TL420 which is embeded @ BK3268
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2017 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __DRV_TL420_H__
#define __DRV_TL420_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "types.h"

enum
{
    DSP_CLK_PLL_0,
    DSP_CLK_PLL_1,
};

enum
{
    DSP_CLK_DIV_1,
    DSP_CLK_DIV_2,
    DSP_CLK_DIV_4,
    DSP_CLK_DIV_8,
};

#define ADR_DSP_CODE_AREA       (0x00B00000)
#define ADR_DSP_DATA_AREA       (0x00C00000)
#define ADR_DSP_DAT1_AREA       (0x00E00000)

#define ADR_DSP_CLK		        (0x00800000 + 8 * 4)
#define REG_DSP_CLK		        (*((volatile unsigned int*)(ADR_DSP_CLK)))
#define SFT_DSP_CLK_HALT        (0) //1: DSP Halt, 0: DSP Going (中断产生，自动清零）
#define SFT_DSP_CLK_DIVID       (1) //00: 1, 01: 2, 10: 4, 11:8
#define SFT_DSP_CLK_SELECT      (3) //0: PLL0, 1: PLL1
#define MSK_DSP_CLK_HALT        (1 << SFT_DSP_CLK_HALT)
#define MSK_DSP_CLK_DIVID       (3 << SFT_DSP_CLK_DIVID)
#define MSK_DSP_CLK_SELECT      (1 << SFT_DSP_CLK_SELECT)

#define ADR_DSP_CFG             (0x00F90000 + 0 * 4)
#define REG_DSP_CFG             (*((volatile unsigned int*)(ADR_DSP_CFG)))
#define SFT_DSP_CFG_RESET       (0) //DSP RESET , defult is low.
#define SFT_DSP_CFG_ORESET      (1) //DSP RESET except OCEM , defualt is low
#define SFT_DSP_CFG_BOOT        (2) //0: DSP boot form 0x0000_0000., 1: DSP boot from DSP_VECTOR address
#define SFT_DSP_CFG_EXT_WAIT    (3) //DSP External Wait, default is low
#define SFT_DSP_CFG_VCNTX_EN    (4) //Enable vector interrupt, default is low (disable)
#define SFT_DSP_CFG_CACHE_INV   (5) //Memory Cache Invalidate Strap
#define SFT_DSP_CFG_PMSS_MODE   (6) //PMSS mode select, 0: Program TCM use 160KB, 1: Program Cache 64KB, Progam TCM 96KB
#define SFT_DSP_CFG_HRST		(7)
#define MSK_DSP_CFG_RESET       (1 << SFT_DSP_CFG_RESET)
#define MSK_DSP_CFG_ORESET      (1 << SFT_DSP_CFG_ORESET)
#define MSK_DSP_CFG_BOOT        (1 << SFT_DSP_CFG_BOOT)
#define MSK_DSP_CFG_EXT_WAIT    (1 << SFT_DSP_CFG_EXT_WAIT)
#define MSK_DSP_CFG_VCNTX_EN    (1 << SFT_DSP_CFG_VCNTX_EN)
#define MSK_DSP_CFG_CACHE_INV   (1 << SFT_DSP_CFG_CACHE_INV)
#define MSK_DSP_CFG_PMSS_MODE   (1 << SFT_DSP_CFG_PMSS_MODE)
#define MSK_DSP_CFG_HRST	    (1 << SFT_DSP_CFG_HRST)

#define ADR_DSP_INT             (0x00F90000 + 1 * 4)
#define REG_DSP_INT             (*((volatile unsigned int*)(ADR_DSP_INT)))
#define SFT_DSP_INT_VINT        (0) //Request DSP vector interrupt
#define SFT_DSP_INT_NMI         (1) //Non-maskable interrupt
#define SFT_DSP_INT_CORE_RCVR   (2) //Recover from STAND BY/LIGHT SLEEP modes
#define MSK_DSP_INT_VINT        (1 << SFT_DSP_INT_VINT)
#define MSK_DSP_INT_NMI         (1 << SFT_DSP_INT_NMI)
#define MSK_DSP_INT_CORE_RCVR   (1 << SFT_DSP_INT_CORE_RCVR)

/**
 * @brief DSP address if the vector interrupt
 * When DSP reset de-assert, If DSP_BOOT = 1, DSP boot from DSP_VECTOR address
 * Else, DSP boot form 0x0000_0000.
 * When DSP_VINT assert, TL420 will context switch to DSP_VECTOR address.
 */
#define ADR_DSP_VECTOR          (0x00F90000 + 2 * 4)
#define REG_DSP_VECTOR          (*((volatile unsigned int*)(ADR_DSP_VECTOR)))

/**
 * @brief DSP POWER control, default is power on
 * 0: POWER DOWN
 * 1: POWER ON
 */
#define ADR_DSP_POWER           (0x00F90000 + 3 * 4)
#define ADR_DSP_HALT            (0x00F90000 + 11 * 4)
#define REG_DSP_POWER           (*((volatile unsigned int*)(ADR_DSP_POWER)))
#define REG_DSP_HALT            (*((volatile unsigned int*)(ADR_DSP_HALT)))            

//TODO

void tl420_boot(uint32_t type);
void start_dsp(void);
void shutdown_dsp(void);
void dsp_halt_clk(void);
void dsp_restart_clk(void);
#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__DRV_TL420_H__
