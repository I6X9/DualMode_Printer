#include "driver_beken_includes.h"
#include "beken_external.h"
#include "app_beken_includes.h"

#define	SD_CARD_OFFLINE	                0
#define	SD_CARD_ONLINE                  1
#ifdef CONFIG_BK_QFN56_DEMO
#define	SD_CLK_PIN						12 // 18
#else
#define	SD_CLK_PIN						 18
#endif
#define	SD_CLK_PIN_TIMEOUT1				0x1000
#define	SD_CLK_PIN_TIMEOUT2				0x8000
#define SD_TOTAL_BLOCK_THRESHOLD		(1000000)

#if defined(CONFIG_APP_SDCARD)

static driver_sdcard_t driver_sdcard;
static uint8 SDOnline = SD_CARD_OFFLINE;
static uint8 SD_detect_pin =0;
static uint16 cnt_online = 0;
static uint16 cnt_offline = 0;
static uint16 NoneedInitflag = 0;
static uint8 SD_Insert_flag=0;
static uint8 SD_det_gpio_flag =0;
static uint16 Sd_MMC_flag = 0;

int (*sd_rd_blk_sync_fptr)(int first_block, int block_num, uint8 *dest) = NULL;

#ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
extern u_int8 syspwr_cpu_halt;
#endif

uint8 sd_clk_is_attached(void);
static int driver_sdcard_cmd_start( uint8 cmd_index, int flag, int timeout, void *arg )
{
    uint32 status;
    uint32 timeoutcnt = 0;
    uint32 timeout2cnt = 0;
	
    flag &= 0x07;
    BK3000_SDIO_CMD_SEND_AGUMENT = (uint32)arg;
    BK3000_SDIO_CMD_RSP_TIMER    = timeout;
    BK3000_SDIO_CMD_SEND_CTRL    = ( cmd_index << SD_CMD_INDEX_SHIFT_BIT )
        | ( flag << SD_CMD_FLAG_SHIFT_BIT )| 0x01; // start
    //the Delay below SHOULD NOT be DELETE, otherwise it will result in command error
    Delay(1);

    while(1)
    {
        status = BK3000_SDIO_CMD_RSP_INT_SEL;

        //wait until cmd response
        //�����Ƴ���timeout����˳�??????
        /**************************************************************
         **��ʱϵͳ��ͣ�� "BT is inactive!!!!!!!!!!!!!!!!!!!!!!!!"��������ͣ�������ѭ����;
         **2014/08/06  lianxue.liu  added        |bit_SD_INT_CMD_CRC_FAIL
         ***************************************************************/
        if(status & (bit_SD_INT_NORSP_END
                     | bit_SD_INT_RSP_END
                     | bit_SD_INT_RSP_TIMEOUT))
        {
            break;
        }
        if(!SD_det_gpio_flag)
        {
            if((timeoutcnt++) >= SD_CLK_PIN_TIMEOUT1)//this value needs to be adjusted
            {
                if(SD_CARD_OFFLINE == sd_clk_is_attached())//detect sdcard is valid or not
                {
                    BK3000_SDIO_CMD_RSP_INT_SEL=status;//clear the int flag
                    return SD_ERR_CMD_TIMEOUT;
                }
                else
                {
                    if((timeout2cnt++) >0x30000)
                    {
                        timer_clear_watch_dog();
                        os_printf("===cmd timeout0==:%x,%x\r\n",status,cmd_index);
                        return SD_ERR_CMD_TIMEOUT;
                    }
                    if(0)//(timeout2cnt %0x30000 == 0) // 1S:clr wdt
                    {
                        timer_clear_watch_dog();
                    }
                }
            }
        }
        else
        {
            if(SD_CARD_OFFLINE == sd_is_attached())//detect sdcard is valid or not
            {
                BK3000_SDIO_CMD_RSP_INT_SEL=status;//clear the int flag
                return SD_ERR_CMD_TIMEOUT;
            }
            else
            {
                if((timeout2cnt++) >0x30000)
                {
                    timer_clear_watch_dog();
                    os_printf("===cmd timeout1==:%x,%x\r\n",status,cmd_index);
                    return SD_ERR_CMD_TIMEOUT;
                }
                if(0)//(timeout2cnt %0x30000 == 0) // 1S:clr wdt
                {
                    timer_clear_watch_dog();
                }
            }
        }

    }

    BK3000_SDIO_CMD_RSP_INT_SEL = bits_SD_CMD_RSP;//clear the int flag

    if((status&bit_SD_INT_NORSP_END)||(status&bit_SD_INT_RSP_TIMEOUT))
        return SD_ERR_CMD_TIMEOUT;
    if(status&bit_SD_INT_CMD_CRC_FAIL)
    {
        if((cmd_index!=41)&&(cmd_index!=2)&&(cmd_index!=9)&&(cmd_index!=1))
            return SD_ERR_CMD_CRC_FAIL;
    }
    return SD_ERR_NONE;
}



static void driver_sdcard_recv_data_start(int timeout )
{
    BK3000_SDIO_DATA_REC_TIMER = timeout;
#ifdef CONFIG_APP_SDCARD_4_LINE
    BK3000_SDIO_DATA_REC_CTRL = 0x1|(1 << 2)|(512<< 4)|(1<< 17);
#else
    BK3000_SDIO_DATA_REC_CTRL = 0x1|(512 << 4)|(1<< 17);
#endif
}

/* static int driver_sdcard_write_data_start(int timeout ) */
/* { */
/*     if( driver_sdcard.sd_state != SD_STATE_STANDBY ) */
/*     { */
/*         SD_PRT("wr_data_start_err\r\n"); */
/*         return -1; */
/*     } */

/*     driver_sdcard.sd_state = SD_STATE_READING_DATA; */

/*     BK3000_SDIO_DATA_REC_TIMER = timeout; */
/*     driver_sdcard.data_func( (uint32 *)&BK3000_SDIO_WR_DATA_ADDR, 512); */

/*     #ifdef CONFIG_APP_SDCARD_4_LINE */
/*     BK3000_SDIO_DATA_REC_CTRL = ( 1 << 2 ) */
/*                                 |( driver_sdcard.block_size << 4 ) */
/*                                 |( 1 << 16) */
/*                                 |( 1 << 17); */
/*     #else */
/*     BK3000_SDIO_DATA_REC_CTRL = ( driver_sdcard.block_size << 4 ) */
/*                                 |( 1 << 16) */
/*                                 |( 1 << 17); */
/*     #endif */

/*     return 0; */
/* } */

static int driver_sdcard_data_trans_stop( void )
{
    //BK3000_SDIO_DATA_REC_CTRL = (0x1 << 1);

    return 0;
}

int driver_sdcard_read_write_stop( void )
{
    int ret=0;

    driver_sdcard_data_trans_stop();
 //   ret=driver_sdcard_cmd_start( 12, SD_CMD_SHORT, DEFAULT_CMD_TIME_OUT, (void *)0 );

    return ret;
}



/*����ֵ:0 -- �ޱ仯
		 1 -- SD INSERT
		 2 -- SD PULLOUT
*/
static void sd_detect_fun(void)
{
#ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    app_handle_t app_h = app_get_sys_handler();
#endif
    if(!SD_det_gpio_flag)
    {
        volatile unsigned long *gpioconfig = (volatile unsigned long *)(BK3000_GPIO_BASE_ADDR + (SD_CLK_PIN << 2));
        uint32 detectstopflag = (*gpioconfig) & (1<<6);
        //os_printf("detect 0 :%x,%x\r\n",detectstopflag,gpio_input(SD_detect_pin));
        if(!detectstopflag )//&&(SD_Insert_flag == 0))
        {
            //		os_printf("detect 1 :%x\r\n",gpio_input(SD_detect_pin));
            if(!gpio_input(SD_detect_pin))
            {
                if (cnt_online < SD_DEBOUNCE_COUNT)
                {
                #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
                    if((app_is_not_auto_reconnection() && syspwr_cpu_halt)&&
                        (( app_h->sys_work_mode != SYS_WM_SD_MUSIC_MODE)))
                    {
                        app_exit_sniff_mode();
                    }
                #endif
                    cnt_online ++;
                }
                else
                {
                    if(SD_CARD_OFFLINE == SDOnline)
                    {
                        os_printf("sd insert!!\r\n");
                        SDOnline    = SD_CARD_ONLINE;
                        msg_put(MSG_SD_ATTACH);//������
                        SD_Insert_flag = 1;
                        cnt_offline = 0;
                    #if defined(CONFIG_LINE_SD_SNIFF)
                        bt_flag2_operate(APP_FLAG2_LINEIN_SD_PLAYING,1);
                    #endif
                    }
                }
            }
            else
            {
                 if (cnt_offline < SD_DETACH_COUNT)
                {
                    #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
                        if((app_is_not_auto_reconnection() && syspwr_cpu_halt))
                        {
                            app_exit_sniff_mode();
                        }	
                    #endif
                    cnt_offline ++;
                }
                else
                {
                    //	os_printf("detect 1 :\r\n");
                    if(SD_CARD_ONLINE == SDOnline)
                    {
                    #if 0//def CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
                        if((app_is_not_auto_reconnection() && syspwr_cpu_halt))
                        {
                            app_exit_sniff_mode();
                        }	
                    #endif
                        os_printf("sd desert!!\r\n");
                        cnt_online = 0;
                        SDOnline   = SD_CARD_OFFLINE;
                        SD_Insert_flag = 1;
                        driver_sdcard.init_flag = 1;
                        NoneedInitflag = 0;
                        msg_put(MSG_SD_DETACH);//���Ƴ�
                    }
                }
            }
        }
        //	gpio_output(22,0);
    }
    else
    {
        if(gpio_input(SD_detect_pin))
        {
            if (cnt_offline < SD_DETACH_COUNT)
            {
                #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
                    if((app_is_not_auto_reconnection() && syspwr_cpu_halt))
                    {
                        app_exit_sniff_mode();
                    }	
                #endif
                cnt_offline ++;
            }
            else
            {
                if(SD_CARD_ONLINE == SDOnline)
                {
                #if 0//def CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
                    if((app_is_not_auto_reconnection() && syspwr_cpu_halt))
                    {
                        app_exit_sniff_mode();
                    }	
                #endif
                    cnt_online = 0;
                    SDOnline   = SD_CARD_OFFLINE;
                    SD_Insert_flag = 0;
                    driver_sdcard.init_flag = 1;
                    NoneedInitflag = 0;
                    msg_put(MSG_SD_DETACH);//���Ƴ�
                }
            }
        }
        else
        {
            if (cnt_online < SD_DEBOUNCE_COUNT)
            {
            #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
                if((app_is_not_auto_reconnection() && syspwr_cpu_halt)&&
                        (( app_h->sys_work_mode != SYS_WM_SD_MUSIC_MODE)))
                {
                    app_exit_sniff_mode();
                }
            #endif
                cnt_online ++;
            }
            else
            {
                if(SD_CARD_OFFLINE == SDOnline)
                {
                    SDOnline    = SD_CARD_ONLINE;
                    SD_Insert_flag = 1;
                    cnt_offline = 0;
                    msg_put(MSG_SD_ATTACH);//������
                }
            }
        }
    }
}
void SD_HW_Init(void)
{
    uint32 status,oldmask;

    status = BK3000_SDIO_CMD_RSP_INT_SEL;
    BK3000_SDIO_CMD_RSP_INT_SEL = status;

    /*Clear tx/rx fifo*/
    BK3000_SDIO_FIFO_THRESHOLD |= SDIO_RX_FIFO_RST_BIT | SDIO_TX_FIFO_RST_BIT;
#ifdef CONFIG_BK_QFN56_DEMO
    gpio_enable_second_function(GPIO_FUNCTION_SDIO);
#else
    BK3000_GPIO_18_CONFIG = 0x70; // 2nd function
    BK3000_GPIO_19_CONFIG = 0x70; // 2nd function
    BK3000_GPIO_20_CONFIG = 0x70; // 2nd function
    (*(volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+0x1f*4)) |= (0x1<<18)|(0x1<<19)|(0x1<<20); //gpio_pconf4
    (*(volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+0x2d*4)) |= (0x1<<12);  //bit[12]:SDIO GPIO sel 0:GPIO 12,13,14      1:GPIO 18,19,20      
#endif   
    /*Disabe all sdio interrupt*/
    BK3000_SDIO_CMD_RSP_INT_MASK = 0;//0x3F;

    /*Config tx/rx fifo threshold*/
    BK3000_SDIO_FIFO_THRESHOLD   = 0x101;// 16 byte to read

    //������SDIO�ж�
    oldmask = get_spr(SPR_VICMR(0));
    oldmask &= (~(1<<VIC_SDIO_ISR_INDEX));
    set_spr(SPR_VICMR(0),oldmask);
    os_printf("SD_HW_Init\r\n");
}

/*
  0 -- 13M
  1 -- 6.5M
  2 -- 3.25M
  3 -- 1.625M
*/
void Set_Sd_Clk(uint8 clkdiv)
{
    Delay(10);
    /*Config sdio clock*/
    uint32 tmp =BK3000_PMU_PERI_PWDS;
    tmp&=~(SDIO_CLK_SEL_BIT0 | SDIO_CLK_SEL_BIT1);
    tmp|=clkdiv<<16;
    tmp&=~(1<<24);
    BK3000_PMU_PERI_PWDS = tmp;
    //os_printf("Set_Sd_Clk\r\n");
}

/*reset card to idle state*/
int Send_Card_CMD0(void)
{
    return driver_sdcard_cmd_start( 0, SD_CMD_FLAG_CRC_CHECK, DEFAULT_CMD_TIME_OUT, (void *)0 );
}

int Send_Card_CMD1(void)
{
    int Ret;
    uint32 response;
	
cmd1_loop:
    Ret = driver_sdcard_cmd_start( 1, SD_CMD_SHORT, 0x900, (void *)0x40ff8000 );
    if(Ret == SD_ERR_NONE)
    {
        response = BK3000_SDIO_CMD_RSP_AGUMENT0;
        if(!(response & OCR_MSK_VOLTAGE_ALL))
            Ret = SD_ERR_CMD41_CNT;
        if(!(response & OCR_MSK_BUSY))
            goto cmd1_loop;
        if(response & OCR_MSK_HC)
            driver_sdcard.Addr_shift_bit = 0;
        else
            driver_sdcard.Addr_shift_bit = 9;//��λ	
    }
    BK3000_SDIO_FIFO_THRESHOLD |= 20;
    delay_us(10);
    return Ret;
}
/**/
static int wait_Receive_Data(void);
int Send_MMCCard_CMD8(void)
{
    int Ret,i;
    uint32 tmp;
    uint8 *tmpptr =(uint8*)jmalloc(512,0);
    if(tmpptr == NULL)
        return 1;
    Ret	= driver_sdcard_cmd_start( 8, SD_CMD_SHORT, 0x90000/*DEFAULT_CMD_TIME_OUT*/,(void *)0);
    if (Ret != SD_ERR_NONE) 
        goto freebuf;
    BK3000_SDIO_FIFO_THRESHOLD = (1 << 20); // reset first
    BK3000_SDIO_FIFO_THRESHOLD = 0x3ffff; // set fifo later
    driver_sdcard_recv_data_start(DEFAULT_DATA_TIME_OUT);
		
    tmp = 0;
    Ret = wait_Receive_Data();
    if(Ret == SD_ERR_NONE)
    {
        for (i = 0; i < 128; i++)
        {
		while(!(BK3000_SDIO_FIFO_THRESHOLD&(0x1<<18)))
		{
			tmp++;
			if(tmp > 0x20)
				break;
		}
			
       	*((uint32 *)tmpptr + i) = BK3000_SDIO_RD_DATA_ADDR;
        }
        driver_sdcard.total_block = tmpptr[212] |(tmpptr[213]<<8)|(tmpptr[214]<<16)|(tmpptr[215]<<24);
    }
	
freebuf:
	jfree(tmpptr);
	return Ret;
}

/*Send SD card interface condition*/
int Send_Card_CMD8(void)
{
    return driver_sdcard_cmd_start( 8, SD_CMD_SHORT, 100000/*DEFAULT_CMD_TIME_OUT*/, (void *)0x000001AA );

}

/*ask the CID number on the CMD line*/
int Send_Card_CMD2(void)
{
    return driver_sdcard_cmd_start( 2, SD_CMD_LONG, DEFAULT_CMD_TIME_OUT, (void  *)0 );
}

int Send_MMCCard_CMD3(void)
{
	int Ret;
	driver_sdcard.card_rca = 1;
	Ret=driver_sdcard_cmd_start(3, SD_CMD_SHORT, DEFAULT_CMD_TIME_OUT, (void  *)(driver_sdcard.card_rca<<16) );
	return Ret;
}
/*ask the card to publish a new RCA*/
int Send_Card_CMD3(void)
{
    int Ret;
    Ret=driver_sdcard_cmd_start(3, SD_CMD_SHORT, DEFAULT_CMD_TIME_OUT, (void  *)0 );
    if(Ret==SD_ERR_NONE)
        driver_sdcard.card_rca = ( BK3000_SDIO_CMD_RSP_AGUMENT0 >> 16 );

    return Ret;
}

#define SD_CARD 0
#define MMC_CARD 1
/*get CSD Register content*/
int Send_Card_CMD9(uint8 SD_MMC)
{
    int Ret,mult, csize;

    Ret=driver_sdcard_cmd_start( 9, SD_CMD_LONG, DEFAULT_CMD_TIME_OUT, (void  *)(driver_sdcard.card_rca << 16) );

    if(Ret==SD_ERR_NONE)
    {
        Delay(2);
        driver_sdcard.block_size = (1 << (( BK3000_SDIO_CMD_RSP_AGUMENT1 >> 16 ) & 0xF));


        if(SD_MMC == SD_CARD)
        {
            if( ((BK3000_SDIO_CMD_RSP_AGUMENT0 >> 30) & 0x3) == 0x00)// v1.0
            {
                csize = (((BK3000_SDIO_CMD_RSP_AGUMENT1 & 0x3FF ) << 2)
                         | ((BK3000_SDIO_CMD_RSP_AGUMENT2 >> 30 ) & 0x3));
                mult = ( BK3000_SDIO_CMD_RSP_AGUMENT2 >> 15 ) & 0x7;

                driver_sdcard.total_block = (csize + 1 )*( 1<< (mult + 2 ) );
                driver_sdcard.total_block *= (driver_sdcard.block_size >> 9);
            }
            else // v2.0
            {
                csize = (((BK3000_SDIO_CMD_RSP_AGUMENT1 & 0x3F ) << 16)
                         | ((BK3000_SDIO_CMD_RSP_AGUMENT2 >> 16 ) & 0xFFFF));

                driver_sdcard.total_block = ( csize + 1 )*1024;
            }
        }
		
        else
        {
            if(driver_sdcard.Addr_shift_bit != 0)
            {
                csize = (((BK3000_SDIO_CMD_RSP_AGUMENT1 & 0x3FF ) << 2)
                    | ((BK3000_SDIO_CMD_RSP_AGUMENT2 >> 30 ) & 0x3));
                mult = ( BK3000_SDIO_CMD_RSP_AGUMENT2 >> 15 ) & 0x7;

                driver_sdcard.total_block = (csize + 1 )*( 1<< (mult + 2 ) );
                driver_sdcard.total_block *= (driver_sdcard.block_size >> 9);
            }
            else
                driver_sdcard.total_block = 0;
	 }
		
        driver_sdcard.block_size   = 0x200;
        os_printf("Bsize:%x;Total_block:%x\r\n", driver_sdcard.block_size, driver_sdcard.total_block);
    }
    return Ret;
}

/*select/deselect card*/
int Send_Card_CMD7(void)
{
    driver_sdcard_cmd_start( 7, SD_CMD_SHORT, DEFAULT_CMD_TIME_OUT, (void  *)(driver_sdcard.card_rca << 16) );
    return 0;
}

int Send_Card_Acmd(uint8 cmd_index,uint16 RCA,void* arg)
{
    int Ret;
    Ret=driver_sdcard_cmd_start( 55, SD_CMD_SHORT, 0x90000/*DEFAULT_CMD_TIME_OUT*/, (void *)(RCA<<16) );
    if(Ret!=SD_ERR_NONE)
        return Ret;//error
    Delay(3);
    Ret=driver_sdcard_cmd_start( cmd_index, SD_CMD_SHORT, 0x90000/*DEFAULT_CMD_TIME_OUT*/, (void *)arg);
    return Ret;
}

/*set bus width*/
int Send_Card_Acmd6(void)
{
    int Ret;
#ifdef CONFIG_APP_SDCARD_4_LINE
    Ret=Send_Card_Acmd(6,driver_sdcard.card_rca,(void*)2);
#else
    Ret=Send_Card_Acmd(6,driver_sdcard.card_rca,(void*)0);
#endif
    return Ret;
}

/*Send host capacity support information(HCS) and  asks
  the card to send its OCR in the response on CMD line*/
int Send_Card_Acmd41(void)
{
    #define RETRY_CNT 10

    int Ret,response;
    volatile int try_num=0;
    do{
        Ret=Send_Card_Acmd(41,0,(void *)SD_DEFAULT_OCR);
        try_num++;
        if(Ret==SD_ERR_NONE)
        {
	     Delay(3);
            response = BK3000_SDIO_CMD_RSP_AGUMENT0;
            if(response & OCR_MSK_BUSY)
            {
                if( response & OCR_MSK_HC )
                    driver_sdcard.Addr_shift_bit = 0;
                else
                    driver_sdcard.Addr_shift_bit = 9;//��λ

                break;//card  ready
            }
	     else
	     {
	         Ret = SD_ERR_CMD41_CNT;
	     }
        }
        else
            break;//cmd error

	Delay(4500);
    }while(try_num < RETRY_CNT);

    return Ret;
}
//extern uint8 test_timeout_print(void);
static int wait_Receive_Data(void)
{
    uint32 ret = SD_ERR_LONG_TIME_NO_RESPONS, status=0;
    uint32 timeoutcnt = 0;
    enable_timeout_timer(0);	
    while (1) 
    {
        if(timeout_handle())
        {
            ret = SD_ERR_LONG_TIME_NO_RESPONS;
            break;
        }

        status = BK3000_SDIO_CMD_RSP_INT_SEL;
        if (status & bit_SD_INT_DATA_REC_END) 
	 {
            if (status & bit_SD_INT_DATA_CRC_FAIL) 
            {
		  ret = SD_ERR_DATA_CRC_FAIL;
                //ret = SD_ERR_NONE;
            } 
            else
            {
                ret = SD_ERR_NONE;
            }
            break;
        } 
        else if (status & bit_SD_INT_DATA_CRC_FAIL) 
        {
            ret = SD_ERR_DATA_CRC_FAIL;
            break;
        } 
	 else if (status & bit_SD_INT_TIME_OUT_INT) 
        {
            ret = SD_ERR_DATA_TIMEOUT;
            break;
        }
        if(!SD_det_gpio_flag)
        {
            if((timeoutcnt++) >= SD_CLK_PIN_TIMEOUT2)
            {
                if (0 == sd_clk_is_attached()) 
                {
                    ret = SD_ERR_DATA_TIMEOUT;
                    break;
                }
            }
        }
        else
        {
            if(0 == sd_is_attached()) 
            { //detect sdcard is valid or not
                ret = SD_ERR_DATA_TIMEOUT;
                break;
            }
        }
    }
    disable_timeout_timer();
	
    if((!SD_det_gpio_flag)&&(SD_ERR_LONG_TIME_NO_RESPONS == ret))
        sd_clk_is_attached();
	
    BK3000_SDIO_CMD_RSP_INT_SEL = bits_SD_DATA_RSP;/*< clear the int flag */
    return ret;
}
/* read Multi-block    block:SD ��ÿ��Ϊ��̶���512Byte,ÿ���鴫��ĺ��涼����һ��CRC У��*/
static int Send_Card_cmd18(uint32 addr) // ���ڶ����Ĵ���,ֱ���յ�һ��CMD12 ����.
{
    return driver_sdcard_cmd_start( 18, SD_CMD_SHORT,0x90000 /*DEFAULT_CMD_TIME_OUT*/,(void *)addr);
}

static int Send_Card_cmd12(uint32 addr) // ���ж����������CMD12 ����ֹͣ,֮�󿨽���Transfer State
{
    return driver_sdcard_cmd_start( 12, SD_CMD_SHORT, DEFAULT_CMD_TIME_OUT,(void *)addr);
}

/* read single block */
static int Send_Card_cmd17(uint32 addr)  //���ڴ��䵥����,������֮��,������Transfer State
{
    return driver_sdcard_cmd_start( 17, SD_CMD_SHORT, 0x90000/*DEFAULT_CMD_TIME_OUT*/,(void *)addr);
}
static int Send_Card_cmd24(uint32 addr)
{
    return driver_sdcard_cmd_start( 24, SD_CMD_SHORT, 0x90000/*DEFAULT_CMD_TIME_OUT*/,(void *)addr);
}

static int Send_Card_cmd25(uint32 addr)
{
    return driver_sdcard_cmd_start( 25, SD_CMD_SHORT, 0x900000/*DEFAULT_CMD_TIME_OUT*/,(void *)addr);
}
int sd_rd_blk_sync_old( int first_block, int block_num, uint8 *dest )
{
    int Ret=SD_ERR_NONE, i;
    uint32 cur_block=first_block;
    uint32 tmp = 0;
    static uint32 CRC_Err_cnt=0;
    uint32 crc_error_flag=0;
    uint32 tmp_data = 0;

    if(( block_num == 0 ) || ( dest == NULL ))
        return -1;
    if(!SD_det_gpio_flag)
        gpio_config(SD_CLK_PIN,2);
    CLK_ENABLE;
    while (block_num--) 
    {
        while (BK3000_SDIO_CMD_RSP_INT_SEL&0x00800000)
        {
            if(!SD_det_gpio_flag)
            {
                if(0 == sd_clk_is_attached())  
                {
                    Ret = SD_ERR_CMD_TIMEOUT;
                }
            }
            else
            {				
                if((tmp++) == 0x3000)//this value needs to be adjusted
                {
                    if(0 == sd_is_attached()) 
                    {
                        Ret = SD_ERR_CMD_TIMEOUT;
                    }
                }
            }
        }
        if(Ret != SD_ERR_NONE)
            break;

    #ifdef CONFIG_SDCARD_BY_MULTIBLOCK_WAY
        if((Sd_MMC_flag == SD_CARD)&&(driver_sdcard.total_block > 0x100000))
            Ret = Send_Card_cmd18(cur_block<<driver_sdcard.Addr_shift_bit); // start
        else
            Ret = Send_Card_cmd17(cur_block<<driver_sdcard.Addr_shift_bit); 
    #else
        Ret = Send_Card_cmd17(cur_block<<driver_sdcard.Addr_shift_bit);
    #endif
        if (Ret != SD_ERR_NONE) 
            break;
        BK3000_SDIO_FIFO_THRESHOLD = (1 << 20); // reset first
        BK3000_SDIO_FIFO_THRESHOLD = 0x3ffff; // set fifo later
        driver_sdcard_recv_data_start(DEFAULT_DATA_TIME_OUT);

        tmp = 0;
        Ret = wait_Receive_Data();
        if((Ret==SD_ERR_NONE) || (Ret==SD_ERR_DATA_CRC_FAIL))
        {
            for (i = 0; i < driver_sdcard.block_size; i += 4)
            {
                while(!(BK3000_SDIO_FIFO_THRESHOLD&(0x1<<18)))
                {
                    if(!SD_det_gpio_flag)
                    {
                        if(0 == sd_clk_is_attached())  
                        {
                            Ret = SD_ERR_CMD_TIMEOUT;
                            break;
                        }
                    }
                    else
                    {				
                        if((tmp++) == 0x3000)//this value needs to be adjusted
                        {
                            if(0 == sd_is_attached()) 
                            {
                                Ret = SD_ERR_CMD_TIMEOUT;
                                break;
                            }
                        }
                    }
                }
				tmp_data = BK3000_SDIO_RD_DATA_ADDR;			
				*(dest + i) = tmp_data & 0xFF;			  
				*(dest + (i+1)) = (tmp_data >>	8) & 0xFF;			  
				*(dest + (i+2)) = (tmp_data >> 16) & 0xFF;			 
				*(dest + (i+3)) = (tmp_data >> 24) & 0xFF;
            }
            dest += 512;
            cur_block++;
        }
        else
            break;

        if(Ret == SD_ERR_NONE)
        {
            CRC_Err_cnt = 0;
        }
        else
        {
            if(Ret == SD_ERR_DATA_CRC_FAIL)
            {
                os_printf("SD_ERR_DATA_CRC_FAIL\r\n");
                crc_error_flag++;
                if((CRC_Err_cnt++) < 3)
                    Ret = SD_ERR_NONE;
            }
        }
    #ifdef CONFIG_SDCARD_BY_MULTIBLOCK_WAY
        if((Sd_MMC_flag == SD_CARD) && (driver_sdcard.total_block > 0x100000))
        {
            Ret = Send_Card_cmd12(DEFAULT_DATA_TIME_OUT);  //cmd18 stop
            if (Ret != SD_ERR_NONE) 
                break;
        }
    #endif
    }
    CLK_DISABLE;
    if(!SD_det_gpio_flag)
        gpio_config(SD_CLK_PIN,3);
	
    if((!SD_det_gpio_flag)&&(Ret != SD_ERR_NONE))
        sd_clk_is_attached();

    if(crc_error_flag)
    {
        Ret = SD_ERR_DATA_CRC_FAIL;
    }
    if(Ret !=SD_ERR_NONE)
        os_printf("SD Ret:%d\r\n",Ret);

    return Ret;
}

static int last_read_block = -1;
static int last_start_flag = 0;
int sd_rd_blk_sync_new( int first_block, int block_num, uint8 *dest )
{
    int Ret=SD_ERR_NONE, i;
#ifndef CONFIG_SDCARD_BY_MULTIBLOCK_WAY    
    int tmp = 0;
    uint32 cur_block=first_block;
#endif
    uint32 tmp_data = 0;
	uint32 reg = 0;
	int size = 0;

    if(( block_num == 0 ) || ( dest == NULL )) 
        return -1;

    if(!SD_det_gpio_flag) 
        gpio_config(SD_CLK_PIN,2);

    //os_printf("sd_rd_blk_sync_new:%d,%d)\n",first_block, block_num);

    CLK_ENABLE;
    if(last_read_block != first_block)
    {
        if(last_start_flag)
        {
        #ifdef CONFIG_SDCARD_BY_MULTIBLOCK_WAY
            if((Sd_MMC_flag == SD_CARD) && (driver_sdcard.total_block > 0x100000))
            {
                BK3000_SDIO_FIFO_THRESHOLD  = 0x101 | (SDIO_RX_FIFO_RST_BIT | (1 << 20));
                Ret = Send_Card_cmd12(DEFAULT_DATA_TIME_OUT);
            }
        #endif

            last_start_flag = 0;
        }

        BK3000_SDIO_DATA_REC_TIMER  = DEFAULT_DATA_TIME_OUT * block_num;
        BK3000_SDIO_CMD_RSP_INT_SEL = 0xFFFFFFFF;
        BK3000_SDIO_FIFO_THRESHOLD  = 0x101 | (SDIO_RX_FIFO_RST_BIT | (1 << 20));

    #ifdef CONFIG_APP_SDCARD_4_LINE
        BK3000_SDIO_DATA_REC_CTRL = 0x1|(1 << 2)|(1 << 3)|(512 << 4)|(1<< 17);
    #else
        BK3000_SDIO_DATA_REC_CTRL = 0x1|(0 << 2)|(1 << 3)|(512 << 4)|(1<< 17);
    #endif

    #ifdef CONFIG_SDCARD_BY_MULTIBLOCK_WAY
        if((SD_CARD == Sd_MMC_flag)&&(driver_sdcard.total_block > 0x100000))
            Ret = Send_Card_cmd18(first_block<<driver_sdcard.Addr_shift_bit); // start
        else
            Ret = Send_Card_cmd17(first_block<<driver_sdcard.Addr_shift_bit);
    #else
        Ret = Send_Card_cmd17(cur_block<<driver_sdcard.Addr_shift_bit);
    #endif

        last_start_flag = 1;
    }
    last_read_block = first_block + block_num;   

    if(Ret == SD_ERR_NONE)
    {
        size = driver_sdcard.block_size * block_num;
        //reveive data
        i = 0;
        while(1)
        {
            if(BK3000_SDIO_FIFO_THRESHOLD & (0x1<<18))
            {
				tmp_data = BK3000_SDIO_RD_DATA_ADDR;			
				*(dest + (i++)) = tmp_data & 0xFF;			  
				*(dest + (i++)) = (tmp_data >>	8) & 0xFF;			  
				*(dest + (i++)) = (tmp_data >> 16) & 0xFF;			 
				*(dest + (i++)) = (tmp_data >> 24) & 0xFF;
                if(i >= size)
                {
                    break;
                }
            }
            else
            {
                reg = BK3000_SDIO_CMD_RSP_INT_SEL;

                if(reg & (1 << 5))
                {
                    Ret = SD_ERR_DATA_TIMEOUT;
                    break;
                }
                if(reg & (1 << 13))
                {
                    Ret = SD_ERR_DATA_CRC_FAIL;
                    break;
                }
            }
        }

        while(SD_ERR_NONE == Ret)
        {
            reg = BK3000_SDIO_CMD_RSP_INT_SEL;

            if(reg & (1 << 5))
            {
                Ret = SD_ERR_DATA_TIMEOUT;
                break;
            }
            if(reg & (1 << 13))
            {
                Ret = SD_ERR_DATA_CRC_FAIL;
                break;
            }
            if(reg & (1 << 3 ))
            {
                break;
            }
        }
    }
   
    //CLK_DISABLE;
    //if(!SD_det_gpio_flag) gpio_config(SD_CLK_PIN,3);
    //if((!SD_det_gpio_flag)&&(Ret != SD_ERR_NONE)) sd_clk_is_attached();
    
    if(Ret !=SD_ERR_NONE)
    {
        if(Ret == SD_ERR_DATA_CRC_FAIL)
            os_printf("SD_ERR_DATA_CRC_FAIL\r\n");
        os_printf("SD Ret:%d\r\n",Ret);
    }
    return Ret;
}

int sd_rd_blk_sync( int first_block, int block_num, uint8 *dest )
{
    return sd_rd_blk_sync_fptr(first_block, block_num, dest);
}
#if 1//def CONFIG_APP_USB_CARD_READER
int sd_wr_sblk_sync(int first_block, uint8_t* data)
{
    int  i, res;

    unsigned long reg;

    //bk_printf("[SDIO]: %s(%d, %d, %p)\n", __func__, first_block, block_num, data);

    if(!SD_det_gpio_flag) 
        gpio_config(SD_CLK_PIN,2);

    CLK_ENABLE;

    BK3000_SDIO_CMD_RSP_INT_SEL = 0xFFFFFFFF;
    BK3000_SDIO_FIFO_THRESHOLD  = (1 << 20);
    BK3000_SDIO_FIFO_THRESHOLD  = 0x0101 | SDIO_TX_FIFO_RST_BIT;

    if(SD_ERR_NONE == (res = Send_Card_cmd24(first_block << driver_sdcard.Addr_shift_bit)))
    {
        //Pre fill data
        i = 0;
        while(BK3000_SDIO_FIFO_THRESHOLD & (1 << 19))
        {
            BK3000_SDIO_WR_DATA_ADDR = (data[i + 0] << 24) | (data[i + 1] << 16) | (data[i + 2] << 8) | (data[i + 3]);
            i += 4;
            if(i >= 512)
            {
                break;
            }
        }

        BK3000_SDIO_DATA_REC_TIMER = DEFAULT_DATA_TIME_OUT;
        BK3000_SDIO_DATA_REC_CTRL  = (1 << 16) | (0 << 3) | (512 << 4) | (1 << 17);

        while((BK3000_SDIO_CMD_RSP_INT_SEL & (1 << 7)) == 0);
        while(BK3000_SDIO_CMD_RSP_INT_SEL & 0x00800000);//BUSY

        while(1)
        {
            reg = BK3000_SDIO_CMD_RSP_INT_SEL;

            if(reg & (1 << 5))
            {
                res = SD_ERR_DATA_TIMEOUT;
                break;
            }
            if(reg & (1 << 13))
            {
                res = SD_ERR_DATA_CRC_FAIL;
                break;
            }
            if(reg & (1 << 4))
            {
                break;
            }
        }
    }

    CLK_DISABLE;

    if(!SD_det_gpio_flag) 
        gpio_config(SD_CLK_PIN,3);

    return res;
}


static int sdcard_write_data(uint8_t *writebuff, uint32_t block)
{
    uint32_t i, j, reg, tmpval;
//os_printf("---0000-----\r\n");
    i = 0;
    // 1. fill the first block to fifo and start write data enable
    while( BK3000_SDIO_FIFO_THRESHOLD & (1 << 19) )
    {
        BK3000_SDIO_WR_DATA_ADDR = (writebuff[i]<<24)|(writebuff[i+1]<< 16)|(writebuff[i+2]<<8)|writebuff[i+3];
        i += 4;
        if(512 <= i)
        {
            break;
        }
    }

    BK3000_SDIO_CMD_RSP_INT_MASK |= 1 << 9;
    BK3000_SDIO_DATA_REC_TIMER    = 900000 * block;
    BK3000_SDIO_DATA_REC_CTRL     = (1 << 16) | (1 << 3) | (512 << 4) | (1 << 17);

    do
    {
    	reg = BK3000_SDIO_CMD_RSP_INT_SEL;
    }while(!(reg&(1<<7)));

    // 2. write other blocks
    while(--block)
    {
        j = 0;
        while(j < 512)
        {
        	if(BK3000_SDIO_FIFO_THRESHOLD &(1<<19))
            {
                BK3000_SDIO_WR_DATA_ADDR = (writebuff[i]<<24)|(writebuff[i+1]<<16)|(writebuff[i+2]<<8)|writebuff[i+3];
                i += 4;
                j += 4;
            }
        }

        do
	    {
	    	reg = BK3000_SDIO_CMD_RSP_INT_SEL;
	    }while(!(reg&(1<<4)));
		BK3000_SDIO_CMD_RSP_INT_SEL = 1<<4;
		
	    do
	    {
	    	reg = BK3000_SDIO_CMD_RSP_INT_SEL;
	    }while(!(reg&(1<<7)));

        if(2 != ((reg & (0x7<<20))>>20))
        {
            return SD_ERR_DATA_CRC_FAIL;
        }
    }	

    // 3. after the last block,write zero
    while(1)
    {
        reg = BK3000_SDIO_FIFO_THRESHOLD;
        if(reg & (1<<19))
        {
        	BK3000_SDIO_WR_DATA_ADDR = 0;
            break;
        }
    }
//os_printf("---3333:%x-----\r\n",reg);
    // 4.wait and clear flag
    do
    {
        reg = BK3000_SDIO_CMD_RSP_INT_SEL;
    }while(!(reg&(1<<4)));
	BK3000_SDIO_CMD_RSP_INT_SEL = (1 << 4);
//os_printf("---4444:%x-----\r\n",reg);

    if(2 != ((reg & (0x7<<20))>>20))
    {
        return SD_ERR_DATA_CRC_FAIL;
    }
	
    return SD_ERR_NONE;
}


int sd_wr_mblk_sync(int first_block, int block_num, uint8_t* data)
{
    int  i, res, size = block_num * 512;

    unsigned long reg;

    //bk_printf("[SDIO]: %s(%d, %d, %p)\n", __func__, first_block, block_num, data);

    if(!SD_det_gpio_flag) 
        gpio_config(SD_CLK_PIN,2);

    CLK_ENABLE;

    BK3000_SDIO_CMD_RSP_INT_SEL = 0xFFFFFFFF;

    reg = BK3000_SDIO_FIFO_THRESHOLD;
    BK3000_SDIO_FIFO_THRESHOLD = reg |(1 << 20);

    reg &= (0xffff|(0x3<<21));
    reg |= (0x0101 | (1<<17));
    BK3000_SDIO_FIFO_THRESHOLD = reg;
	
  //  BK3000_SDIO_FIFO_THRESHOLD  = (1 << 20);
  //  BK3000_SDIO_FIFO_THRESHOLD  = 0x0801 | SDIO_TX_FIFO_RST_BIT;

    if(SD_ERR_NONE == (res = Send_Card_cmd25(first_block << driver_sdcard.Addr_shift_bit)))
    {
        //Pre fill data
        res = sdcard_write_data(data,block_num);
        //os_printf("write data:%x\r\n",res);
    }
   else
   {
        os_printf("write 25 cmd error:%x\r\n",res);
   }
    #if 0
	    i = 0;
        while(BK3000_SDIO_FIFO_THRESHOLD & (1 << 19))
        {
            BK3000_SDIO_WR_DATA_ADDR = (data[i + 0] << 24) | (data[i + 1] << 16) | (data[i + 2] << 8) | (data[i + 3]);
            i += 4;
            if(i >= 512)
            {
                break;
            }
        }

        BK3000_SDIO_CMD_RSP_INT_MASK |= 1 << 9;
        BK3000_SDIO_DATA_REC_TIMER    = DEFAULT_DATA_TIME_OUT * block_num;
        BK3000_SDIO_DATA_REC_CTRL     = (1 << 16) | (1 << 3) | (512 << 4) | (1 << 17);

        //send data
        while(i < size)
        {
            if(BK3000_SDIO_FIFO_THRESHOLD & (1 << 19))
            {
                BK3000_SDIO_WR_DATA_ADDR = (data[i + 0] << 24) | (data[i + 1] << 16) | (data[i + 2] << 8) | (data[i + 3]);
                i += 4;
            }
            else
            {
                reg = BK3000_SDIO_CMD_RSP_INT_SEL;

                if(reg & (1 << 4))
                {
                    BK3000_SDIO_CMD_RSP_INT_SEL |= 1 << 4;
                }
                if(reg & (1 << 5))
                {
                    res = SD_ERR_DATA_TIMEOUT;
                    break;
                }
                if(reg & (1 << 13))
                {
                    res = SD_ERR_DATA_CRC_FAIL;
                    break;
                }
            }
        }
    }

    if(SD_ERR_NONE == res)
    {
        while((BK3000_SDIO_CMD_RSP_INT_SEL & (1 << 7)) == 0);
        while(BK3000_SDIO_CMD_RSP_INT_SEL & 0x00800000);//BUSY

        int count = 16;
        while(count && (BK3000_SDIO_FIFO_THRESHOLD & (1 << 19)))
        {
            BK3000_SDIO_WR_DATA_ADDR = 0;
            count--;
        }

        while(1)
        {
            reg = BK3000_SDIO_CMD_RSP_INT_SEL;

            if(reg & (1 << 5))
            {
                res = SD_ERR_DATA_TIMEOUT;
                break;
            }
            if(reg & (1 << 13))
            {
                res = SD_ERR_DATA_CRC_FAIL;
                break;
            }
            if(reg & (1 << 4))
            {
                break;
            }
        }

        while((BK3000_SDIO_CMD_RSP_INT_SEL & (1 << 7)) == 0);
    }
#endif
    BK3000_SDIO_DATA_REC_CTRL    |= (1 << 1);
    BK3000_SDIO_CMD_RSP_INT_MASK &= ~(1 << 9);

    res += Send_Card_cmd12(DEFAULT_DATA_TIME_OUT);

    CLK_DISABLE;

    if(!SD_det_gpio_flag) 
        gpio_config(SD_CLK_PIN,3);

    return res;
}
int sd_wr_blk_sync(int first_block, int block_num, uint8_t* data)
{
    #if 0
    int i;
    int res;
    for(i = 0; i < block_num; i++)
    {
        if(SD_ERR_NONE != (res = sd_wr_sblk_sync(first_block + i, data + i * 512)))
        {
            break;
        }
    }
    return res;
    #else
    return sd_wr_mblk_sync(first_block, block_num, data);
    #endif
}
#endif
int driver_sdcard_get_init_status( void )
{
    return driver_sdcard.init_flag;
}

int driver_sdcard_set_init_status( uint16 val )
{
    driver_sdcard.init_flag = val;
    return 0;
}

int sd_init_process(void)
{
    /*���Խ���ͣʱ�ӵĲ���*/
    int Ret;
    uint8 tmp = 0;

    //����SDIO������
    //BK3000_A1_CONFIG |= (3 << 18);

Reinit:
    SD_HW_Init();
    Set_Sd_Clk(CLK_1_625M);
   CLK_ENABLE;
    Send_Card_CMD0();
    os_printf("cmd 0 ok\r\n");
    os_delay_ms(50);
    Ret = Send_Card_CMD1(); //��ý�忨�ĳ�ʼ����CMD1 ��ʼ
    os_printf("cmd 1:%x \r\n",Ret);
    if(Ret == SD_ERR_NONE)
        goto MMC_init;
		
    Ret = Send_Card_CMD8(); //CMD8 ��Ҫ��ǿ�Ƴ�ʼ�����������õ�,����Ӧ����2.0���ϵĿ�������Ӧ����2.0����
    os_printf("cmd 8 ok:%x\r\n",Ret);
    Ret=Send_Card_Acmd41(); //
    if(Ret==SD_ERR_CMD_TIMEOUT)
        goto exit_init;
    else if(Ret != SD_ERR_NONE)
        goto RE;
    os_printf("acmd 41 ok\r\n");

    Ret=Send_Card_CMD2(); //
    if(Ret!=SD_ERR_NONE)
        goto RE;
    os_printf("cmd 2 ok\r\n");

    Ret=Send_Card_CMD3(); //������롰Stand-by��״̬
    if(Ret!=SD_ERR_NONE)
        goto RE;
    os_printf("cmd 3 ok\r\n");

    //�˴��л�Ƶ��
    //��Щ��������Ϊ26M�󣬳�ʼ����������????
    //sd1.1��֮ǰ�Ŀ�ʱ���Ͽ��ܻ������⣬�����������̫��
    //����2.0�Ŀ�����Ҫ�л�Ϊ26M
    Set_Sd_Clk(CLK_6_5M);

    Ret=Send_Card_CMD9(SD_CARD); //��ÿ���������(Card Specific Data)
    if(Ret!=SD_ERR_NONE)
        goto RE;
    os_printf("cmd 9 ok\r\n");

    Ret=Send_Card_CMD7(); //������ѡ��һ�ſ���Ȼ������л�������ģʽ
    if(Ret!=SD_ERR_NONE)
        goto RE;
    os_printf("cmd 7 ok\r\n");

    Ret=Send_Card_Acmd6(); //ѡ�����߿��
    if(Ret!=SD_ERR_NONE)
        goto RE;
    os_printf("cmd 6 ok\r\n");
    Sd_MMC_flag = SD_CARD;
    NoneedInitflag = 1;

    sd_rd_blk_sync_fptr = driver_sdcard.total_block <= SD_TOTAL_BLOCK_THRESHOLD ? sd_rd_blk_sync_old : sd_rd_blk_sync_new;

    return Ret;
	
MMC_init:
    Ret = Send_Card_CMD2();
    os_printf("cmd 2 :%x\r\n",Ret);
    if(Ret != SD_ERR_NONE)
        goto exit_init;
    Ret = Send_MMCCard_CMD3(); //
    os_printf("cmd 3 :%x\r\n",Ret);
    Set_Sd_Clk(CLK_6_5M);
    Ret = Send_Card_CMD9(MMC_CARD); //
    os_printf("cmd 9 :%x\r\n",Ret);
    if(driver_sdcard.Addr_shift_bit == 0)
    {
        Ret = Send_MMCCard_CMD8(); //
        os_printf("cmd 8 :%x\r\n",Ret);
    }
    if(Ret != SD_ERR_NONE)
        goto exit_init;
    Ret = Send_Card_CMD7();
    if(Ret!=SD_ERR_NONE)
        goto exit_init;
    NoneedInitflag = 1;
    Sd_MMC_flag = MMC_CARD;

    sd_rd_blk_sync_fptr = driver_sdcard.total_block <= SD_TOTAL_BLOCK_THRESHOLD ? sd_rd_blk_sync_old : sd_rd_blk_sync_new;

    return Ret;
	
 RE:
    if(tmp++ < 3)
        goto Reinit;
exit_init:
    os_printf("Error Code: %d\r\n", Ret);
    NoneedInitflag = 0;
    return Ret;
}
//Ret=0:SD init OK,
//     other value:error
int SD_init(void)
{
   
    os_printf("SD_init:%d\r\n",NoneedInitflag);

    /*���Խ���ͣʱ�ӵĲ���*/
    if(NoneedInitflag == 1)
    {
        //os_printf("no need initialize\r\n");
        if(!SD_det_gpio_flag)
            gpio_config(SD_CLK_PIN,2);

        os_delay_ms(5);
        CLK_ENABLE;
        os_delay_ms(5);
        uint32 status = BK3000_SDIO_CMD_RSP_INT_SEL;
        BK3000_SDIO_CMD_RSP_INT_SEL = status;
        BK3000_SDIO_FIFO_THRESHOLD |= SDIO_RX_FIFO_RST_BIT | SDIO_TX_FIFO_RST_BIT|(1<<20);
        os_delay_ms(5);
        /*Config tx/rx fifo threshold*/
        BK3000_SDIO_FIFO_THRESHOLD   = 0x101;// 16 byte to read
        return SD_ERR_NONE;
    }
    return (sd_init_process());
}



void app_sd_init(void)
{
    app_env_handle_t env_h = app_env_get_handle();
    os_memset( &driver_sdcard, 0, sizeof( driver_sdcard ) ) ;

    if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_SD_DETECT_ENA)
    {
        if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_SD_DET_GPIO_ENA)
            SD_det_gpio_flag = 1;
        else
            SD_det_gpio_flag = 0;

        if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_SD_DET_GPIO_ENA)
        {
            env_h->env_cfg.system_para.sdcard_pin = 4; //for bk demo test
            SD_detect_pin = env_h->env_cfg.system_para.sdcard_pin;
        }
        else
        {
            SD_detect_pin = SD_DETECT_DEFAULT_GPIO;
        }

        gpio_config(SD_detect_pin, 3);
        SDOnline    = SD_CARD_OFFLINE;
        driver_sdcard.detect_func = sd_detect_fun;
    }
    os_printf("SD_detect_pin:%d\r\n",SD_detect_pin);
}

void app_sd_scanning(void)
{
    if(driver_sdcard.detect_func)
    {
        (*driver_sdcard.detect_func)();
    }
}
uint8 sd_is_attached(void)
{
    return (SD_CARD_ONLINE == SDOnline);
}

uint8 sd_clk_is_attached(void)
{
    uint32 tmp,mask;
    CLK_DISABLE;
    delay_us(1);
    gpio_config(SD_CLK_PIN,3);
    delay_us(5);
    if(gpio_input(SD_CLK_PIN))
    {
        //os_printf("sd is pull out in BSR:%x\r\n",SDOnline);
        //VICMR_disable_interrupts(&tmp,&mask);
        if(SDOnline == SD_CARD_ONLINE)
        {
            SDOnline    = SD_CARD_OFFLINE;
            msg_put(MSG_SD_DETACH);
            SD_Insert_flag = 0;
            NoneedInitflag = 0;
            cnt_online = 0;
        }
        //VICMR_restore_interrupts(tmp,mask);
        return SD_CARD_OFFLINE;
    }
    else
    {
        gpio_config(SD_CLK_PIN,2);
        delay_us(1);
        CLK_ENABLE;
        cnt_offline = 0;
        return SD_CARD_ONLINE;
    }
}

void sdcard_idle(char enable)
{
    static uint8 flag_idle = 0;
    os_printf("sdcard_idle(%d),%d\r\n",enable,flag_idle);
    if(sd_rd_blk_sync_fptr == sd_rd_blk_sync_new) //תģʽ/��ͣʱ���跢CMD12��������ӦGPIO,����ο��޷��ж�
    {
    #ifdef CONFIG_SDCARD_BY_MULTIBLOCK_WAY
        if(enable&&(flag_idle != 1))
        {
            if((Sd_MMC_flag == SD_CARD) && (driver_sdcard.total_block > 0x100000))
            {
                //os_printf("---Send_Card_cmd12---\r\n");
                BK3000_SDIO_FIFO_THRESHOLD  = 0x101 | (SDIO_RX_FIFO_RST_BIT | (1 << 20));
            if(sd_is_attached())
                Send_Card_cmd12(DEFAULT_DATA_TIME_OUT);
                last_read_block = -1;
                last_start_flag = 0;

                CLK_DISABLE;
                if(!SD_det_gpio_flag)
                {
                    gpio_config(SD_CLK_PIN,3);
                }
            }
            flag_idle = 1;
        }
        else if(flag_idle != 0)
        {
            gpio_config(SD_CLK_PIN,2);
            delay_us(1);
            CLK_ENABLE;
            flag_idle = 0;
        }
    #endif
    }
}

void sdcard_uninit(void)
{	
    os_printf("sdcard_uninit\r\n");
//	CLK_ENABLE;       
    driver_sdcard_read_write_stop();
    sdcard_idle(1);
//    driver_sdcard_cmd_start( 0, SD_CMD_FLAG_CRC_CHECK, 0xFFFFF, 0 );
//    driver_sdcard.init_flag = LastInitFlag;
    /* Close GPIO12_SD CLK,it can improve RF performance */
    //CLK_ENABLE;
    CLK_DISABLE;
    //	os_delay_ms(1);
    if(!SD_det_gpio_flag)
        gpio_config(SD_CLK_PIN,3); 
}

int sdcard_get_size( void )
{
    return driver_sdcard.total_block;
}
int sdcard_get_block_size(void)
{
    return driver_sdcard.block_size;
}
void clr_sd_noinitial_flag(void)
{
    NoneedInitflag = 0;
}
void clear_sd_attached(void)
{
    cnt_online = 0;
}
void clear_sd_detached(void)
{
    cnt_offline = 0;
}
#else
int sd_wr_mblk_sync(int first_block, int block_num, uint8_t* data)
{
    return 0;
}
int sdcard_get_size( void )
{
    return 0;
}
int sdcard_get_block_size(void)
{
    return 0;
}
#endif
// EOF
