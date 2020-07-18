/***********************************************************************
 *
 * MODULE NAME:    sys_irq.c
 * PROJECT CODE:   Bluetooth
 * DESCRIPTION:    Hardware Interrupt Interface
 * MAINTAINER:     Tom Kerwick
 * CREATION DATE:  10 Jan 2011
 *
 * LICENSE:
 *     This source code is copyright (c) 2011 Ceva Inc.
 *     All rights reserved.
 *
 ***********************************************************************/
#include "sys_config.h"
#include "config.h"
#include "sys_types.h"
#include "sys_irq.h"
#include "sys_irq_impl.h"
#include "bautil.h"
#include "lslc_irq.h"
#include "hw_leds.h"
#include "beken_external.h"
#include "bk3000_mcu.h"
#ifdef BT_DUALMODE_RW
#include "rwble.h"
#endif
#include "drv_mailbox.h"

static u_int32 int_disable_count = 0;
static u_int32 int_restore_count = 0;

extern void uart_handler(void);
extern void timer_pwm1_pt2_isr(void);
extern void gpio_isr(void);

#ifdef CONFIG_TWS
extern void pwm1_pt1_isr(void);
#endif
extern void saradc_isr(void);
extern void gpio_isr(void);
#ifdef CONFIG_IRDA
extern DRAM_CODE void IrDA_isr(void);
#endif
#if (CONFIG_AUDIO_USED_MCU == 1)
extern void aud_isr(int mode_sel );
#endif
extern void usb_isr(void);
/******************************************************************************
 *
 * FUNCTION:  SYSirq_Disable_Interrupts_Save_Flags
 * PURPOSE:   Disables ARM IRQ and FIQ Interrupts, saves previous
 *            PSR
 *
 ******************************************************************************/
DRAM_CODE void SYSirq_Disable_Interrupts_Save_Flags(u_int32 *flags, u_int32 *mask) 
{
    *flags = get_spr(SPR_SR);
    cpu_set_interrupts_enabled(0);
    *mask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), 0x00);

    int_disable_count++;
}
void SYSirq_Disable_Interrupts_Except(u_int32 *oldflags, u_int32 flags)
{
    cpu_set_interrupts_enabled(0);
    *oldflags = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), flags);
    cpu_set_interrupts_enabled(1);
}
void SYSirq_Enable_All_Interrupts(u_int32 flags)
{
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), flags);
    cpu_set_interrupts_enabled(1);
}
void SYSirq_Unmask_Interrupt(u_int32 *oldflags,u_int32 flags)
{
    cpu_set_interrupts_enabled(0);
    *oldflags = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), (*oldflags) & (~flags));
    cpu_set_interrupts_enabled(1);
}
/******************************************************************************
 *
 * FUNCTION:  SYSirq_Interrupts_Restore_Flags
 * PURPOSE:   Restores previously saved previous PSR
 *
 ******************************************************************************/
DRAM_CODE void SYSirq_Interrupts_Clear_Trig_Flags(void)
{
    uint32 trg_flags = get_spr(SPR_VICTR(0));
    if(trg_flags)
    {
        set_spr(SPR_VICTR(0),0);    
    }
}
DRAM_CODE void SYSirq_Interrupts_Restore_Flags(u_int32 flags, u_int32 mask) {

    int_restore_count++;

    if (int_disable_count > int_restore_count)
    {
        return;
    }
    else
    {
        int_disable_count = 0;
        int_restore_count = 0;
    }
	
    if(mask != 0)
    {
        uint32 trg_flags = get_spr(SPR_VICTR(0));
        set_spr(SPR_VICTR(0), trg_flags &(~(1<<VIC_CEVA_ISR_INDEX)));
        set_spr(SPR_VICMR(0), mask);
    }

    set_spr(SPR_SR,flags);

}
void interrupt_enable(int index)
{
    unsigned long oldmask = get_spr(SPR_VICMR(0));    // read old spr_vicmr
    set_spr(SPR_VICMR(0), oldmask | (1<<index));      // set  new spr_vicmr
}

void interrupt_disable(int index)
{
    unsigned long oldmask = get_spr(SPR_VICMR(0));    // read old spr_vicmr
    set_spr(SPR_VICMR(0), oldmask & (~(1<<index)));   // set  new spr_vicmr
}
void SYSirq_soft_enable_interrupt(u_int32 id)
{
    u_int32 val;

    val = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), (val | (1 << id)));
}

void SYSirq_soft_trigger_interrupt(u_int32 id)
{
    u_int32 val;

    val = get_spr(SPR_VICTR0);
    set_spr(SPR_VICTR0, (val | (1 << id)));
}

void SYSirq_soft_clear_interrupt(u_int32 id)
{
    u_int32 val;

    val = get_spr(SPR_VICTR0);
    set_spr(SPR_VICTR0, (val & (~(1 << id))));
}


/******************************************************************************
 *
 * FUNCTION:  SYSirq_Initialise
 * PURPOSE:   Initialise Interrupt Requests
 *
 ******************************************************************************/
void SYSirq_Initialise(void) {
    unsigned int sr;

    // clear all interrupts
    set_spr( SPR_VICTR(0), 0x00000000 );

    // enable interrupts
    set_spr( SPR_VICMR(0), (  (1<<VIC_AUD_ISR_INDEX)
                            | (1<<VIC_PWM5_ISR_INDEX)
                            | (1<<VIC_UART_ISR_INDEX)
                            | (1<<VIC_SADC_ISR_INDEX)
                        #if (CONFIG_SFT_INTR_TRIGGER == 1)
                            | (1<<VIC_SOFT_INTR_ISR_INDEX)
                        #endif

                        #if 0
                            | (1<<VIC_USB_PLUG_IN_ISR_INDEX)
                        #endif
                        #ifdef CONFIG_TWS
                            | (1<<VIC_PWM4_ISR_INDEX)
                        #endif
                            | (1 << VIC_MAILBOX_ISR_INDEX)
                        #ifdef BT_DUALMODE_RW
                            | (1<<VIC_RWBLE_ISR_INDEX)
                        #endif
                            | (1<<VIC_CEVA_ISR_INDEX)));

    // set interrupt handler
    set_vic_handler( SPR_VICVA(VIC_SDIO_ISR_INDEX), _sdio_isr );
    set_vic_handler( SPR_VICVA(VIC_SADC_ISR_INDEX), _sadc_isr );
    set_vic_handler( SPR_VICVA(VIC_AUD_ISR_INDEX ), _aud_isr  );
    set_vic_handler( SPR_VICVA(VIC_PWM0_ISR_INDEX), _pwm0_isr );
    set_vic_handler( SPR_VICVA(VIC_PWM1_ISR_INDEX), _pwm1_isr );
    set_vic_handler( SPR_VICVA(VIC_PWM2_ISR_INDEX), _pwm2_isr );
    set_vic_handler( SPR_VICVA(VIC_PWM3_ISR_INDEX), _pwm3_isr );
    set_vic_handler( SPR_VICVA(VIC_PWM4_ISR_INDEX), _pwm4_isr );
    set_vic_handler( SPR_VICVA(VIC_PWM5_ISR_INDEX), _pwm5_isr );
    set_vic_handler( SPR_VICVA(VIC_GPIO_ISR_INDEX), _gpio_isr );
    set_vic_handler( SPR_VICVA(VIC_SPI_ISR_INDEX ), _spi_isr  );
    set_vic_handler( SPR_VICVA(VIC_I2C2_ISR_INDEX), _i2c2_isr );
    set_vic_handler( SPR_VICVA(VIC_UART_ISR_INDEX), _uart_isr );
    set_vic_handler( SPR_VICVA(VIC_I2C1_ISR_INDEX), _i2c1_isr );
    set_vic_handler( SPR_VICVA(VIC_FFT_ISR_INDEX ), _fft_isr  );
    set_vic_handler( SPR_VICVA(VIC_DMA_ISR_INDEX ), _dma_isr  );
    set_vic_handler( SPR_VICVA(VIC_CEVA_ISR_INDEX), _ceva_isr );
    set_vic_handler( SPR_VICVA(VIC_USB_ISR_INDEX ), _usb_isr  );
    set_vic_handler( SPR_VICVA(VIC_IRDA_ISR_INDEX), _irda_isr );

    set_vic_handler( SPR_VICVA(VIC_USB_PLUG_IN_ISR_INDEX), _usg_plug_in_isr );
#ifdef BT_DUALMODE_RW
    set_vic_handler( SPR_VICVA(VIC_RWBLE_ISR_INDEX), _rwble_isr );
#endif
    set_vic_handler( SPR_VICVA(VIC_MAILBOX_ISR_INDEX), _mailbox_isr);

    // set priority
    set_spr( SPR_VICPR(VIC_SDIO_ISR_INDEX), 1 );
    set_spr( SPR_VICPR(VIC_SADC_ISR_INDEX), 2 );
    set_spr( SPR_VICPR(VIC_AUD_ISR_INDEX ), 7 );
    set_spr( SPR_VICPR(VIC_PWM0_ISR_INDEX), 5 );
    set_spr( SPR_VICPR(VIC_PWM1_ISR_INDEX), 5 );
    set_spr( SPR_VICPR(VIC_PWM2_ISR_INDEX), 2 );
    set_spr( SPR_VICPR(VIC_PWM3_ISR_INDEX), 3 );
    set_spr( SPR_VICPR(VIC_PWM4_ISR_INDEX), 5 );
    set_spr( SPR_VICPR(VIC_PWM5_ISR_INDEX), 5 );
    set_spr( SPR_VICPR(VIC_GPIO_ISR_INDEX), 2 );
    set_spr( SPR_VICPR(VIC_SPI_ISR_INDEX ), 3 );
    set_spr( SPR_VICPR(VIC_I2C2_ISR_INDEX), 1 );
    set_spr( SPR_VICPR(VIC_UART_ISR_INDEX), 2 );
    set_spr( SPR_VICPR(VIC_I2C1_ISR_INDEX), 1 );
    set_spr( SPR_VICPR(VIC_FFT_ISR_INDEX ), 2 );
    set_spr( SPR_VICPR(VIC_DMA_ISR_INDEX ), 3 );
    set_spr( SPR_VICPR(VIC_CEVA_ISR_INDEX), 6 );
    set_spr( SPR_VICPR(VIC_USB_ISR_INDEX ), 1 );
    set_spr( SPR_VICPR(VIC_IRDA_ISR_INDEX), 3 );

    set_spr( SPR_VICPR(VIC_USB_PLUG_IN_ISR_INDEX), 1 );
#ifdef BT_DUALMODE_RW
    set_spr( SPR_VICPR(VIC_RWBLE_ISR_INDEX), 1 );
#endif
    set_spr( SPR_VICPR(VIC_MAILBOX_ISR_INDEX), 1 );

    set_spr(SPR_PM_EVENT_CTRL, 0x01);//enable pic  envent
    // set global interrupt enable
    sr = get_spr(SPR_SR);
    sr = sr | SPR_SR_IEE;
    set_spr(SPR_SR, sr);
}

void UART_InterruptHandler(void) {
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    /* unsigned int sr ; */
    set_spr(SPR_VICMR(0), oldmask & (0
									#if DEBUG_WATCHDOG_RESET_ISSUE_ENABLE
									| (1 << VIC_PWM5_ISR_INDEX)
									#endif
									));
    if (oldmask & (0
					#if DEBUG_WATCHDOG_RESET_ISSUE_ENABLE
					| (1 << VIC_PWM5_ISR_INDEX)
					#endif
					)
    )
        cpu_set_interrupts_enabled(1);

    /* //disable dcache */
    /* sr = get_spr(SPR_SR); */
    /* sr = sr & (~SPR_SR_DCE); */
    /* set_spr(SPR_SR, sr); */

    uart_handler();

    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);

    /* //enable dcache */
    /* sr = get_spr(SPR_SR); */
    /* sr = sr | SPR_SR_DCE; */
    /* set_spr(SPR_SR, sr); */
}
#ifdef CONFIG_TWS
extern void audio_sync_process_for_dac_open(void);
#endif
DRAM_CODE void CEVA_InterruptHandler (void) {
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    set_spr(SPR_VICMR(0), oldmask & ((1 << VIC_AUD_ISR_INDEX)                                            
                                            #if DEBUG_WATCHDOG_RESET_ISSUE_ENABLE
                                            | (1 << VIC_PWM5_ISR_INDEX)
                                            #endif
                                            )
               );                     //mask all priority interrupt.
        
    if (oldmask & ((1 << VIC_AUD_ISR_INDEX)        
        #if DEBUG_WATCHDOG_RESET_ISSUE_ENABLE
        | (1 << VIC_PWM5_ISR_INDEX)
        #endif
        )
    )
    cpu_set_interrupts_enabled(1);
	//set_spr(SPR_VICMR(0), 0x00);					   //mask all priority interrupt.
#ifdef CONFIG_TWS
    audio_sync_process_for_dac_open();
#endif
    LSLCirq_IRQ_Handler();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.

}
#ifdef BT_DUALMODE_RW
DRAM_CODE void RWBLE_InterruptHandler(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    set_spr( SPR_VICMR(0), oldmask & (1 << VIC_CEVA_ISR_INDEX));
    cpu_set_interrupts_enabled(1);
    rwble_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
}
#else
void RWBLE_InterruptHandler  (void) {os_printf("RWBLE_InterruptHandler\r\n");}
#endif
void PWM5_InterruptHandler (void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
#ifdef BT_DUALMODE_RW
    set_spr( SPR_VICMR(0), oldmask & ((1 << VIC_CEVA_ISR_INDEX)|(1 << VIC_RWBLE_ISR_INDEX)));
#else
	set_spr( SPR_VICMR(0), oldmask & (1 << VIC_CEVA_ISR_INDEX));
#endif
    cpu_set_interrupts_enabled(1);

    //set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.
    timer_pwm1_pt2_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
}

void AUD_InterruptHandler (void) 
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.
#if (CONFIG_AUDIO_USED_MCU == 1)
    aud_isr(0);
#endif
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
}

void FFT_InterruptHandler  (void) {os_printf("FFT_InterruptHandler\r\n");}
#ifdef CONFIG_APP_USB
void USB_InterruptHandler (void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
#ifdef CONFIG_BLUETOOTH_COEXIST
    set_spr( SPR_VICMR(0), oldmask & ((1 << VIC_CEVA_ISR_INDEX)|(1 << VIC_RWBLE_ISR_INDEX)));
#else
    set_spr(SPR_VICMR(0), 0x00);                        //mask all/low priority interrupt.
#endif
    usb_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask); 
}
#else
void USB_InterruptHandler  (void) {os_printf("USB_InterruptHandler\r\n");}
#endif
void SADC_InterruptHandler (void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.
    saradc_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
}
void MAILBOX_InterruptHandler(void)
{
    void mailbox_isr_handler(void);
    mailbox_isr_handler();
}
void INT0_InterruptHandler (void) {os_printf("INT0_InterruptHandler\r\n");}
void INT1_InterruptHandler (void) {os_printf("INT1_InterruptHandler\r\n");}
void TIME_InterruptHandler (void) {os_printf("TIME_InterruptHandler\r\n");}
void PWM0_InterruptHandler (void) {os_printf("PWM0_InterruptHandler\r\n");}
void PWM1_InterruptHandler (void) {os_printf("PWM1_InterruptHandler\r\n");}
void PWM2_InterruptHandler (void) {os_printf("PWM2_InterruptHandler\r\n");}
void PWM3_InterruptHandler (void) 
{
#if (CONFIG_SFT_INTR_TRIGGER == 1)
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr

    set_spr( SPR_VICMR(0), oldmask & (1 << VIC_CEVA_ISR_INDEX));
    cpu_set_interrupts_enabled(1);

    SYSirq_soft_clear_interrupt(VIC_SOFT_INTR_ISR_INDEX);    
    // sbc do decode...
    
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
#else
    return;
#endif
}
void PWM4_InterruptHandler (void) {
#ifdef CONFIG_TWS
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.
    pwm1_pt1_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
#else
    os_printf("PWM4_InterruptHandler\r\n");
#endif
}

void GPIO_InterruptHandler (void) //{os_printf("GPIO_InterruptHandler\r\n");}
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.
    //Open_26M_Clock();
    gpio_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
}
void SPI_InterruptHandler  (void) {os_printf("SPI_InterruptHandler\r\n");}
void I2C1_InterruptHandler (void)   {os_printf("I2C1_InterruptHandler\r\n");}
void I2C2_InterruptHandler (void) {os_printf("I2C2_InterruptHandler\r\n");}
void DMA_InterruptHandler  (void) {os_printf("DMA_InterruptHandler\r\n");}
void SDIO_InterruptHandler (void)
{
	return;
/*
	os_printf("SDIO_InteruptHandler:%08x\r\n",get_spr(SPR_VICMR(0)));
*/

}
void USB_Plug_In_InterruptHandler (void) 
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.
    os_printf("USB_Plug_In_InterruptHandler\r\n");
    cpu_set_interrupts_enabled(0);
    oldmask &= ~(1 << VIC_USB_PLUG_IN_ISR_INDEX);
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.

}
#ifdef CONFIG_IRDA
DRAM_CODE void IRDA_InterruptHandler (void) 
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    set_spr(SPR_VICMR(0), 0x00);                         //mask all/low priority interrupt.
    IrDA_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);   
}
#else
void IRDA_InterruptHandler (void)  {os_printf("IRDA_InterruptHandler\r\n");}
#endif


