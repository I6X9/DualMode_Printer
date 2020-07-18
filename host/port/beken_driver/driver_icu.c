#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "drv_tl420.h"

#if 0
__inline void clear_bit(volatile uint32 *p,uint32 count,...)
{
    uint32 val;
    uint8 i = 0;
    int bit_no;
    va_list args;

    va_start(args, count);

    val = 0;
    for(;i < count;i++)
    {
        bit_no = va_arg(args,int);
        val |= (1 << bit_no);
    }

    (*p) &= (~(val));

    va_end(args);
}

__inline void set_bit(volatile uint32 *p,uint32 count,...)
{
    uint32 val;
    uint8 i = 0;
    int bit_no;
    va_list args;

    va_start(args, count);

    val = 0;
    for(;i < count;i++)
    {
        bit_no = va_arg(args,int);
        val |= (1 << bit_no);
    }

    (*p) |= val;

    va_end(args);
}
#endif
#if 0
void ldo_enable(void)
{
	BK3000_A3_CONFIG |= sft_GPIO_LDO_en;
}

void ldo_disable(void)
{
	BK3000_A3_CONFIG &= ~sft_GPIO_LDO_en;
}
#endif

#define PLL0	(0)
#define PLL1	(1)

void analog_pll_config(uint32_t pll, uint32_t freq)
{
	switch(pll)
	{
	case PLL0:
		{
			uint32_t div;
			uint32_t reg = 0x02C80400;

			if(freq > 100)
			{
				div = freq * 64 / 26;
				reg = ((~0x01FF0000) & reg) + ((div & 0x1FF) << 16) + (0 << 12);
			}
			else
			{
				div = freq * 96 / 26;
				reg = ((~0x01FF0000) & reg) + ((div & 0x1FF) << 16) + (1 << 12);
			}

			BK3000_A0_CONFIG &= ~(1 << 26); // DCO power ON;
			delay_us(5);
			BK3000_XVR_REG_0x06 = reg;
			reg &= 0xFFFF7FFF;
			BK3000_XVR_REG_0x06 = reg;
			delay_us(5);
			reg |= 0x00008000;
			BK3000_XVR_REG_0x06 = reg;
			delay_us(5);
			reg &= 0xFFFF7FFF;
			BK3000_XVR_REG_0x06 = reg;
			delay_us(5);
			reg |= 0x00008000;
			BK3000_XVR_REG_0x06 = reg;
			delay_us(5);

			XVR_analog_reg_save[6] = reg;
		}
		break;

	case PLL1:
		{
			#if 0

			BK3000_XVR_REG_0x05 = XVR_analog_reg_save[5] = 0x4DC09A0;
			BK3000_A0_CONFIG   &= ~(1 << 18);

			#else

			uint32_t reg = 0x00800000;
			uint32_t div;
			uint32_t ictrl;
			uint32_t dctrl;

			//FIXME: 已下表格存在问题
			// fck(MHz)		    Recommended value
			// min	max	 ictrl<3:0>	divctrl<1:0>	fvco
			// 28	35	    0	        3	        fck*6
			// 35	40	    1	        3
			// 40	50	    2	        3
			// 50	60	    4	        3
			// 60	70	    0	        1
			// 70	80	    1	        1
			// 80	100	    2	        1
			// 100	120	    4	        1	        fck*3
			// 120	145	    2	        0
			// 145	170	    3	        0	        fck*2
			// 170	205	    5	        0

			if(freq < 35)
			{
				ictrl = 0;
				dctrl = 3;
			}
			else if(freq < 40)
			{
				ictrl = 1;
				dctrl = 3;
			}
			else if(freq < 50)
			{
				ictrl = 2;
				dctrl = 3;
			}
			else if(freq < 60)
			{
				ictrl = 4;
				dctrl = 3;
			}
			else if(freq < 70)
			{
				ictrl = 0;
				dctrl = 1;
			}
			else if(freq < 80)
			{
				ictrl = 1;
				dctrl = 1;
			}
			else if(freq < 100)
			{
				ictrl = 2;
				dctrl = 1;
			}
			else if(freq < 120)
			{
				ictrl = 4;
				dctrl = 1;
			}
			else if(freq < 145)
			{
				ictrl = 2;
				dctrl = 0;
			}
			else if(freq < 170)
			{
				ictrl = 3;
				dctrl = 0;
			}
			else
			{
				ictrl = 5;
				dctrl = 0;
			}

			div  = (freq * (dctrl == 3 ? 6 : (2 + dctrl)) * 32 + 13 ) / 26;
			reg |= (dctrl << 10) | (div << 14) | (ictrl << 24);

			BK3000_A0_CONFIG &= ~(1 << 18); // DCO power ON;
			delay_us(5);
			BK3000_XVR_REG_0x05 = reg;
			reg &= ~(1 << 13);
			BK3000_XVR_REG_0x05 = reg;
			delay_us(5);
			reg |= (1 << 13);
			BK3000_XVR_REG_0x05 = reg;
			delay_us(5);
			reg &= ~(1 << 13);
			BK3000_XVR_REG_0x05 = reg;
			delay_us(5);
			reg |= (1 << 13);
			BK3000_XVR_REG_0x05 = reg;
			delay_us(5);

			XVR_analog_reg_save[5] = reg;

			#endif
		}
		break;

	default:
		break;
	}
}

static volatile boolean b_26m_clock_closed   = FALSE;
static volatile u_int32 backup_pmu_peri_pwds = 0XFFFFFFFF;

DRAM_CODE void BK3000_set_clock (int clock_sel, int div)
{
#ifdef TRACE32_DEBUG
    return;
#endif
#ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    if (b_26m_clock_closed)
    {
        return;
    }
#endif
#if ((CONFIG_CHARGE_EN == 1))
    if(bt_flag2_is_set(APP_FLAG2_CHARGE_POWERDOWN))
    {
        clock_sel = 1;
        div = 0;
    }
#endif

#ifdef BEKEN_OTA
    if(app_ota_is_ongoing()) 
    {    
        clock_sel = CPU_CLK_SEL;
        div = CPU_CLK_DIV;
    }
#endif

#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
    u_int32 cpu_sel = 0;
    u_int32 div_sel = 0;
    cpu_sel = (BK3000_PMU_CONFIG >> sft_PMU_CPU_CLK_SEL) & 0x03;
    div_sel = (BK3000_PMU_CONFIG >> sft_PMU_CPU_CLK_DIV) & 0x7f;
    if((cpu_sel == clock_sel)&&(div_sel == div))
        return;
#endif	
    //os_printf("%d,%d,%d,%d\r\n",cpu_sel,div_sel,clock_sel,div);
    BK3000_PMU_WAKEUP_INT_MASK = 0;
    BK3000_PMU_CONFIG = (clock_sel << sft_PMU_CPU_CLK_SEL)
                      | (div << sft_PMU_CPU_CLK_DIV
                      /* | PMU_AHB_CLK_ALWAYS_ON */);
}

DRAM_CODE void BK3000_cpu_halt(void) 
{
    BK3000_PMU_CONFIG |= 1;	/* MCU CLK HALOT */
}

uint8 g_sniff_flag = 0;
DRAM_CODE void BK3000_cpu_pre_halt(void) 
{
#if 0    
    //if(g_sniff_flag == 0x02) // active=0,hold=1,sniff=2,park=3
    {
        BK3000_A0_CONFIG |= (1<<25); /* controlled by DSP */
        delay_us(10);
#if ( BUTTON_DETECT_IN_SNIFF == 1)
        BK3000_PMU_WAKEUP_INT_MASK = 0x00080300; // ceva not mask, gpio int not mask
#else
        BK3000_PMU_WAKEUP_INT_MASK = 0x00080000;
#endif
        BK3000_GPIO_WKUPEN &= ~0x10000000; // select sleep mode
        BK3000_GPIO_PAD_CTRL |= (0x1f<<3);
        BK3000_GPIO_DPSLP = 0x3261;
        delay_us(1);
        //BK3000_PMU_CONFIG |= 1;
    }
#endif

#if 1
    {   
        #if 1
        if(g_sniff_flag == 2)   // 2 = sniff mode
        {
            //shutdown_dsp(); 
            dsp_halt_clk();         
        }
        else                    // inquiry/page scan 
        {
            dsp_halt_clk();    
        }
        #endif
		BK3000_PMU_WAKEUP_INT_MASK = 0x00000000;
    //BK3000_PMU_CONFIG |= 1;	/**< CPU时钟停止使能 -- 1: CPU Halt, 0: CPU Going (中断产生，自动清零） */
    }
#endif
    
}

//static u_int8 s_ana_dac_open = 0;
RAM_CODE void Close_26M_Clock(void)
{
    u_int32 cpu_flags, mask;
    //uint8 i=0;
#if 0
    app_env_handle_t  env_h = app_env_get_handle();
#endif

    if (b_26m_clock_closed)
        return;

    SYSirq_Disable_Interrupts_Save_Flags(&cpu_flags, &mask);

    CLEAR_SLEEP_TICK;
#if 0
    os_printf("--------------------------------------\r\n");
    os_printf("|RegA0:%08x\r\n",BK3000_A0_CONFIG);
    os_printf("|RegA1:%08x\r\n",BK3000_A1_CONFIG);
    os_printf("|RegA2:%08x\r\n",BK3000_A2_CONFIG);
    os_printf("|RegA3:%08x\r\n",BK3000_A3_CONFIG);
    os_printf("|RegA4:%08x\r\n",BK3000_A4_CONFIG);
    os_printf("|RegA5:%08x\r\n",BK3000_A5_CONFIG);
    os_printf("|RegA6:%08x\r\n",BK3000_A6_CONFIG);
    os_printf("|RegA7:%08x\r\n",BK3000_A7_CONFIG);
    for(i=0;i<16;i++)
        os_printf("|xvr[%d]:%08x\r\n",i,XVR_analog_reg_save[i]);
    os_printf("--------------------------------------\r\n");
#endif
    /* Step 1: close BT xvr */
    BK3000_XVR_REG_0x03 = XVR_analog_reg_save[3] & ~(0x01 << 0);
    BK3000_XVR_REG_0x09 =XVR_analog_reg_save[9]= 0x2BFFF304;
    backup_pmu_peri_pwds = BK3000_PMU_PERI_PWDS;

    if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_DAC_ALWAYS_ON)
        && app_env_check_sniff_mode_Enable())
    {
        BK3000_A6_CONFIG |= (1 << 0);
    }
    /* Step 2: close audio */
    //BK3000_Ana_Dac_enable(0);
    //aud_close();
    /* Step 3: set cpu clk = xtal 26M */
    BK3000_set_clock(1, 0);

    /* Step 4: set flash parameters */
    set_flash_clk(FLASH_CLK_26mHz);
    flash_set_line_mode(FLASH_LINE_2);

    /* Step 5: power down peripheral clock*/
#ifdef CONFIG_PWM_NOT_SLEEP
    extern uint8 g_sniff_flag;
    if(g_sniff_flag == 2) // enter sniff mode
    {
        BK3000_PMU_PERI_PWDS |= 0x00703FFF;
    }
    else
        BK3000_PMU_PERI_PWDS |= 0x00703BFF;  /* if have on connections,open pwm timer for LED */
#else
    BK3000_PMU_PERI_PWDS |= 0x00703FFF;
#endif

    /* Step 6: Optimizaiotn BUCK */
#if 0
    if( env_h->env_cfg.used == 0x01 )
    {
        char enable;
        enable = (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_BUCK_ENABLE) ? 1:0;

        if(enable)
        {
#if 0
        	/* 20160808 */
	        BK3000_A1_CONFIG  = (BK3000_A1_CONFIG & (~(0x1<<22)));
        	BK3000_A2_CONFIG  = (BK3000_A2_CONFIG & (~(0x1<<22)));
        	BK3000_A1_CONFIG |= (1 << 28);
#endif
            GPIO_Ax.a1->bustconspiEn = 0;// burst mode controlled by digital(auto)
            GPIO_Ax.a5->spiselDigLDO30 = 0x06;
        }
        else
        {
            GPIO_Ax.a5->spiselDigLDO30 = 0x07;
        }
    }
    else
    {
#if ( SYS_CFG_BUCK_ON == 1)
#if 0
        BK3000_A1_CONFIG  = (BK3000_A1_CONFIG & (~(0x1<<22)));
    	BK3000_A2_CONFIG  = (BK3000_A2_CONFIG & (~(0x1<<22)));
    	BK3000_A1_CONFIG |= (1 << 28);
#endif
        GPIO_Ax.a1->bustconspiEn = 0;// burst mode controlled by digital(auto)
        GPIO_Ax.a5->spiselDigLDO30 = 0x06;
#else
        GPIO_Ax.a5->spiselDigLDO30 = 0x07;
#endif
    }
#endif

    /* Step 7: close saradc */
    #if 0
    GPIO_Ax.a3->spipwdSarADCA = 1;
    GPIO_Ax.a3->spipwdSarADCBuffer = 1;
    GPIO_Ax.a3->spipwdSARADCLDO = 1;
    #endif

    /* Step 8: close RF clk/ldo and DCO */
    BK3000_A0_CONFIG |= (1<<29);   //xtal buff off;
    //BK3000_A0_CONFIG &= ~(1<<20);   //bt ldo off;
    BK3000_A0_CONFIG |= (1<<26);  // DCO power off;
    //BK3000_A0_CONFIG &= ~(1<<15);  // cbias off, DCO switch
    BK3000_A1_CONFIG |= (1<<31);   // BUCKA pfm

    /* Step 9: setting for GPIO wakeup */
#if 0
#if ( BUTTON_DETECT_IN_SNIFF == 1)
    GPIO_Ax.a3->spipwdrsc100K  = 0;
#else
	GPIO_Ax.a3->spipwdrsc100K = 1;
#endif
#endif

    /* Step 10: close DSP power */
    //REG_DSP_CLK |= MSK_DSP_CLK_HALT;

    b_26m_clock_closed = TRUE;
    SYSirq_Interrupts_Restore_Flags(cpu_flags, mask);
}
#if 0//def BEKEN_DEBUG
extern uint32 sleep_slots;
#endif
RAM_CODE void Open_26M_Clock(void)
{
    u_int32 cpu_flags, mask;

    if (!b_26m_clock_closed)
        return;

    SYSirq_Disable_Interrupts_Save_Flags(&cpu_flags, &mask);
    BK3000_GPIO_47_CONFIG = 0x01;  // clear gpio latch;
    /* Step 0:  */
    BK3000_A0_CONFIG |= (1<<15);  // cbias on;
    //BK3000_Ana_PLL_enable(CPU_DCO_CLK);
    analog_pll_config(PLL0, CPU_DCO_CLK);
    BK3000_A1_CONFIG &= ~(1<<31);   // BUCKA pfm
    BK3000_A0_CONFIG &= ~(1<<29);   //xtal buff on;
    //BK3000_A0_CONFIG |= (1<<20);   //bt ldo on;



    /* Step 1: Power on CEVA clock */
    BK3000_PMU_PERI_PWDS &= (~bit_PMU_CEVA_PWD);
    /* Step 2: Open saradc and ...*/
#if 0
    GPIO_Ax.a3->spipwdxtalbufa = 0;
    GPIO_Ax.a3->spipwdSarADCA = 0;
    GPIO_Ax.a3->spipwdSarADCBuffer = 0;
    GPIO_Ax.a3->spipwdSARADCLDO = 0;
    GPIO_Ax.a3->spipwdrsc100K     = 0;    /**< 打开100K时钟 */
#endif

    /* Step 3: Power on peripheral */
    BK3000_PMU_PERI_PWDS = backup_pmu_peri_pwds;

    b_26m_clock_closed = FALSE;
    /* cpu clk 1--->3---2 */

    BK3000_set_clock(CPU_CLK_XTAL, 2);				/* 26M/2  xtal  */

    /* Step 5: set cpu clk */
#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#else
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#endif
    
    /* Step 6: set flash */
    flash_config();

    /* Step 7: Open BT xvr 52M */
    //BK3000_XVR_REG_0x09 = XVR_analog_reg_save[9]&(~(1<<11));
    BK3000_XVR_REG_0x09 = XVR_analog_reg_save[9] = 0x23DFF304;//0x23DFF304;//0x2BDFF304;
    BK3000_XVR_REG_0x03 = XVR_analog_reg_save[3] | (0x01 << 0);

    /* Step 8: set uart */
#if (CONFIG_UART_IN_SNIFF == 1)
    BK3000_GPIO_1_CONFIG = 0x7C;
#endif

    /* Step 9: open DSP power */
    //REG_DSP_CLK &= ~MSK_DSP_CLK_HALT;
    dsp_restart_clk();
//    BK3000_GPIO_Initial();
    SYSirq_Interrupts_Restore_Flags(cpu_flags, mask);
}
extern u_int32 XVR_analog_reg_save[];
void BK3000_Ana_PLL_enable( u_int16 freq )
{
#if 0
	u_int16 cnt_i ;
    BK3000_A0_CONFIG &= ~(1<<26);  // DCO power ON;
	delay_us(5);
    XVR_analog_reg_save[6] = 0x02C80400;//default value;
    if(freq>100)
    {
	    cnt_i = freq * 64 / 26 ;
	    XVR_analog_reg_save[6] = ((~0x01FF3000) & XVR_analog_reg_save[6]) + ((cnt_i & 0x1FF)<<16) + (0x00 << 12);
    }
    else
    {
	    cnt_i = freq * 96 / 26 ;
	    XVR_analog_reg_save[6] = ((~0x01FF3000) & XVR_analog_reg_save[6]) + ((cnt_i & 0x1FF)<<16) + (0x01 << 12);
    }
	BK3000_XVR_REG_0x06 = XVR_analog_reg_save[6] ;
	BK3000_XVR_REG_0x06 = XVR_analog_reg_save[6] = XVR_analog_reg_save[6] & 0xFFFF7FFF ;
    delay_us(5);
	BK3000_XVR_REG_0x06 = XVR_analog_reg_save[6] = XVR_analog_reg_save[6] | 0x00008000 ;
	delay_us(5);
	BK3000_XVR_REG_0x06 = XVR_analog_reg_save[6] = XVR_analog_reg_save[6] & 0xFFFF7FFF ;
	delay_us(5);
	BK3000_XVR_REG_0x06 = XVR_analog_reg_save[6] = XVR_analog_reg_save[6] | 0x00008000 ;
	delay_us(5);

    BK3000_XVR_REG_0x05 = XVR_analog_reg_save[5] = 0x4DC09A0;

#else

    analog_pll_config(PLL0, freq);
    analog_pll_config(PLL1, 180);

#endif
}
/* DCO table */
/*
122	300.3076923	300	12C
123	302.7692308	303	12F
124	305.2307692	305	131
125	307.6923077	308	134
			
120	295.3846154	295	127
96	354.4615385	236	EC
*/
static uint16_t s_DCO_table[2][5] = 
{
    {0xfa, 0xfc, 0xfe, 0x100,0x103},
    {0x12c,0x12f,0x131,0x134,0x127}
};
DRAM_CODE void BK3000_Set_DCO_By_Chnl(uint8 chnl)
{
    return ;
    volatile u_int8 i =0;
    uint16 *p_dco_tbl;
    if(BK3000_GPIO_DRB & (1 << 1)) // usb plug in
		return;
    #if(CPU_DCO_CLK == 120)
    p_dco_tbl = s_DCO_table[1]; // 120MHz
    #else
    p_dco_tbl = s_DCO_table[0]; // 96 or 104MHz
    #endif

    BK3000_set_clock(1,0);
    for(i = 0; i < 5; i ++);
    XVR_analog_reg_save[6] = 0x02C80400;//default value;
    XVR_analog_reg_save[6] &= ~(0x1ff<<16);
    XVR_analog_reg_save[6] &= ~(0x03<<12);
    XVR_analog_reg_save[6] &= ~(0x07<<9);
    if(chnl<20)
        XVR_analog_reg_save[6] |= (p_dco_tbl[0] << 16);
    else if(chnl < 40)
        XVR_analog_reg_save[6] |= (p_dco_tbl[1] << 16);
    else if(chnl<60)
        XVR_analog_reg_save[6] |= (p_dco_tbl[2] << 16);
    else if(chnl == 0xff)  // not change dco
        XVR_analog_reg_save[6] |= (p_dco_tbl[4] << 16);
    else
        XVR_analog_reg_save[6] |= (p_dco_tbl[3] << 16);
    XVR_analog_reg_save[6] |= (0x2 << 9);
    BK3000_XVR_REG_0x06 = XVR_analog_reg_save[6] ;
    BK3000_XVR_REG_0x06 = XVR_analog_reg_save[6] = XVR_analog_reg_save[6] & 0xFFFF7FFF ;
    delay_us(1);
    BK3000_XVR_REG_0x06 = XVR_analog_reg_save[6] = XVR_analog_reg_save[6] | 0x00008000 ;
    delay_us(1);
    BK3000_XVR_REG_0x06 = XVR_analog_reg_save[6] = XVR_analog_reg_save[6] & 0xFFFF7FFF ;
    delay_us(1);
    BK3000_XVR_REG_0x06 = XVR_analog_reg_save[6] = XVR_analog_reg_save[6] | 0x00008000 ;
    delay_us(1);
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
    for(i = 0; i < 5; i ++);
}


void BK3000_set_ana_dig_voltage(uint8 ana,uint8 dig)
{
    uint32 reg;
    reg = BK3000_A4_CONFIG;
    reg &= ~(0x03 << 10);     //  analog voltage;
    reg &= ~(0x03 << 6);      // digital voltage;
    reg |= (ana << 10);
    reg |= (2 << 6); //dig
    BK3000_A4_CONFIG = reg;
}
void BK3000_ana_buck_enable( char enable)
{
    if(enable)
    {
        BK3000_A2_CONFIG &= ~(1<<10);
        delay_us(10);
        BK3000_A2_CONFIG &= ~(1<<18);
        delay_us(100);
        BK3000_A2_CONFIG |=  (1<<18);
        delay_us(100);
    }
    else
    {
        BK3000_A2_CONFIG |= (1<<10);
    }
    return;
}
void BK3000_Dig_Buck_open( char enable )
{
    if(enable)
    {
        BK3000_A1_CONFIG &= ~(1 << 20);
    }
    else
    {
        BK3000_A1_CONFIG |= (1 << 20);
    }
}
static void BK3000_analog_reg_init( void )
{
#if 0//(CONFIG_APP_TOOLKIT_5 == 0)&&(SYS_CFG_BUCK_ON ==  1)  // buck on
    BK3000_A0_CONFIG = 0xC706B621; //0xE706B621; C706B621 0xC716B621
    delay_us(2000);
    BK3000_A1_CONFIG = 0x1F03FE6C;//0x9F09FC6C;
    BK3000_A2_CONFIG = 0x7E12A99C;//0x7E12BF9C;
    delay_us(2000);
    BK3000_A0_CONFIG = 0xC716B621;
    delay_us(2000);
    BK3000_A3_CONFIG = 0x180004A0;//0x180004A0;
    BK3000_A4_CONFIG = 0x84200E7F;//0x84200A7F;//0x84200A5F;84200E7F
    BK3000_A5_CONFIG  = 0x8080F788 ;
    BK3000_A6_CONFIG  = 0x683B1846 ;
    BK3000_A7_CONFIG  = 0x82206004 ;
#else  // ldo
    BK3000_A0_CONFIG = 0xC706B621; //0xE706B621;
    delay_us(2000);
    BK3000_A1_CONFIG = 0x1F13BE7C;//0x1F13FE6C;//0x9F09FC6C;
    BK3000_A2_CONFIG = 0x7E133D9C;//0x7E12AD9C;//0x7E12BF9C;  0x7E132D9C vddio:2--3
    delay_us(2000);
    BK3000_A0_CONFIG = 0xC716B621;
    delay_us(2000);
    BK3000_A3_CONFIG = 0x180004A0;//0x180004A0;
    BK3000_A4_CONFIG = 0x84200E7F;//0x84200A7F;//0x84200A5F;84200E7F
    BK3000_A5_CONFIG  = 0x8080F788;
    BK3000_A6_CONFIG  = 0x683B1846 ;
    BK3000_A7_CONFIG  = 0x82206004 ;
#endif

    BK3000_A0_CONFIG &= ~(1 << 18);

    BK3000_PMU_GATE_CFG = 0x800;
}
void BK3000_ICU_Initial(void)
{
    BK3000_PMU_CONFIG = (1 << sft_PMU_CPU_CLK_SEL)
                      | (0 << sft_PMU_CPU_CLK_DIV);
    //delay_us(1000);
    BK3000_analog_reg_init();
    set_flash_ctrl_config();	/*Read Flash status register to Flash controller config register*/
    BK3000_Dig_Buck_open(SYS_CFG_BUCK_ON);          
    BK3000_ana_buck_enable(SYS_CFG_BUCK_ON);      

//    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);   /**< CPU时钟源的选择--  0: ROSC, 1: XTAL(26MHz), 2:PLL0(39MHz), 3:PLL1(78MHz) */
    /*
    BK3000_PMU_CONFIG = (CPU_CLK_SEL << sft_PMU_CPU_CLK_SEL)
                      | (CPU_CLK_DIV << sft_PMU_CPU_CLK_DIV);
    */
    //set_flash_qe();                               /**< 4线模式打开 */
    //flash_set_line_mode(FLASH_LINE_2);
    //flash_set_line_mode(DEFAULT_LINE_MODE);
    //set_flash_clk(FLASH_CLK_SEL);                 /**< FLASH CLK 4 : 78M, 8 : 26M */
    BK3000_GPIO_DEEP_SLEEP_LATCH = 0x1;


    BK3000_PMU_PERI_PWDS &= ~bit_PMU_UART_PWD;
    BK3000_PMU_PERI_PWDS &= ~bit_PMU_CEVA_PWD;               // enable CEVA baseband clock
    //BK3000_PMU_PERI_PWDS &= ~bit_PMU_SDIO_PWD;               // enable SDCard clock
    BK3000_PMU_PERI_PWDS &= ~bit_PMU_LPO_CLK_STEAL_ENABLE;   // Clock Stealing Enable to get 32k clock
    BK3000_PMU_PERI_PWDS |= bit_PMU_LPO_CLK_SEL_XTAL;        // 1:XTAL分频 0:ROSC输入 32.768
    BK3000_PMU_PERI_PWDS &= ~bit_PMU_LPO_CLK_DISABLE;        // enable LPO clock using for 32K clock
    BK3000_PMU_PERI_PWDS |= bit_PMU_CLK_FREQ_SEL;
}
void BK3000_start_wdt( u_int32 val )
{
#ifdef TRACE32_DEBUG
    return;
#endif
    BK3000_WDT_CONFIG = 0x5A0000|val;
    BK3000_WDT_CONFIG = 0xA50000|val;
}

void BK3000_wdt_power_on(void)
{
#ifdef TRACE32_DEBUG
    return;
#endif
    BK3000_WDT_CONFIG = 0x5A0000|0x3fff;
    BK3000_WDT_CONFIG = 0xA50000|0x3fff;
}

void BK3000_stop_wdt(void )
{
    BK3000_WDT_CONFIG = 0x5A0000;
    BK3000_WDT_CONFIG = 0xA50000;
}

void BK3000_wdt_reset(void )
{
#ifdef TRACE32_DEBUG
    return;
#endif
    BK3000_WDT_CONFIG = 0x5A0001;
    BK3000_WDT_CONFIG = 0xA50001;
}

static void app_set_powerdown_gpio(void)
{
    uint8 i=0;
    app_handle_t app_h = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();

    if((env_h->env_cfg.system_para.system_flag&APP_ENV_SYS_FLAG_LED_REVERSE) >> 9)
    {
        for(i = 0; i < LED_NUM; i++)
        {
            if(app_h->led_map[i] == LED_INVALID_INDEX)
                continue;
            gpio_config(app_h->led_map[i],1);	
            gpio_output(app_h->led_map[i],1);
        }
    }	
    aud_PAmute_oper(1);
    if ((env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_PAMUTE_HIGH) >> 6 )
    {
        gpio_config(env_h->env_cfg.system_para.pamute_pin,1);	
        gpio_output(env_h->env_cfg.system_para.pamute_pin,1);
    }	
}

extern uint8 app_get_auto_powerdown(void);
static void BK3000_icu_deepsleep(uint8 wakup_pin,t_powerdown_mode pwrdown_mode)
{
    int i,jitter_low_cnt = 0,jitter_high_cnt = 0;
    app_env_handle_t  env_h = app_env_get_handle();
    uint8 high_flag = (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_PWRCTRL_HIGH) ? 1:0 ;

    os_printf("icu_deepsleep:%d,%d\r\n",wakup_pin,high_flag);
    Delay(1000);
    set_spr( SPR_VICMR(0), 0); 
    Delay(10);
    cpu_set_interrupts_enabled(0);
    BK3000_PMU_CONFIG = (1 << sft_PMU_CPU_CLK_SEL)
                          | (0 << sft_PMU_CPU_CLK_DIV
                          /* | PMU_AHB_CLK_ALWAYS_ON */);             
    set_flash_clk(FLASH_CLK_26mHz);     
    flash_set_line_mode(FLASH_LINE_2);
    if(pwrdown_mode == POWERDOWN_SELECT)
    {
        if(GPIO_CHARGER_FLAG)
        {
            bt_flag2_operate(APP_FLAG2_CHARGE_POWERDOWN,0);
            BK3000_start_wdt(0x1ff);
            while(1);
            //return;
        }
    }
    /* Step 1: close RF cb and 52M pll */
    BK3000_XVR_REG_0x09 =XVR_analog_reg_save[9] = 0x2BFFF304;
    /* Step 2: close ABB CB */
    BK3000_A0_CONFIG &= ~(1<<15);
    /* Step 3: close DCO */
    BK3000_A0_CONFIG |= (1<<26);
    /* Step 4: ana buck pfm mode */
    BK3000_A1_CONFIG &= ~(1<<31);

    /* if system working in LDO MODE */
    /* Step 5: dig buck ramp compensation disable */
    /* Step 6: ana buck ramp compensation disable */
    if(env_h->env_cfg.used == 0x01)
    {
    #if 1/*(CONFIG_APP_TOOLKIT_5 == 1)*/
        if(!app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_DIG_BUCK_ENABLE))
        {
            BK3000_A1_CONFIG &= ~(1<<15);
        }
        if(!app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_ANA_BUCK_ENABLE))
        {
            BK3000_A2_CONFIG &= ~(1<<7);
        }
    #elif(CHIP_PACKAGE_TSSOP_28 == 1) // LDO MODE
        BK3000_A1_CONFIG &= ~(1<<15);
        BK3000_A2_CONFIG &= ~(1<<7);
    #endif
    }
    else
    {
    #if(SYS_CFG_BUCK_ON == 0)    // LDO MODE
        BK3000_A1_CONFIG &= ~(1<<15);
        BK3000_A2_CONFIG &= ~(1<<7);
    #endif
    }
 
    Delay(100);
    for( i = 0; i <= GPIO_NUM; i++)
    {
        gpio_config(i,3);  // input & pullup;
    }

    //if (high_flag)
    // 	gpio_config(wakup_pin, 0);
    //else
    	gpio_config(wakup_pin, 3);

    /* the elimination of GPIO jitter */
    if(pwrdown_mode == POWERDOWN_SELECT)
    {
        while(app_get_auto_powerdown())
        {
            os_delay_ms(20);
            if(gpio_input(wakup_pin) == 0)//(gpio_input(wakup_pin) == (high_flag ^ 0x01))
            {
                jitter_high_cnt = 0;
                jitter_low_cnt++;
                if(jitter_low_cnt > 3) // if gpio detect is anti-high_flag level more than 3 times,then deep sleep!
                    break;
            }
            else
            {
                jitter_low_cnt = 0;
                jitter_high_cnt++;
                if(jitter_high_cnt > 0x0f) // if GPIO is high_flag level for a long time,then reset mcu;
                {
                    BK3000_start_wdt(0xff);
                    while(1);
                }
            }
        }

        if(gpio_input(wakup_pin) == 0)//(gpio_input(wakup_pin) == high_flag)
        {
            gpio_config(wakup_pin, 5);
            if (DEFAULT_WAKEUP_PIN == wakup_pin)
                gpio_config(7, 5); //gpio3 
        }
    }

    app_set_powerdown_gpio();
    Delay(10);
    
    BK3000_GPIO_DEEP_SLEEP_LATCH = 0x1;
    BK3000_GPIO_PAD_CTRL |= (0x1f<<3);
    /*
    * Bit 29: wakeup enable;
    * Bit xx: gpio pin(0:26)wakeup;
    * Bit 27: USB plug in wakeup;
    */
    BK3000_GPIO_WKUPEN  = ((1<<29) | (1<<wakup_pin) | (1<<27));
    BK3000_GPIO_DPSLP  = 0x3261;

    while(1);

}
#if 0//(CONFIG_CHARGER_DEEPSLEEP == 1)
static void BK3000_rtc_init(void)
{
    os_printf("BK3000_rtc_init()\n");
#define PERI_PWDS           (*((volatile unsigned long *)    0x00800004))

    // s1
    PERI_PWDS &= ~(1<<28); //32k_divd open

    // s2
    BK3000_GPIO_DEEP_SLEEP_LATCH = 0x1;

    os_printf ("rtc test start\n");
    os_delay_ms(2);

    // s3 reset RTC model
    BK3000_GPIO_58_CONFIG = 0;
    BK3000_GPIO_58_CONFIG |=   (1<<28) ;
    BK3000_GPIO_58_CONFIG &= (~(1<<28));
    os_delay_ms(2);
    BK3000_GPIO_58_CONFIG |= (1<<27);//rtc_supply_en open to check rtc status
    os_delay_ms(2);
    os_printf ("rtc first time\n");

    //0x3a[30:29]	rtc_clk_sel
    //0x3a[28]		rtc_latch_en
    //0x3a[27]		rtc_supply_en
    //0x3a[26]		RESERVED
    //0x3a[25]		rtc_clk_en
    //0x3a[24]		rtc_mode
    //0x3a[23]		rtc_int_clr


    // 1. loop mode
    /*
    BK3000_GPIO_61_CONFIG = 125;

    BK3000_GPIO_58_CONFIG |= ((0x1<<29) + (0x1<<25) + (0x1<<24));//(1<<27)already 1
    BK3000_GPIO_58_CONFIG |=   (1<<28) ;
    BK3000_GPIO_58_CONFIG &= (~(1<<28));
    */

    // 2. one-time mode

    // s4 how long
    BK3000_GPIO_61_CONFIG = 125 * 105; // about 60 seconds.

    // s5 config
    BK3000_GPIO_58_CONFIG |= ((0x1<<29) + (0x1<<25) + (0x0<<24));//(1<<27)already 1

    // s6 trigger RTC
    BK3000_GPIO_58_CONFIG |=   (1<<28) ;
    BK3000_GPIO_58_CONFIG &= (~(1<<28));

}

static void BK3000_deepsleep_with_rtc(uint8 wakup_pin,t_powerdown_mode pwrdown_mode)
{
    int i, jitter_low_cnt = 0, jitter_high_cnt = 0;

    app_env_handle_t  env_h = app_env_get_handle();

    uint8 high_flag = (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_PWRCTRL_HIGH) ? 1:0 ;

    //uint8 wakup_pin = env_h->env_cfg.system_para.wakup_pin+ GPIO0;

    //t_powerdown_mode pwrdown_mode = POWERDOWN_CHG_DEEPSLEEP;

    os_printf("BK3000_deepsleep_with_rtc: %d, %d\n",  wakup_pin, high_flag);

    BK3000_rtc_init();

    Delay(1000);
    set_spr(SPR_VICMR(0), 0);

    Delay(10);
    cpu_set_interrupts_enabled(0);

    BK3000_PMU_CONFIG = (1 << sft_PMU_CPU_CLK_SEL)
                          | (0 << sft_PMU_CPU_CLK_DIV
                          /* | PMU_AHB_CLK_ALWAYS_ON */);

    set_flash_clk(FLASH_CLK_26mHz);

    flash_set_line_mode(FLASH_LINE_2);

    if (pwrdown_mode == POWERDOWN_SELECT)
    {
        if (GPIO_CHARGER_FLAG)
        {
            bt_flag2_operate(APP_FLAG2_CHARGE_POWERDOWN,0);
            BK3000_start_wdt(0x1ff);
            while(1);
            //return;
        }
    }

    /* Step 1: close RF cb and 52M pll */
    BK3000_XVR_REG_0x09 = 0x2BFFF304;
    /* Step 2: close ABB CB */
    BK3000_A0_CONFIG &= ~(1<<15);
    /* Step 3: close DCO */
    BK3000_A0_CONFIG |= (1<<26);
    /* Step 4: ana buck pfm mode */
    BK3000_A1_CONFIG &= ~(1<<31);

    /* if system working in LDO MODE */
    /* Step 5: dig buck ramp compensation disable */
    /* Step 6: ana buck ramp compensation disable */
    if (env_h->env_cfg.used == 0x01)
    {
        #if(CONFIG_APP_TOOLKIT_5 == 1)
            if (!app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_DIG_BUCK_ENABLE))
            {
                BK3000_A1_CONFIG &= ~(1<<15);
            }

            if (!app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_ANA_BUCK_ENABLE))
            {
                BK3000_A2_CONFIG &= ~(1<<7);
            }
        #elif(CHIP_PACKAGE_TSSOP_28 == 1) // LDO MODE
            BK3000_A1_CONFIG &= ~(1<<15);
            BK3000_A2_CONFIG &= ~(1<<7);
        #endif
    }
    else
    {
        #if(SYS_CFG_BUCK_ON == 0)    // LDO MODE
            BK3000_A1_CONFIG &= ~(1<<15);
            BK3000_A2_CONFIG &= ~(1<<7);
        #endif
    }

    Delay(100);

    for (i = 0; i <= 26; i++)
    {
        gpio_config(i,0);  // High impedance
    }

    if (high_flag)
    	gpio_config(wakup_pin, 0);
    else
    	gpio_config(wakup_pin, 3);

    /* the elimination of GPIO jitter */
    if (pwrdown_mode == POWERDOWN_SELECT)
    {
        while (app_get_auto_powerdown())
        {
            os_delay_ms(20);
            if (gpio_input(wakup_pin) == (high_flag ^ 0x01))
            {
                jitter_high_cnt = 0;
                jitter_low_cnt++;
                if(jitter_low_cnt > 3) // if gpio detect is anti-high_flag level more than 3 times,then deep sleep!
                    break;
            }
            else
            {
                jitter_low_cnt = 0;
                jitter_high_cnt++;
                if(jitter_high_cnt > 0x0f) // if GPIO is high_flag level for a long time,then reset mcu;
                {
                  	BK3000_start_wdt(0xff);
                    while(1);
                }
            }
        }
    }

    if (gpio_input(wakup_pin) == high_flag)
    {
        gpio_config(wakup_pin, 5);
        if (wakup_pin == 22)
            gpio_config(7, 5);
    }

    app_set_powerdown_gpio();
    Delay(10);

    BK3000_GPIO_DEEP_SLEEP_LATCH = 0x1;
//    BK3000_GPIO_PAD_CTRL |= (0x1f<<3);
    BK3000_GPIO_PAD_CTRL &= (~(0x1F << 3));
    BK3000_GPIO_PAD_CTRL |= (0x1d << 3);

    /*
    * Bit 29: wakeup enable;
    * Bit xx: gpio pin(0:26)wakeup;
    * Bit 27: USB plug in wakeup;
    * Bit 28: RTC wakeup;
    */
    BK3000_GPIO_WKUPEN  = ((1<<29) | (1<<wakup_pin) | (1<<28));
    if (POWERDOWN_SELECT == pwrdown_mode)
        BK3000_GPIO_WKUPEN |= (1<<27);

    //BK3000_GPIO_WKUPEN  = 0;
    //BK3000_GPIO_WKUPEN  |= ((1<<29) | (1<<wakup_pin) | (1<<28));

    BK3000_GPIO_DPSLP  = 0x3261;

    while(1);
}

static void BK3000_deepsleep_with_gpio(uint8 wakup_pin,t_powerdown_mode pwrdown_mode)
{
    int i = 0, jitter_low_cnt = 0, jitter_high_cnt = 0;

    app_env_handle_t  env_h = app_env_get_handle();

    uint8 high_flag = (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_PWRCTRL_HIGH) ? 1:0 ;

    os_printf("BK3000_deepsleep_with_gpio: %d, %d\n",  wakup_pin, high_flag);

    set_spr(SPR_VICMR(0), 0);

    Delay(10);
    cpu_set_interrupts_enabled(0);

    BK3000_PMU_CONFIG = (1 << sft_PMU_CPU_CLK_SEL)
                                      | (0 << sft_PMU_CPU_CLK_DIV
                          /* | PMU_AHB_CLK_ALWAYS_ON */);

    set_flash_clk(FLASH_CLK_26mHz);

    flash_set_line_mode(FLASH_LINE_2);

    /* Step 1: close RF cb and 52M pll */
    BK3000_XVR_REG_0x09 = 0x2BFFF304;
    /* Step 2: close ABB CB */
    BK3000_A0_CONFIG &= ~(1 << 15);
    /* Step 3: close DCO */
    BK3000_A0_CONFIG |= (1 << 26);
    /* Step 4: ana buck pfm mode */
    BK3000_A1_CONFIG &= ~(1 << 31);

    /* if system working in LDO MODE */
    /* Step 5: dig buck ramp compensation disable */
    /* Step 6: ana buck ramp compensation disable */
    if (env_h->env_cfg.used == 0x01)
    {
        #if(CONFIG_APP_TOOLKIT_5 == 1)
            if (!app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_DIG_BUCK_ENABLE))
            {
                BK3000_A1_CONFIG &= ~(1 << 15);
            }

            if (!app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_ANA_BUCK_ENABLE))
            {
                BK3000_A2_CONFIG &= ~(1 << 7);
            }
        #elif(CHIP_PACKAGE_TSSOP_28 == 1) // LDO MODE
            BK3000_A1_CONFIG &= ~(1 << 15);
            BK3000_A2_CONFIG &= ~(1 << 7);
        #endif
    }
    else
    {
        #if(SYS_CFG_BUCK_ON == 0)    // LDO MODE
            BK3000_A1_CONFIG &= ~(1 << 15);
            BK3000_A2_CONFIG &= ~(1 << 7);
        #endif
    }

    Delay(100);

    for (i = 0; i <= 26; i++)
    {
        gpio_config(i,0);  // High impedance
    }

    SET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHG_FINISHED_WITH_GPIO);
    gpio_config(WAKEUP_GPIO_OUT_CHARGE, 4);

    if (high_flag)
    	gpio_config(wakup_pin, 0);
    else
    	gpio_config(wakup_pin, 3);

    while (get_Charge_state() != BATTERY_CHARGE_FULL_POWER) /* the elimination of GPIO jitter */
    {
        os_delay_ms(20);
        if (gpio_input(WAKEUP_GPIO_OUT_CHARGE) == 1)
        {
            jitter_low_cnt = 0;
            jitter_high_cnt++;
            if (jitter_high_cnt > 3) // if gpio detect is anti-high_flag level more than 3 times, means detect the falling edge,  then enter into deep sleep!
            {
                #if 1
                USB_OPEN_CHARGE; // for reading the charge state register.
                os_delay_ms(10);

                os_printf("break,%d,%d, 0x%x, 0x%x\n", jitter_low_cnt, jitter_high_cnt, USB_IS_PLUG_IN, USB_CHARGE_IS_FULL);
                os_delay_ms(50); //for printf

                if (USB_IS_PLUG_IN && (!USB_CHARGE_IS_FULL))
                {
                    USB_CLOSE_CHARGE;
                    os_delay_ms(10);

                    BK3000_start_wdt(0xFF);
                    while(1);
                }

                USB_CLOSE_CHARGE;
                os_delay_ms(10);
                #endif

                break;
            }
        }
        else
        {
            jitter_high_cnt = 0;
            jitter_low_cnt++;
            if (jitter_low_cnt > 0x0F) // if GPIO is high_flag level for a long time, then reset mcu;
            {
                os_printf("reset wdt,%d,%d\n", jitter_low_cnt, jitter_high_cnt);
                os_delay_ms(50); //for printf

                BK3000_start_wdt(0xFF);
                while(1);
            }
        }
    }

    if (gpio_input(wakup_pin) == high_flag)
    {
        gpio_config(wakup_pin, 5);
        if (22 == wakup_pin)
            gpio_config(7, 5);
    }

    app_set_powerdown_gpio();
    Delay(10);

    BK3000_GPIO_DEEP_SLEEP_LATCH = 0x01;
//BK3000_GPIO_PAD_CTRL |= (0x1F << 3);
    BK3000_GPIO_PAD_CTRL &= (~(0x1F << 3));
    BK3000_GPIO_PAD_CTRL |= (0x1D << 3);

    /*
    * Bit 29: wakeup enable;
    * Bit xx: gpio pin(0:26)wakeup;
    * Bit 27: USB plug in wakeup;
    * Bit 28: RTC wakeup;
    */
    BK3000_GPIO_WKUPEN  = ((1 << 29) | (1 << wakup_pin) | (1 << WAKEUP_GPIO_OUT_CHARGE) | (1 << 27));

    BK3000_GPIO_DPSLP  = 0x3261;

    while(1);
}

#endif
static void BK3000_icu_shutdown( uint8  wakup_pin)
{
    int i = 0;
    app_handle_t app_h = app_get_sys_handler();
	
    os_printf("icu_shutdown:%d\r\n",wakup_pin);
    set_spr( SPR_VICMR(0), 0);
    Delay(10);
    cpu_set_interrupts_enabled(0);
    if(GPIO_CHARGER_FLAG)
    {
        bt_flag2_operate(APP_FLAG2_CHARGE_POWERDOWN,0);
        BK3000_start_wdt(0x1ff);
        while(1);
    }

    for( i = 0; i <= GPIO_NUM; i++)
    {
        gpio_config(i,0);//input & pulldown;
    }

    if(app_h->button_ad_enable) //(ADC_CHANNEL_NULL != app_h->button_ad_channel)
    {//配置ADC按键为高阻状态
        if(ADC_CHANNEL_1 == app_h->button_ad_channel)
        {
            gpio_config(6,5);//input & pulldown;
        }
        else if(ADC_CHANNEL_2 == app_h->button_ad_channel)
        {
            gpio_config(7,5);//High impedance
        }
        else if(ADC_CHANNEL_4 == app_h->button_ad_channel)
        {
            gpio_config(18,5);//High impedance
        }
        else if(ADC_CHANNEL_6 == app_h->button_ad_channel)
        {
            gpio_config(20,5);//High impedance
        }
        else if(ADC_CHANNEL_7 == app_h->button_ad_channel)
        {
            gpio_config(21,5);//High impedance
        }
        else if(ADC_CHANNEL_10 == app_h->button_ad_channel)
        {
            gpio_config(22,5);//High impedance
        }
    }

    gpio_config(wakup_pin, 3); 
    app_set_powerdown_gpio();

    BK3000_A0_CONFIG |= (1<<16);
}
void BK3000_icu_sw_powerdown(uint8  wakup_pin,t_powerdown_mode pwrdown_mode)
{

	#if 0
    app_env_handle_t  env_h = app_env_get_handle();
    uint8 high_flag = (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_PWRCTRL_HIGH) ? 1:0 ;
    switch(pwrdown_mode)
    {
    case POWERDOWN_SELECT:
#if 1/*(CONFIG_APP_TOOLKIT_5 == 1)*/&&(CONFIG_SW_SWITCH_KEY_POWER == 1)
        if((SW_PWR_KEY_SWITCH==app_env_check_pwrCtrl_mode())
            && (((gpio_input(wakup_pin)!=high_flag)&&(DEFAULT_WAKEUP_PIN != wakup_pin))
            || (gpio_input(wakup_pin)==high_flag)))
        {
        		BK3000_icu_deepsleep(wakup_pin,POWERDOWN_SELECT);
        }
        else
#endif
        {
            BK3000_icu_shutdown(wakup_pin);
        }
        break;
#if (CONFIG_CHARGER_DEEPSLEEP == 1)
    case POWERDOWN_CHG_DEEPSLEEP:
        BK3000_icu_deepsleep(wakup_pin,POWERDOWN_CHG_DEEPSLEEP);
        break;
    case POWERDOWN_DEEPSLEEP_WITH_RTC:
        BK3000_deepsleep_with_rtc(wakup_pin, POWERDOWN_DEEPSLEEP_WITH_RTC);
        break;

    case POWERDOWN_DEEPSLEEP_WITH_GPIO:
        BK3000_deepsleep_with_gpio(wakup_pin, POWERDOWN_DEEPSLEEP_WITH_GPIO);
        break;
#endif
    case POWERDOWN_SHUTDOWN:
        BK3000_icu_shutdown(wakup_pin);
        break;
    default:
        BK3000_icu_deepsleep(wakup_pin,POWERDOWN_SELECT);
        break;
    }
	#else
	BK3000_icu_shutdown(wakup_pin);
	#endif
}
void open_linein2aux_loop(void)
{
    return;
}

void close_linein2aux_loop(void)
{
    return;
}

void clear_sco_connection(void)
{
    bt_flag1_operate(APP_FLAG_SCO_CONNECTION, 0);
}
void clear_wave_playing(void)
{
    bt_flag1_operate(APP_FLAG_WAVE_PLAYING, 0);
}
void clear_music_play(void)
{
    bt_flag1_operate(APP_FLAG_MUSIC_PLAY, 0);
}

void clear_line_in(void)
{
    bt_flag1_operate(APP_FLAG_LINEIN, 0);
}

void BK3000_GPIO_Initial (void)
{
    /// UART
    BK3000_GPIO_0_CONFIG  = 1<<sft_GPIO_INPUT_MONITOR;
    BK3000_GPIO_1_CONFIG  = 1<<sft_GPIO_INPUT_MONITOR;
//GPIO配置为高阻态
/*
    BK3000_GPIO_2_CONFIG  = 1<<sft_GPIO_INPUT_MONITOR;
    BK3000_GPIO_3_CONFIG  = 1<<sft_GPIO_INPUT_MONITOR;

    BK3000_GPIO_4_CONFIG  = 1<<sft_GPIO_INPUT_MONITOR;
    BK3000_GPIO_5_CONFIG  = 1<<sft_GPIO_INPUT_MONITOR;
    BK3000_GPIO_6_CONFIG  = 1<<sft_GPIO_INPUT_MONITOR;
    BK3000_GPIO_7_CONFIG  = 1<<sft_GPIO_FUNCTION_ENABLE;
    BK3000_GPIO_8_CONFIG  = 1<<sft_GPIO_FUNCTION_ENABLE;
    BK3000_GPIO_9_CONFIG  = 1<<sft_GPIO_FUNCTION_ENABLE;
    BK3000_GPIO_10_CONFIG = 0x08;
    BK3000_GPIO_11_CONFIG = 0x08;

    BK3000_GPIO_12_CONFIG = 1<<sft_GPIO_FUNCTION_ENABLE;
    BK3000_GPIO_13_CONFIG = 1<<sft_GPIO_FUNCTION_ENABLE;
    BK3000_GPIO_14_CONFIG = 1<<sft_GPIO_FUNCTION_ENABLE;
    BK3000_GPIO_15_CONFIG = 1<<sft_GPIO_INPUT_MONITOR;
    BK3000_GPIO_16_CONFIG = 1<<sft_GPIO_INPUT_MONITOR;
    BK3000_GPIO_17_CONFIG = 1<<sft_GPIO_INPUT_MONITOR;

#ifdef TRACE32_DEBUG
#else
    BK3000_GPIO_18_CONFIG = 0x08;
    BK3000_GPIO_19_CONFIG = 0x08;
    BK3000_GPIO_20_CONFIG = 0x08;
    BK3000_GPIO_21_CONFIG = 0x08;
#endif
    BK3000_GPIO_22_CONFIG = 0x08;
    BK3000_GPIO_23_CONFIG = 1<<sft_GPIO_INPUT_MONITOR;
    BK3000_GPIO_24_CONFIG = 0x08;
    BK3000_GPIO_25_CONFIG = 1<<sft_GPIO_INPUT_MONITOR;
    BK3000_GPIO_26_CONFIG = 1<<sft_GPIO_INPUT_MONITOR;
*/
    //BK3000_GPIO_2_CONFIG  = 0x08;
    //BK3000_GPIO_3_CONFIG  = 0x08;

    //BK3000_GPIO_4_CONFIG  = 0x08;
    //BK3000_GPIO_5_CONFIG  = 0x08;
    //BK3000_GPIO_6_CONFIG  = 0x08;
    BK3000_GPIO_6_CONFIG  = 0x70;   // for 28pin package,shutdown
    BK3000_GPIO_7_CONFIG  = 0x7c;

    BK3000_GPIO_8_CONFIG  = 0x2c;   // for line in
    BK3000_GPIO_9_CONFIG  = 0x2c;   // for line in
    BK3000_GPIO_10_CONFIG = 0x2c;
    BK3000_GPIO_11_CONFIG = 0x2c;

    BK3000_GPIO_12_CONFIG = 0x2c;
    BK3000_GPIO_13_CONFIG = 0x2c;
    BK3000_GPIO_14_CONFIG = 0x2c;
    BK3000_GPIO_15_CONFIG = 0x2c;
    BK3000_GPIO_16_CONFIG = 0x2c;
    BK3000_GPIO_17_CONFIG = 0x2c;

#ifdef TRACE32_DEBUG
#else
    BK3000_GPIO_18_CONFIG = 0x2c;
    BK3000_GPIO_19_CONFIG = 0x2c;
    BK3000_GPIO_20_CONFIG = 0x2c;
    BK3000_GPIO_21_CONFIG = 0x2c;
#endif

    BK3000_GPIO_22_CONFIG = 0x2c;
    BK3000_GPIO_23_CONFIG = 0x2c;
    BK3000_GPIO_24_CONFIG = 0x2c;
    BK3000_GPIO_25_CONFIG = 0x2c;
    BK3000_GPIO_26_CONFIG = 0x2c;

    BK3000_GPIO_GPIODCON = 0;

    //BK3000_GPIO_6_CONFIG  = 0x40;   
    //BK3000_GPIO_7_CONFIG  = 0x40;
    //BK3000_GPIO_18_CONFIG  = 0x40;

    //BK3000_GPIO_GPIODCON = (1<<6) | (1<<7) | (1<<18);
	/// Perial Mode 1 function
    BK3000_GPIO_GPIOCON = 0;
    BK3000_GPIO_PAD_CTRL = 1<<13;
}

uint32_t BK3000_GPIO_MAPPING(uint32_t from, uint32_t index)
{
    uint32_t index2;

    if(from == 0)
    {
        if(index >= 2 && index <= 5)
        {
            index2 = index + 14;
        }
        else if(index>= 6 && index<= 14)
        {
            index2 = index - 4;
        }
        else if(index>= 15 && index<= 17)
        {
            index2 = index + 5;
        }
        else if(index>= 18 && index<= 22)
        {
            index2 = index - 7;
        }
        else
        {
            index2 = index;
        }
    }
    else
    {
        if(index >= 16 && index <= 17)
        {
            index2 = index - 14;
        }
        else if(index>= 2 && index<= 12)
        {
            index2 = index + 4;
        }
        else if(index>= 20 && index<= 22)
        {
            index2 = index - 5;
        }
        else if(index>= 11 && index<= 15)
        {
            index2 = index + 7;
        }
        else
        {
            index2 = index;
        }
    }

    return index2;
}


//RAM_CODE unsigned char BK3000_hfp_set_powercontrol(void)
//{
//    unsigned char set_power = 0;
//    set_power = 0x18;
//    set_power = set_power &0x1f;  
//    return set_power;
//}

void VICMR_enable_intr_src(uint32 mask)
{
    uint32 int_mask;

    int_mask = get_spr(SPR_VICMR(0));
    int_mask |= mask;
    set_spr( SPR_VICMR(0), int_mask);
}

void VICMR_disable_intr_src(uint32 mask)
{
    uint32 int_mask;

    int_mask = get_spr(SPR_VICMR(0));
    int_mask &= (~mask);
    set_spr( SPR_VICMR(0), int_mask);
}

void VICMR_usb_chief_intr_enable(void)
{
    VICMR_enable_intr_src(1<<VIC_USB_ISR_INDEX);
}

void VICMR_usb_chief_intr_disable(void)
{
    VICMR_disable_intr_src(1<<VIC_USB_ISR_INDEX);
}

void ba22_disable_intr_exception(void)
{
    cpu_set_interrupts_enabled(0);
}

void ba22_enable_intr_exception(void)
{
    cpu_set_interrupts_enabled(1);
}

DRAM_CODE void  VICMR_disable_interrupts(uint32 *interrupts_info_ptr, uint32 *mask)
{
    uint32 oldmask = get_spr(SPR_VICMR(0));

    set_spr(SPR_VICMR(0), 0);

    *interrupts_info_ptr = oldmask;
}
DRAM_CODE void VICMR_restore_interrupts(uint32 interrupts_info, uint32 mask)
{

    if(interrupts_info != 0)
    {
        uint32 trg_flags = get_spr(SPR_VICTR(0));
        set_spr(SPR_VICTR(0), trg_flags &(~(1<<VIC_CEVA_ISR_INDEX)));
        set_spr(SPR_VICMR(0), interrupts_info);
    }
    //set_spr( SPR_VICMR(0), interrupts_info);
}

#ifdef	WROK_AROUND_DCACHE_BUG
inline void app_Dcache_disable(void)
{
    //disable dcache
	unsigned int sr ;
	sr = get_spr(SPR_SR);
	sr = sr & (~SPR_SR_DCE);
	set_spr(SPR_SR, sr);

	//gpio_output( 9, 1 );
	//os_printf("Dcache_disable\r\n");
}

inline void app_Dcache_enable(void)
{
	//enable dcache
	unsigned int sr ;
	sr = get_spr(SPR_SR);
	sr = sr | SPR_SR_DCE;
	set_spr(SPR_SR, sr);
	//gpio_output( 9, 0 );
	//os_printf("old\r\n");
}

inline void app_Dcache_initial(void)
{
	unsigned int sr ;

	//disable dcache
	sr = get_spr(SPR_SR);
	sr = sr & (~SPR_SR_DCE);
	set_spr(SPR_SR, sr);

	//initial dcache
	set_spr(SPR_RIR_MIN, 0);
	set_spr(SPR_RIR_MAX, 0xfffffff4);

	//enable dcache
	sr = get_spr(SPR_SR);
	sr = sr | SPR_SR_DCE;
	set_spr(SPR_SR, sr);

	//disable dcache
	sr = get_spr(SPR_SR);
	sr = sr & (~SPR_SR_DCE);
	set_spr(SPR_SR, sr);
}
#endif

#ifdef CONFIG_PRODUCT_TEST_INF
uint8 aver_rssi = 0;
int16 freq_offset = 0,aver_offset=0;

uint8 rssi = 0;
inline void get_freqoffset_rssi(void)
{
     rssi = BK3000_XVR_REG_0x12 & 0xff;
     freq_offset = BK3000_XVR_REG_0x14 & 0x1ff;
}

void average_freqoffset_rssi(void)
{
    static uint16 sum_rssi=0;
    static int16 sum_offset=0;
    static uint8 i = 0;
    int16 temp = freq_offset;
    sum_rssi += rssi;
    temp <<= 7;
    temp >>= 7;
    sum_offset += temp;
    i++;
    if (5 == i)
    {
        aver_rssi=sum_rssi/i;
        aver_offset = sum_offset/i;
        i = 0;
        sum_rssi = 0;
        sum_offset = 0;
    }

}
#else
inline void get_freqoffset_rssi(void)
{

}
#endif
// end of file
