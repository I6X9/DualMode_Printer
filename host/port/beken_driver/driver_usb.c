#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "driver_usb.h"
#include "mu_stdint.h"

#ifdef CONFIG_APP_USB
extern uint8 MGC_MsdGetMediumstatus(void);
extern uint8 MUSB_GetConnect_Flag(void);
extern void MUSB_Host_init(void);
extern uint32 MUSB_NoneRunBackground(void);
extern void MUSB_Host_uninit(void);
extern uint32 MUSB_HfiRead(uint32 first_block, uint32 block_num, uint8 *dest);
extern void MGC_AfsUdsIsr(uint16 wIntrTxValue,uint16 wIntrRxValue,uint8 bIntrUsbValue);
extern uint32 get_HfiMedium_Block_CountLo(void);
extern uint32 get_HfiMedium_Block_Size(void);
extern int usb_sw_init(void);


static driver_udisk_t driver_udisk;
// __VOLATILE__ int usb_start_enumerate_sem;



void usb_interrupt_enable(void)
{
	interrupt_enable(VIC_USB_ISR_INDEX);
}

void usb_interrupt_disable(void)
{
	interrupt_disable(VIC_USB_ISR_INDEX);
}

void usb_enumberator(void)
{
    uint32 ret = USB_RET_ERROR;
    app_handle_t app_h = app_get_sys_handler();
//    int ret_val;
//    MSG_T msg;
    //os_printf("usb_enumberator\r\n");
//    while (1)
//    {
//        while (usb_start_enumerate_sem == 0)
//        {
//        }
//        usb_start_enumerate_sem = 0;

//        enable_timeout_timer(1);
//        MUSB_Host_init();
//        driver_udisk.dwBlockCountLo = 0;
//        driver_udisk.dwBlockSize = 0;
//        driver_udisk.InitFlag = 1;
//        while (1)
//        {
//            ret_val = msg_get(&msg);
//            if (MSG_FAILURE_RET != ret_val)
//            {
//                if (msg.id != MSG_KEY_MODE)
//                {
//                    msg_put(msg.id);
//                }
//                else
//                    break;
//            }
//            if (timeout_handle())
//            {
//                ret = USB_RET_ERROR;
//                break;
//            }

            ret = MUSB_NoneRunBackground();
            if (MGC_MsdGetMediumstatus())
            {
                driver_udisk.dwBlockCountLo = get_HfiMedium_Block_CountLo();
                driver_udisk.dwBlockSize    = get_HfiMedium_Block_Size();
//                ret = USB_RET_OK;
                os_printf("usb_enumberator: OK\r\n");
//                break;
            }
            else
            {
                if ((USB_RET_DISCONNECT == ret) || (USB_RET_ERROR == ret))
                {
                    MUSB_ERR_PRINTF("usb_enumberator: ERROR ret = %d\r\n", ret);
//                    break;
                }
                else
                {
                    MUSB_DPRINTF("usb_enumberator: ret = %d\r\n", ret);
                    //jtask_stop(app_h->usb_host_enumerate_task);
                	jtask_schedule(app_h->usb_host_enumerate_task, 10,(jthread_func)usb_enumberator, NULL);
                }
            }
//        }
//        disable_timeout_timer();
//    }
//    MUSB_DPRINTF("usb_enumberator ret = %d\r\n", ret);
}

void usb_host_start_enumberate(void)
{
    app_handle_t app_h = app_get_sys_handler();

    MUSB_DPRINTF("usb_host_start_enumberate\r\n");
    MUSB_Host_init();
    driver_udisk.dwBlockCountLo = 0;
    driver_udisk.dwBlockSize = 0;
    driver_udisk.InitFlag = 1;
    jtask_stop(app_h->usb_host_enumerate_task);
    jtask_schedule(app_h->usb_host_enumerate_task, 10, (jthread_func)usb_enumberator, NULL);
}

void usb_init(USB_MODE usb_mode)
{
    uint32 tmp = 0;
    uint32 ahb_int_state = 0;
    //app_handle_t app_h = app_get_sys_handler();

    os_printf("usb_init(%d)\r\n", usb_mode);
    //BK3000_XVR_REG_0x0E = XVR_analog_reg_save[14] = XVR_analog_reg_save[14] | ( (1<<30));
    //BK3000_PMU_GATE_CFG = 0xFFFFFFFF;
    // open DPLL
    tmp = BK3000_A0_CONFIG;
    tmp &= ~((1<<0) |(1<<15) | (1<<20)| (1<<29) | (1<<31));// OPEN 48M, open  DPLL
    tmp |= (1<<15) | (1<<20) | (1<<31);
    BK3000_A0_CONFIG = tmp;

    BK3000_XVR_REG_0x0E = XVR_analog_reg_save[14] = 0xE0B99358; // bit30: DPLL clock for USB
    BK3000_XVR_REG_0x0F = XVR_analog_reg_save[15] = 0x3B13B13B;
    BK3000_XVR_REG_0x0E = XVR_analog_reg_save[14] = 0xE0B99358 | (1<<18);
    Delay(50);
    BK3000_XVR_REG_0x0E = XVR_analog_reg_save[14] = 0xE0B99358 & (~(1<<18));
    Delay(5000);
    BK3000_A0_CONFIG &= (~(1<<5));
    BK3000_PMU_PERI_PWDS &= (~bit_PMU_USB_PWD);
    Delay(20);
#if 1
    //(*(volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+0x30*4)) |= (1 << 4);
    BK3000_A0_CONFIG |= (1 << 4);
    Delay(500);
    //(*(volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+0x30*4)) |= (4 << 1);
    if (usb_mode == USB_HOST_MODE)
    {
        BK3000_A0_CONFIG &= (~(4 << 1));
    }
    else
    {
        BK3000_A0_CONFIG |= (4 << 1);
    }
    Delay(500);
#endif

    BK3000_GPIO_10_CONFIG = (1 << 3);  // DP
    BK3000_GPIO_11_CONFIG = (1 << 3);  // DN
    //关闭所有USB 中断使能
    REG_USB_INTRRX1E = 0x0;
    REG_USB_INTRTX1E = 0x0;
    REG_USB_INTRUSBE = 0x0;
    REG_AHB2_USB_VTH &= ~(1<<7);

    if (usb_mode == USB_HOST_MODE) // HOST  usb_mode
    {
        REG_AHB2_USB_OTG_CFG = 0x50; // USB 默认为HOST 模式 打开DP DN下拉
        REG_AHB2_USB_DEV_CFG = 0x00;
    }
    else //device
    {
        REG_USB_INTRRX1E = 0x07;
        REG_USB_INTRTX1E = 0x07;
        REG_USB_INTRUSBE = 0x3F;
        REG_AHB2_USB_OTG_CFG = 0x08;        // dp pull up
        REG_AHB2_USB_DEV_CFG = 0xF4;
        REG_AHB2_USB_OTG_CFG |= 0x01;       // device
    }
    Delay(500);
    REG_AHB2_USB_INT = 0xFF;

    ahb_int_state = REG_AHB2_USB_INT;
    Delay(500);
    REG_AHB2_USB_INT = ahb_int_state;
    Delay(500);
    //配置USB DP DN 模拟驱动配置
    REG_AHB2_USB_GEN = (0x6<<4) | (0x6<<0);   //  DP_EN DN_EN
    Delay(5);

    //ananog_printf();
    memory_usage_show();
    if (usb_sw_init() == 0)//初始化
    {
        usb_interrupt_enable();
        os_printf("usb_sw_init ok\r\n");
        if (usb_mode == USB_HOST_MODE)
        {
            // jtask_init(app_h->usb_host_enumerate_task, J_TASK_TIMEOUT);
            //jtask_schedule(app_h->usb_host_enumerate_task, 10, (jthread_func)usb_enumberator, NULL);
        }
    }
    else
    {
        usb_interrupt_disable();
        os_printf("usb_sw_init fail\r\n");
    }
    memory_usage_show();
}

void usb_uninit(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));//read old spr_vicmr
//    set_spr(SPR_VICMR(0), 0x00);  //mask all/low priority interrupt.
    oldmask &= (~(1 << VIC_USB_ISR_INDEX));
    set_spr(SPR_VICMR(0), oldmask); //recover the old spr_vicmr.

    REG_AHB2_USB_OTG_CFG = 0;
}

void usb_reinit_to_mode(USB_MODE usb_mode)
{
	usb_reset();
	usb_init(usb_mode);
}

void usb_reset(void)
{
    usb_interrupt_disable();

    BK3000_A0_CONFIG &= (~(4 << 1));
    Delay(50);
    REG_AHB2_USB_OTG_CFG  = 0x00;
    REG_AHB2_USB_DMA_ENDP = 0x00;
    REG_AHB2_USB_VTH      = 0x83;
    REG_AHB2_USB_GEN      = 0x00;
    REG_AHB2_USB_STAT     = 0x00;
    REG_AHB2_USB_INT      = 0xA0;
    REG_AHB2_USB_RESET    = 0x01;
    REG_AHB2_USB_DEV_CFG  = 0x00;
    BK3000_PMU_PERI_PWDS |= bit_PMU_USB_PWD;
}

//void usb_reinit(uint8 usb_mode)
//{
//    if (usb_mode) // HOST  usb_mode
//    {
//        REG_AHB2_USB_OTG_CFG = 0x50; //USB 默认为HOST 模式 打开DP DN下拉
//        REG_AHB2_USB_DEV_CFG = 0x00;
//    }
//    else //device
//    {
//        REG_AHB2_USB_OTG_CFG = 0x08;        // dp pull up
//        REG_AHB2_USB_DEV_CFG = 0xF4;
//        REG_AHB2_USB_OTG_CFG |= 0x01;       // device
//    }
//
//    Delay(5);
//
//    unsigned int oldmask = get_spr(SPR_VICMR(0));//read old spr_vicmr
////    set_spr(SPR_VICMR(0), 0x00);  //mask all/low priority interrupt.
//    oldmask |= ((1 << VIC_USB_ISR_INDEX));
//    set_spr(SPR_VICMR(0), oldmask); //recover the old spr_vicmr.
//}

uint8 udisk_is_attached(void)
{
    app_env_handle_t env_h = app_env_get_handle();
    MUSB_DPRINTF("udisk_is_attached\r\n");
    if (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_USB_ENA)
    {
        return MUSB_GetConnect_Flag();
    }
    else
        return 0;
}

uint8 udisk_is_enumerated(void)
{
    app_env_handle_t env_h = app_env_get_handle();
    MUSB_DPRINTF("udisk_is_enumerated\r\n");
    if (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_USB_ENA)
    {
        return MGC_MsdGetMediumstatus();
    }
    else
        return 0;
}

uint8 udisk_init(void)
{
    uint32 ret = USB_RET_ERROR;
//    int ret_val;
    MSG_T msg;
    MUSB_DPRINTF("udisk_init\r\n");
    if (MGC_MsdGetMediumstatus())
    {
        ret = USB_RET_OK;
    }
    else
    {
        MUSB_ERR_PRINTF("udisk_init: Medium not ready\r\n");
    }
/*    else
    {
        //os_printf("===musb host init===\r\n");
        enable_timeout_timer(1);
        MUSB_Host_init();
        driver_udisk.dwBlockCountLo = 0;
        driver_udisk.dwBlockSize = 0;
        driver_udisk.InitFlag = 1;
        while (1)
        {
            ret_val = msg_get(&msg);
            if (MSG_FAILURE_RET != ret_val)
            {
                if (msg.id != MSG_KEY_MODE)
                {
                    msg_put(msg.id);
                }
                else
                    break;
            }
            if (timeout_handle())
            {
                ret = USB_RET_ERROR;
                break;
            }

            ret = MUSB_NoneRunBackground();
            if (MGC_MsdGetMediumstatus())
            {
                driver_udisk.dwBlockCountLo = get_HfiMedium_Block_CountLo();
                driver_udisk.dwBlockSize    = get_HfiMedium_Block_Size();
                ret = USB_RET_OK;
                break;
            }
            else
            {
                if ((USB_RET_DISCONNECT == ret)||(USB_RET_ERROR == ret))
                    break;
            }
        }
        disable_timeout_timer();
    }*/
    MUSB_DPRINTF("===udisk init end: ret=%x\r\n",ret);
    return ret;
}

void udisk_uninit(void)
{
#if 0
    uint8 tmp0,tmp1,tmp2;
    tmp0 = REG_USB_INTRUSBE;
    tmp1 = REG_USB_INTRRX1E;
    tmp2 = REG_USB_INTRTX1E;
    MUSB_DPRINTF("udisk_uninit\r\n");
    if (driver_udisk.InitFlag == 1)
    {
        driver_udisk.InitFlag = 0;
    	MUSB_Host_uninit();
    }
/*    if (!udisk_is_attached())
    {
    	REG_AHB2_USB_RESET = 1;
    	
    	REG_AHB2_USB_OTG_CFG = 0x50;
        Delay(5);
    	uint8 ahb_int_state = REG_AHB2_USB_INT;
    	Delay(5);
    	REG_AHB2_USB_INT = ahb_int_state;
    	Delay(5);
    	
    	REG_USB_INTRUSBE = tmp0;
    	REG_USB_INTRRX1E = tmp1;
    	REG_USB_INTRTX1E = tmp2;
    	
        REG_AHB2_USB_GEN |= (0x7<<4);   //  DP EN
        REG_AHB2_USB_GEN |= (0x7<<0);   //  DN EN
    }*/
#endif
}

//extern uint8 test_timeout_print(void);
int udisk_rd_blk_sync(uint32 first_block, uint32 block_num, uint8 *dest )
{
    int ret = USB_RET_ERROR;

    if (!MGC_MsdGetMediumstatus())
        return ret;

    enable_timeout_timer(1);
    ret = MUSB_HfiRead(first_block,block_num,dest);
    if (ret)
        goto Exit;

    while(1)
    {
        if (timeout_handle())
        {
            ret = USB_RET_ERROR ;
            break;
        }

        ret = MUSB_NoneRunBackground();
        if ( (USB_RET_ERROR == ret) 
            ||(USB_RET_DISCONNECT == ret) 
            ||(USB_RET_READ_OK == ret) )
            break;
    }

    if (USB_RET_READ_OK == ret)
        ret = USB_RET_OK;
    else
        ret = USB_RET_ERROR;
Exit:
    //test_timeout_print();
    disable_timeout_timer();
    return ret;
}

uint32 udisk_get_size(void)
{
    return driver_udisk.dwBlockSize;
}

void usb_isr(void)
{
    uint8 bIntrUsbValue = REG_USB_INTRUSB;
    uint16 wIntrTxValue = REG_USB_INTRTX1 | (REG_USB_INTRTX2 << 8);
    uint16 wIntrRxValue = REG_USB_INTRRX1 | (REG_USB_INTRRX2 << 8);

#if 0//def CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    extern boolean SYSpwr_Is_Low_Power_Mode_Active(void);
    extern void SYSpwr_Handle_Early_Wakeup(void);
    
    app_exit_sniff_mode();
    SYSpwr_Handle_Early_Wakeup();
#endif
//	os_printf("usb_isr in\r\n");
    MUSB_DPRINTF("------------------- USB_InterruptHandler in  ---------------------\r\n");
    MUSB_DPRINTF("bIntrUsbValue = 0x%x, wIntrTxValue = 0x%x, wIntrRxValue = 0x%x, REG_USB_DEVCTL = 0x%x, REG_AHB2_USB_INT = 0x%x\r\n",
            bIntrUsbValue, wIntrTxValue, wIntrRxValue, REG_USB_DEVCTL, REG_AHB2_USB_INT);
    bIntrUsbValue &= ~(0xc0);
    MGC_AfsUdsIsr(wIntrTxValue, wIntrRxValue, bIntrUsbValue);
    MUSB_DPRINTF("------------------- USB_InterruptHandler out ---------------------\r\n");
//	os_printf("usb_isr out\r\n");
}
#endif

