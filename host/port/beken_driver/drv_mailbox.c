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

#include "bautil.h"
#include "drv_mailbox.h"

/* Set mail_box cmd and p1 p2 p3 data */
uint32 set_mailbox_param(t_mailbox_ctrl *mailbox_ctrl,uint32 cmd,uint32 p1,uint32 p2,uint32 p3)
{
    mailbox_ctrl->cmd = cmd;
    mailbox_ctrl->p1 = p1;
    mailbox_ctrl->p2 = p2;
    mailbox_ctrl->p3 = p3;
    return 0;
}
/* Read DSP to MCU interrupt status register */
uint32 read_mailbox_intr_status(void)
{
    return READ_MAILBOX_REG_SEG(MAILBOX_BASEADDR + MAILBOX_D2M_INTR_STATUS);
}
/* Read mailbox register group include 5 * 4 octs: cmd + intr_flag + p1 + p2 + p3 */
uint32 read_mailbox_ctrl(uint32 idx,t_mailbox_ctrl *mailbox_ctrl)
{
    READ_MAILBOX_REG((MAILBOX_BASEADDR + idx*MAILBOX_OFFSET),(uint32 *)mailbox_ctrl);
    return 0;
}
/* Write mailbox register group include 5 * 4 octs: cmd + intr_flag + p1 + p2 + p3 */
t_mbx_error write_mailbox_ctrl(uint32 idx,t_mailbox_ctrl *mailbox_ctrl,uint8 block_mode)
{
	int32 os_printf(const char *fmt, ...);
	//os_printf("[MCU]:  write_mailbox_ctrl(%d, %d, %d);\r\n", idx, mailbox_ctrl->cmd, block_mode);
	//os_printf("M: %x\n", mailbox_ctrl->cmd);
#if 0
	if((mailbox_ctrl->cmd & MAILBOX_CMD_SET_MASK) != MAILBOX_CMD_AUDIO_ADC_PCM_READ && (mailbox_ctrl->cmd & MAILBOX_CMD_SET_MASK) != MAILBOX_CMD_AUDIO_DAC_PCM_WRITE)
	{
		os_printf("[MCU]: %x,%d,%d,%d\n", mailbox_ctrl->cmd,mailbox_ctrl->p1,mailbox_ctrl->p2,mailbox_ctrl->p3);
	}
#endif
#if 1
    uint32 intr_flag = 0;
    t_mbx_error ret = MBX_NO_ERROR;
    mailbox_ctrl->intr_flag = 1;
    intr_flag = READ_MAILBOX_REG_SEG(MAILBOX_BASEADDR + idx*MAILBOX_OFFSET + MAILBOX_INTR_SEG);
    if(block_mode)
    {
        mailbox_ctrl->intr_flag = 1;
        WRITE_MAILBOX_REG((MAILBOX_BASEADDR + idx*MAILBOX_OFFSET),(uint32 *)mailbox_ctrl);
        while(READ_MAILBOX_REG_SEG(MAILBOX_BASEADDR + idx*MAILBOX_OFFSET + MAILBOX_INTR_SEG));
    }
    else
    {
        if(intr_flag)
        {
            ret =  MBX_CMD_ERROR;
        }
        else
        {
            mailbox_ctrl->intr_flag = 1;
            WRITE_MAILBOX_REG((MAILBOX_BASEADDR + idx*MAILBOX_OFFSET),(uint32 *)mailbox_ctrl);
        }
    }
    //while(*(volatile uint32_t*)(MAILBOX_BASEADDR + idx*MAILBOX_OFFSET + MAILBOX_INTR_SEG));
    return ret;   
#else
	return 0;
#endif
}

void wait_mailbox_ready(uint32_t idx)
{
    while(*(volatile uint32_t*)(MAILBOX_BASEADDR + idx*MAILBOX_OFFSET + MAILBOX_INTR_SEG));
}

void cycle_counter_reset(void)
{
    set_spr(SPR_TTMR, 0);
    set_spr(SPR_TTCR, 0);
    set_spr(SPR_TTMR, SPR_TTMR_SR | 0xFFFFFFF);
    set_spr(SPR_SR, (get_spr(SPR_SR) | SPR_SR_TEE));
}

uint32_t cycle_counter_get(void)
{
    volatile unsigned int reg;
    volatile unsigned int result;

    reg = SPR_TTCR;

    __asm("b.nop 5");
    __asm
    (
        "b.mfspr %0,%1,0;"
        :"=r" (result)
        :"r" (reg)
        :
    );

    return result;
}
void mailbox_isr_handler(void)
{
    int32 os_printf(const char *fmt, ...);
    extern u_int32 XVR_analog_reg_save[];

    uint32_t addr;
    uint32_t cmd = REG_MAILBOX_DSP2MCU1_CMD & MAILBOX_CMD_SET_MASK;
	uint32_t flg = REG_MAILBOX_DSP2MCU1_CMD & MAILBOX_CMD_FLAG_MASK;
    uint32_t p0 = ADR_MAILBOX_DSP2MCU1_PARAM0;
    if(REG_MAILBOX_DSP2MCU_INT_STATUS & 0x1)
    {
        os_printf("MCU[%08X, %d, %d, %d]\n", REG_MAILBOX_DSP2MCU1_CMD, REG_MAILBOX_DSP2MCU1_PARAM0, REG_MAILBOX_DSP2MCU1_PARAM1, REG_MAILBOX_DSP2MCU1_PARAM2);

		switch(cmd)
		{
		case MAILBOX_CMD_REG_READ:
			addr = (*(uint32_t*)ADR_MAILBOX_DSP2MCU1_PARAM0) * 2;
			if((addr >= 0x00F00000) && (addr <= (0x00F00000 + 15 *  4)))
			{
				
				REG_MAILBOX_DSP2MCU1_PARAM1 = XVR_analog_reg_save[(addr - 0x00F00000) / 4];
			}
			else
			{
				REG_MAILBOX_DSP2MCU1_PARAM1 = *(uint32_t*)addr ; 
			}
			break;
		case MAILBOX_CMD_REG_WRITE:
			addr = (*(uint32_t*)ADR_MAILBOX_DSP2MCU1_PARAM0) * 2;
			if((addr >= 0x00F00000) && (addr <= (0x00F00000 + 15 *  4)))
			{
				*(uint32_t*)addr = XVR_analog_reg_save[(addr - 0x00F00000) / 4] = REG_MAILBOX_DSP2MCU1_PARAM1;
			}
			else
			{
				*(uint32_t*)addr = REG_MAILBOX_DSP2MCU1_PARAM1;
			}
			*(uint32_t*)((*(uint32_t*)ADR_MAILBOX_DSP2MCU1_PARAM0) * 2)  = REG_MAILBOX_DSP2MCU1_PARAM1;
			break;
		case MAILBOX_CMD_CYCLE_COUNTER_RESET:
			cycle_counter_reset();
			break;
		case MAILBOX_CMD_CYCLE_COUNTER_GET:
			REG_MAILBOX_DSP2MCU1_PARAM0 = cycle_counter_get();
			break;
        case MAILBOX_CMD_AUDIO_DAC_ENABLE:

            if(flg & MAILBOX_CMD_IS_RSP_FLAG)
            {
                aud_set_adda_en_response();
                aud_set_dac_opened(p0&0xff);
            }
        case MAILBOX_CMD_AUDIO_ADC_ENABLE:

            if(flg & MAILBOX_CMD_IS_RSP_FLAG)
            {
                aud_set_adda_en_response();
                aud_set_adc_opened(p0&0xff);
            }

            break;
		default:
			break;
		}

		REG_MAILBOX_DSP2MCU1_FLAG = 0;
	}

	if(REG_MAILBOX_DSP2MCU_INT_STATUS & 0x2)
	{
	       //os_printf("[%d, %08X, %08X, %08X]\n", REG_MAILBOX_DSP2MCU2_CMD, REG_MAILBOX_DSP2MCU2_PARAM0, REG_MAILBOX_DSP2MCU2_PARAM1, REG_MAILBOX_DSP2MCU2_PARAM2);

		switch(REG_MAILBOX_DSP2MCU1_CMD)
		{
		case MAILBOX_CMD_REG_READ:
			addr = (*(uint32_t*)ADR_MAILBOX_DSP2MCU2_PARAM0) * 2;
			if((addr >= 0x00F00000) && (addr <= (0x00F00000 + 15 *  4)))
			{
				
				REG_MAILBOX_DSP2MCU2_PARAM1 = XVR_analog_reg_save[(addr - 0x00F00000) / 4];
			}
			else
			{
				REG_MAILBOX_DSP2MCU2_PARAM1 = *(uint32_t*)addr ; 
			}
			break;
		case MAILBOX_CMD_REG_WRITE:
			addr = (*(uint32_t*)ADR_MAILBOX_DSP2MCU2_PARAM0) * 2;
			if((addr >= 0x00F00000) && (addr <= (0x00F00000 + 15 *  4)))
			{
				*(uint32_t*)addr = XVR_analog_reg_save[(addr - 0x00F00000) / 4] = REG_MAILBOX_DSP2MCU2_PARAM1;
			}
			else
			{
				*(uint32_t*)addr = REG_MAILBOX_DSP2MCU2_PARAM1;
			}
			break;
		case MAILBOX_CMD_CYCLE_COUNTER_RESET:
			cycle_counter_reset();
			break;
		case MAILBOX_CMD_CYCLE_COUNTER_GET:
			REG_MAILBOX_DSP2MCU2_PARAM0 = cycle_counter_get();
			break;
		default:
			break;
		}

		REG_MAILBOX_DSP2MCU2_FLAG = 0;
	}

    return;    
}
