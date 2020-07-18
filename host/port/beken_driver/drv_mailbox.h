/**
 **************************************************************************************
 * @file    drv_tmailbox.c
 * @brief   Driver for the mailbox between MCU and DSP
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2017 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __DRV_MAILBOX_H__
#define __DRV_MAILBOX_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "types.h"
#define MAILBOX_BASEADDR        0x00F88000
#define MAILBOX_OFFSET          32
#define MAILBOX_CMD_SEG         0
#define MAILBOX_INTR_SEG        4
#define MAILBOX_P1_SEG          8
#define MAILBOX_P2_SEG          12
#define MAILBOX_P3_SEG          16

#define MAILBOX_M2D_INTR_STATUS  (64)     
#define MAILBOX_D2M_INTR_STATUS  (192)

#define MAILBOX_INTR1_SFT        (0)
#define MAILBOX_INTR2_SFT        (1)
#define MAILBOX_INTR1_MSK        (1 << MAILBOX_INTR1_SFT)
#define MAILBOX_INTR2_MSK        (1 << MAILBOX_INTR1_SFT)

#define MCU2DSP_DAC_CTRL        0x00
#define MCU2DSP_ADC_CTRL        0x01
#define DSP2MCU_ADC_CTRL        0x04
#define DSP2MCU_DAC_CTRL        0x05


#define PARAM_TYPE_VOLUME_HFP               0x00
#define PARAM_TYPE_VOLUME_A2DP              0x01
#define PARAM_TYPE_VOLUME_WAV               0x02
#define PARAM_TYPE_VOLUME_LINE              0x03
#define PARAM_TYPE_VOLUME_MIC               0x04

#define PARAM_TYPE_DAC_DIFF_EN              0x10
#define PARAM_TYPE_ADC_DIFF_EN              0x11

#define PARAM_TYPE_DAC_FAKE_DIFF            0x20


#define ADDA_ENABLE                 1
#define ADDA_DISABLE                0

#define ADR_MAILBOX_BASE                (0x00F88000)
#define ADR_MAILBOX_MCU2DSP1_CMD        (ADR_MAILBOX_BASE + 0 * 4)
#define REG_MAILBOX_MCU2DSP1_CMD        (*((volatile unsigned int*)(ADR_MAILBOX_MCU2DSP1_CMD)))
#define ADR_MAILBOX_MCU2DSP1_FLAG       (ADR_MAILBOX_BASE + 1 * 4)
#define REG_MAILBOX_MCU2DSP1_FLAG       (*((volatile unsigned int*)(ADR_MAILBOX_MCU2DSP1_FLAG)))
#define ADR_MAILBOX_MCU2DSP1_PARAM0     (ADR_MAILBOX_BASE + 2 * 4)
#define REG_MAILBOX_MCU2DSP1_PARAM0     (*((volatile unsigned int*)(ADR_MAILBOX_MCU2DSP1_PARAM0)))
#define ADR_MAILBOX_MCU2DSP1_PARAM1     (ADR_MAILBOX_BASE + 3 * 4)
#define REG_MAILBOX_MCU2DSP1_PARAM1     (*((volatile unsigned int*)(ADR_MAILBOX_MCU2DSP1_PARAM1)))
#define ADR_MAILBOX_MCU2DSP1_PARAM2     (ADR_MAILBOX_BASE + 4 * 4)
#define REG_MAILBOX_MCU2DSP1_PARAM2     (*((volatile unsigned int*)(ADR_MAILBOX_MCU2DSP1_PARAM2)))
#define ADR_MAILBOX_MCU2DSP2_CMD        (ADR_MAILBOX_BASE + 8 * 4)
#define REG_MAILBOX_MCU2DSP2_CMD        (*((volatile unsigned int*)(ADR_MAILBOX_MCU2DSP2_CMD)))
#define ADR_MAILBOX_MCU2DSP2_FLAG       (ADR_MAILBOX_BASE + 9 * 4)
#define REG_MAILBOX_MCU2DSP2_FLAG       (*((volatile unsigned int*)(ADR_MAILBOX_MCU2DSP2_FLAG)))
#define ADR_MAILBOX_MCU2DSP2_PARAM0     (ADR_MAILBOX_BASE + 10 * 4)
#define REG_MAILBOX_MCU2DSP2_PARAM0     (*((volatile unsigned int*)(ADR_MAILBOX_MCU2DSP2_PARAM0)))
#define ADR_MAILBOX_MCU2DSP2_PARAM1     (ADR_MAILBOX_BASE + 11 * 4)
#define REG_MAILBOX_MCU2DSP2_PARAM1     (*((volatile unsigned int*)(ADR_MAILBOX_MCU2DSP2_PARAM1)))
#define ADR_MAILBOX_MCU2DSP2_PARAM2     (ADR_MAILBOX_BASE + 12 * 4)
#define REG_MAILBOX_MCU2DSP2_PARAM2     (*((volatile unsigned int*)(ADR_MAILBOX_MCU2DSP2_PARAM2)))

#define ADR_MAILBOX_MCU2DSP_INT_STATUS  (ADR_MAILBOX_BASE + 16 * 4)
#define REG_MAILBOX_MCU2DSP_INT_STATUS  (*((volatile unsigned int*)(ADR_MAILBOX_MCU2DSP_INT_STATUS)))
#define SFT_MAILBOX_MCU2DSP_INT1        (0)
#define SFT_MAILBOX_MCU2DSP_INT2        (1)
#define MSK_MAILBOX_MCU2DSP_INT1        (1 << SFT_MAILBOX_MCU2DSP_INT1)
#define MSK_MAILBOX_MCU2DSP_INT2        (1 << SFT_MAILBOX_MCU2DSP_INT2)

#define ADR_MAILBOX_DSP2MCU1_CMD        (ADR_MAILBOX_BASE + 32 * 4)
#define REG_MAILBOX_DSP2MCU1_CMD        (*((volatile unsigned int*)(ADR_MAILBOX_DSP2MCU1_CMD)))
#define ADR_MAILBOX_DSP2MCU1_FLAG       (ADR_MAILBOX_BASE + 33 * 4)
#define REG_MAILBOX_DSP2MCU1_FLAG       (*((volatile unsigned int*)(ADR_MAILBOX_DSP2MCU1_FLAG)))
#define ADR_MAILBOX_DSP2MCU1_PARAM0     (ADR_MAILBOX_BASE + 34 * 4)
#define REG_MAILBOX_DSP2MCU1_PARAM0     (*((volatile unsigned int*)(ADR_MAILBOX_DSP2MCU1_PARAM0)))
#define ADR_MAILBOX_DSP2MCU1_PARAM1     (ADR_MAILBOX_BASE + 35 * 4)
#define REG_MAILBOX_DSP2MCU1_PARAM1     (*((volatile unsigned int*)(ADR_MAILBOX_DSP2MCU1_PARAM1)))
#define ADR_MAILBOX_DSP2MCU1_PARAM2     (ADR_MAILBOX_BASE + 36 * 4)
#define REG_MAILBOX_DSP2MCU1_PARAM2     (*((volatile unsigned int*)(ADR_MAILBOX_DSP2MCU1_PARAM2)))
#define ADR_MAILBOX_DSP2MCU2_CMD        (ADR_MAILBOX_BASE + 40 * 4)
#define REG_MAILBOX_DSP2MCU2_CMD        (*((volatile unsigned int*)(ADR_MAILBOX_DSP2MCU2_CMD)))
#define ADR_MAILBOX_DSP2MCU2_FLAG       (ADR_MAILBOX_BASE + 41 * 4)
#define REG_MAILBOX_DSP2MCU2_FLAG       (*((volatile unsigned int*)(ADR_MAILBOX_DSP2MCU2_FLAG)))
#define ADR_MAILBOX_DSP2MCU2_PARAM0     (ADR_MAILBOX_BASE + 42 * 4)
#define REG_MAILBOX_DSP2MCU2_PARAM0     (*((volatile unsigned int*)(ADR_MAILBOX_DSP2MCU2_PARAM0)))
#define ADR_MAILBOX_DSP2MCU2_PARAM1     (ADR_MAILBOX_BASE + 43 * 4)
#define REG_MAILBOX_DSP2MCU2_PARAM1     (*((volatile unsigned int*)(ADR_MAILBOX_DSP2MCU2_PARAM1)))
#define ADR_MAILBOX_DSP2MCU2_PARAM2     (ADR_MAILBOX_BASE + 44 * 4)
#define REG_MAILBOX_DSP2MCU2_PARAM2     (*((volatile unsigned int*)(ADR_MAILBOX_DSP2MCU2_PARAM2)))

#define ADR_MAILBOX_DSP2MCU_INT_STATUS  (ADR_MAILBOX_BASE + 48 * 4)
#define REG_MAILBOX_DSP2MCU_INT_STATUS  (*((volatile unsigned int*)(ADR_MAILBOX_DSP2MCU_INT_STATUS)))
#define SFT_MAILBOX_DSP2MCU_INT1        (0)
#define SFT_MAILBOX_DSP2MCU_INT2        (1)
#define MSK_MAILBOX_DSP2MCU_INT1        (1 << SFT_MAILBOX_DSP2MCU_INT1)
#define MSK_MAILBOX_DSP2MCU_INT2        (1 << SFT_MAILBOX_DSP2MCU_INT2)

typedef enum
{
    MBX_NO_ERROR        = 0,
    MBX_CMD_ERROR       = 0xE0
}t_mbx_error;
/**
 * @brief MAILBOX COMMAND DESCRIPTION
 * ___________________________________________
 * | FLAGs  | Reserved | Group IDs | Cmd IDs |
 * -------------------------------------------
 */

#define MAILBOX_CMD_NEED_RSP_FLAG	0x80000000
#define MAILBOX_CMD_IS_RSP_FLAG  	0x40000000
#define MAILBOX_CMD_FAST_RSP_FLAG	0x20000000
#define MAILBOX_CMD_FLAG_MASK	 	0xFF000000

typedef enum _MAILBOX_CMD_SET
{
	MAILBOX_CMD_COMMON_SUBSET = 0x0000,
	MAILBOX_CMD_REG_READ,
	MAILBOX_CMD_REG_WRITE,
	MAILBOX_CMD_CYCLE_COUNTER_RESET,
	MAILBOX_CMD_CYCLE_COUNTER_GET,
	MAILBOX_CMD_AUDIO_COMMON_SUBSET = 0x0100,
	MAILBOX_CMD_AUDIO_INIT,
	MAILBOX_CMD_AUDIO_DEINIT,
	MAILBOX_CMD_AUDIO_FIFO_CONFIG,
	MAILBOX_CMD_AUDIO_FIFO_STATUS,
	MAILBOX_CMD_AUDIO_ADC_SUBSET = 0x0200,
	MAILBOX_CMD_AUDIO_ADC_INIT,
	MAILBOX_CMD_AUDIO_ADC_DEINIT,
	MAILBOX_CMD_AUDIO_ADC_SAMPLE_RATE_SET,
	MAILBOX_CMD_AUDIO_ADC_SAMPLE_RATE_GET,
	MAILBOX_CMD_AUDIO_ADC_ANALOG_VOLUME_SET,
	MAILBOX_CMD_AUDIO_ADC_ANALOG_VOLUME_GET,
	MAILBOX_CMD_AUDIO_ADC_DIGITAL_VOLUME_SET,
	MAILBOX_CMD_AUDIO_ADC_DIGITAL_VOLUME_GET,
    MAILBOX_CMD_AUDIO_ADC_VOLUME_SET, //
    MAILBOX_CMD_AUDIO_ADC_VOLUME_GET, //
	MAILBOX_CMD_AUDIO_ADC_ENABLE,
	MAILBOX_CMD_AUDIO_ADC_PCM_CHECK, //Check fill space for read
	MAILBOX_CMD_AUDIO_ADC_PCM_READ,
	MAILBOX_CMD_AUDIO_DAC_SUBSET = 0x0300,
	MAILBOX_CMD_AUDIO_DAC_INIT,
	MAILBOX_CMD_AUDIO_DAC_DEINIT,
	MAILBOX_CMD_AUDIO_DAC_SAMPLE_RATE_SET,
	MAILBOX_CMD_AUDIO_DAC_SAMPLE_RATE_GET,
	MAILBOX_CMD_AUDIO_DAC_ANALOG_VOLUME_SET,
	MAILBOX_CMD_AUDIO_DAC_ANALOG_VOLUME_GET,
	MAILBOX_CMD_AUDIO_DAC_DIGITAL_VOLUME_SET,
	MAILBOX_CMD_AUDIO_DAC_DIGITAL_VOLUME_GET,
    MAILBOX_CMD_AUDIO_DAC_VOLUME_SET,  //
    MAILBOX_CMD_AUDIO_DAC_VOLUME_GET,  //
	MAILBOX_CMD_AUDIO_DAC_ENABLE,
	MAILBOX_CMD_AUDIO_DAC_PCM_CHECK, //Check free space for write
	MAILBOX_CMD_AUDIO_DAC_PCM_WRITE,
    MAILBOX_CMD_AUDIO_DAC_FRAC_COEF,
    MAILBOX_CMD_AUDIO_DAC_VOL_TBL,
	MAILBOX_CMD_AUDIO_DAC_PCM_AVARIABLE,
	MAILBOX_CMD_AUDIO_DAC_DMA_ENABLE,

    MAILBOX_CMD_FUNC_SUBSET = 0x0400,

    MAILBOX_CMD_AUDIO_AUX_SUBSET = 0x0500,
	MAILBOX_CMD_AUDIO_AUX_INIT,
	MAILBOX_CMD_AUDIO_AUX_DEINIT,
    MAILBOX_CMD_AUDIO_AUX_ENABLE,
    MAILBOX_CMD_AUDIO_AUX_ANA_GAIN_SET,
    MAILBOX_CMD_AUDIO_AUX_ANA_GAIN_GET,
    MAILBOX_CMD_AUDIO_AUX_DIGI_GAIN_SET,
    MAILBOX_CMD_AUDIO_AUX_DIGI_GAIN_GET,

    MAILBOX_CMD_AUDIO_IDLE_SUBSET = 0x0600,
    MAILBOX_CMD_AUDIO_BT_IDLE,
    MAILBOX_CMD_AUDIO_BT_WORKING,
    MAILBOX_CMD_AUDIO_TF_USB,
    MAILBOX_CMD_AUDIO_AUX_MODE,


	MAILBOX_CMD_TEST_SUBSET = 0xFF00,
	MAILBOX_CMD_AUDIO_ADC_TEST,
	MAILBOX_CMD_AUDIO_DAC_TEST,
	MAILBOX_CMD_AUDIO_VOL_ADD,
	MAILBOX_CMD_AUDIO_VOL_SUB,
	MAILBOX_CMD_FLOAT_POINT_TEST,
	MAILBOX_CMD_SET_MASK = 0xFFFF
}MAILBOX_CMD_SET;
#define PTR(ADDR)  (( volatile uint32 *)(ADDR))

#define WRITE_MAILBOX_REG(ADDR,pCtrl) \
           { volatile uint32 *pDst =(volatile uint32 *)PTR(ADDR); \
             *pDst = *((pCtrl)); \
             *((pDst) + 2) = *((pCtrl) + 2); \
             *((pDst) + 3) = *((pCtrl) + 3); \
             *((pDst) + 4) = *((pCtrl) + 4); \
             *((pDst) + 1) = *((pCtrl) + 1); }   /* interrupt trig must be written at the last */

#define READ_MAILBOX_REG(ADDR,pCtrl) \
           { volatile uint32 *pSrc = (volatile uint32 *)PTR(ADDR); \
             volatile uint32 *pDst = (volatile uint32 *)(pCtrl); \
             *pDst = *((pSrc)); \
             *(++pDst) = *((pSrc) + 1); \
             *(++pDst) = *((pSrc) + 2); \
             *(++pDst) = *((pSrc) + 3); \
             *(++pDst) = *((pSrc) + 4); }
#define READ_MAILBOX_REG_SEG(SEG)  (*PTR(SEG))

typedef struct mailbox_ctrl
{
    uint32 cmd;
    uint32 intr_flag;
    uint32 p1;
    uint32 p2;
    uint32 p3;
    uint32 reserved;
}t_mailbox_ctrl;

uint32 set_mailbox_param(t_mailbox_ctrl *mailbox_ctrl,uint32 cmd,uint32 p1,uint32 p2,uint32 p3);
t_mbx_error write_mailbox_ctrl(uint32 idx,t_mailbox_ctrl *mailbox_ctrl,uint8 block_mode);
void wait_mailbox_ready(uint32_t idx);
uint32 read_mailbox_ctrl(uint32 idx,t_mailbox_ctrl *mailbox_ctrl);
uint32 read_mailbox_intr_status(void);
void mailbox_isr_handler(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__DRV_MAILBOX_H__
