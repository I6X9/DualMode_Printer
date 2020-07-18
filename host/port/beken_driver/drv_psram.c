/**
 **************************************************************************************
 * @file    drv_psram.c
 * @brief   Driver API for PSRAM
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2018 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#include "drv_psram.h"
#ifdef __TEAKLITE4__
#include "drv_mailbox.h"
#endif

#define SIZE_OF_CHAR                        (sizeof(short) == 2 ? 1 : 2)

#define GPIO_REG_BASE    		            (0x00F40000 / SIZE_OF_CHAR)
#define GPIO_REG_READ(i)                    *(volatile uint32_t*)(GPIO_REG_BASE + (i) * 4 / SIZE_OF_CHAR)
#define GPIO_REG_WRITE(i, v)                *(volatile uint32_t*)(GPIO_REG_BASE + (i) * 4 / SIZE_OF_CHAR) = v

#define PSRAM_REG_BASE    		            (0x00910000 / SIZE_OF_CHAR)
#define PSRAM_REG_READ(i)                   *(volatile uint32_t*)(PSRAM_REG_BASE + (i) * 4 / SIZE_OF_CHAR)
#define PSRAM_REG_WRITE(i, v)               *(volatile uint32_t*)(PSRAM_REG_BASE + (i) * 4 / SIZE_OF_CHAR) = v

void psram_init(uint32_t pin, uint32_t clk, uint32_t mode)
{
    uint32_t reg;
    uint32_t bw, cnt, cmd;

    #ifdef __TEAKLITE4__
    reg  = mailbox_reg_read((0x00800000 + 1 * 4) / SIZE_OF_CHAR);
    reg &= ~((1 << 20) | (1  << 31));
    reg |= (((clk >> 3) & 1) << 20);
    mailbox_reg_write((0x00800000 + 1 * 4) / SIZE_OF_CHAR, reg);
    #else
    reg  = *(volatile uint32_t*)(0x00800000 + 1 * 4);
    reg &= ~((1 << 20) | (1  << 31));
    reg |= (((clk >> 3) & 1) << 20);
    *(volatile uint32_t*)(0x00800000 + 1 * 4) = reg;
    #endif

    if(pin == PSRAM_PIN_GPIO)
    {
		GPIO_REG_WRITE(0x06, 0x48);
		GPIO_REG_WRITE(0x07, 0x48);
		GPIO_REG_WRITE(0x08, 0x48);
		GPIO_REG_WRITE(0x09, 0x48);

		reg  = GPIO_REG_READ(0x20);
		reg &= ~(0xF << 6);
		GPIO_REG_WRITE(0x20, reg);

		reg  = GPIO_REG_READ(0x3B);
		reg &= ~(0xF << 6);
		GPIO_REG_WRITE(0x3B, reg);

		reg  = GPIO_REG_READ(0x26);
		reg |= (0xF << 6);
		GPIO_REG_WRITE(0x26, reg);

		reg  = GPIO_REG_READ(0x1F);
		reg &= ~(0xF << 6);
		GPIO_REG_WRITE(0x1F, reg);

		if(mode == PSRAM_MODE_LINE_4)
		{
			GPIO_REG_WRITE(0x0A, 0x48);
			GPIO_REG_WRITE(0x0B, 0x48);

			reg  = GPIO_REG_READ(0x20);
			reg &= ~(0x3 << 10);
			GPIO_REG_WRITE(0x20, reg);

			reg  = GPIO_REG_READ(0x3B);
			reg &= ~(0x3 << 10);
			GPIO_REG_WRITE(0x3B, reg);

			reg  = GPIO_REG_READ(0x26);
			reg |= (0x3 << 10);
			GPIO_REG_WRITE(0x26, reg);

			reg  = GPIO_REG_READ(0x1F);
			reg &= ~(0x3 << 10);
			GPIO_REG_WRITE(0x1F, reg);
		}
    }

    reg  = GPIO_REG_READ(0x2D);
    reg &= ~(0x1 << 14);
    if(pin == PSRAM_PIN_GPIO)
    {
        reg |= (0x1 << 14);
    }
    reg |= (0x3 << 15);
    GPIO_REG_WRITE(0x2D, reg);

    #ifdef __TEAKLITE4__
    reg  = mailbox_reg_read((0x00F00000 + 0xD * 4) / SIZE_OF_CHAR);
    reg |= ((1 << 6) | (1  << 29));
    mailbox_reg_write((0x00F00000 + 0xD * 4) / SIZE_OF_CHAR, reg);
    #else
	#define XVR_ANALOG_REG_BAK  XVR_analog_reg_save
    extern uint32_t XVR_ANALOG_REG_BAK[16];
    reg  = XVR_ANALOG_REG_BAK[0xD];
    reg |= ((1 << 6) | (1  << 29));
    *(volatile uint32_t*)(0x00F00000 + 0xD * 4) = XVR_ANALOG_REG_BAK[0xD] = reg;
    #endif

    reg  = PSRAM_REG_READ(0x1C);
    reg &= ~(0x7 << 8);
    reg |= ((clk & 0x7) << 8);
    PSRAM_REG_WRITE(0x1C, reg);

	if(mode == PSRAM_MODE_LINE_4)
	{
		bw  = 0;
		cnt = 8;
		cmd = 0x35;
	}
	else
	{
		bw  = 1;
		cnt = 2;
		cmd = 0xF5;
	}

	reg  = PSRAM_REG_READ(0x1A);
	reg &= ~((1 << 4) | (1 << 5));
	PSRAM_REG_WRITE(0x1A, reg);
	PSRAM_REG_WRITE(0x00, (1 << 0) | (bw  << 1) | (cmd << 2) | (cnt << 10));
	PSRAM_REG_WRITE(0x01, (bw << 1) | ((cnt * ( 24 / 8)) << 26));
	PSRAM_REG_WRITE(0x02, (bw << 1));
	PSRAM_REG_WRITE(0x03, (bw << 1) | (1 << 14));
	PSRAM_REG_WRITE(0x09, 5);
	PSRAM_REG_WRITE(0x17, (0x10 << 0) | (0x10 << 11));
	PSRAM_REG_WRITE(0x12, 0);
	reg |= (1 << 5);
	PSRAM_REG_WRITE(0x1A, reg);
	PSRAM_REG_WRITE(0x09, 5 | (1 << 4));
	while((PSRAM_REG_READ(0x36) & (1 << 16)) == 0);
	PSRAM_REG_WRITE(0x09, 5 | (1 << 6));
	reg &= ~(1 << 5);
	PSRAM_REG_WRITE(0x1A, reg);

    if(mode == PSRAM_MODE_LINE_4)
    {
        PSRAM_REG_WRITE(0x24, 0x00000BAF);
        PSRAM_REG_WRITE(0x25, 0x18000003);
        PSRAM_REG_WRITE(0x26, 0x0000001B);
        PSRAM_REG_WRITE(0x27, 0x00000083);
        PSRAM_REG_WRITE(0x28, 0x000008E3);
        PSRAM_REG_WRITE(0x29, 0x18000003);
        PSRAM_REG_WRITE(0x2A, 0x00000002);
        PSRAM_REG_WRITE(0x2B, 0x00004083);
        PSRAM_REG_WRITE(0x2C, 0x00000001);
    }
    else
    {
        PSRAM_REG_WRITE(0x24, 0x0000200D);
        PSRAM_REG_WRITE(0x25, 0x60000001);
        PSRAM_REG_WRITE(0x26, 0x00000000);
        PSRAM_REG_WRITE(0x27, 0x00000201);
        PSRAM_REG_WRITE(0x28, 0x00002009);
        PSRAM_REG_WRITE(0x29, 0x60000001);
        PSRAM_REG_WRITE(0x2A, 0x00000000);
        PSRAM_REG_WRITE(0x2B, 0x00004201);
        PSRAM_REG_WRITE(0x2C, 0x00000001);
    }
}
