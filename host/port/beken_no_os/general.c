/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include "includes.h"
#include "bk3000_reg.h"
#include "tra_hcit.h"
#include "tc_const.h"
#include "app_beken_includes.h"
#include "string.h"
#ifdef CONFIG_PRODUCT_TEST_INF
#include "driver_icu.h"
#endif

#if PTS_TESTING
#include "pts/pts.h"
#endif

#include "beken_external.h"
#include "app.h"
#include "app_env.h"
#include "app_task.h"
#include "app_env.h"
#include "driver_flash.h"
#include "app_fcc0.h"




unsigned char uart_rx_buf[200]={0};
unsigned char uart_tx_buf[200]={0};
volatile boolean uart_rx_done = FALSE;
volatile unsigned int uart_rx_index = 0;

#ifdef	CONFIG_BLUETOOTH_AVDTP_SCMS_T
extern void security_control_cp_support_print(void);  //print the array's param for check
#endif

enum
{
    DBG_HCI_STATE_RX_TYPE = 0,
    DBG_HCI_STATE_RX_COMMAND_OPCODE1,
    DBG_HCI_STATE_RX_COMMAND_OPCODE2,
    DBG_HCI_STATE_RX_COMMAND_LENGTH,
    DBG_HCI_STATE_RX_DATA_START,
    DBG_HCI_STATE_RX_DATA_CONTINUE,
    DBG_HCI_STATE_RX_DATA_COMMIT
};


/*
 * uart_initialise
 *
 * This function initialises the UART registers & UART driver paramaters.
 */
void uart_initialise(u_int32 baud_rate) {
    u_int32 baud_divisor;
    app_env_handle_t  env_h = app_env_get_handle();

    baud_divisor           = UART_CLOCK_FREQ_26M/baud_rate;
    baud_divisor           = baud_divisor-1;

    REG_APB3_UART_CFG      = (   (DEF_STOP_BIT    << sft_UART_CONF_STOP_LEN)
                                 | (DEF_PARITY_MODE << sft_UART_CONF_PAR_MODE)
                                 | (DEF_PARITY_EN   << sft_UART_CONF_PAR_EN)
                                 | (DEF_DATA_LEN    << sft_UART_CONF_UART_LEN)
                                 | (baud_divisor    << sft_UART_CONF_CLK_DIVID)
                                 | (DEF_IRDA_MODE   << sft_UART_CONF_IRDA)
                                 | (DEF_RX_EN       << sft_UART_CONF_RX_ENABLE)
                                 | (DEF_TX_EN       << sft_UART_CONF_TX_ENABLE));

    REG_APB3_UART_FIFO_THRESHOLD = ((RX_FIFO_THRD << sft_UART_FIFO_CONF_RX_FIFO)
                                    | (TX_FIFO_THRD << sft_UART_FIFO_CONF_TX_FIFO));
    REG_APB3_UART_INT_ENABLE=0;             /* Disable UART Interrupts */
    REG_APB3_UART_INT_ENABLE = bit_UART_INT_RX_NEED_READ | bit_UART_INT_RX_STOP_END; //enable Rx interrupt

    if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_UART01DBG)
    {
        BK3000_GPIO_0_CONFIG = 0x70; //GPIO 0
        BK3000_GPIO_1_CONFIG = 0x7C; //GPIO 1
        BK3000_GPIO_PAD_CTRL |= 1 << 13;
    }
    else if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_UART67DBG)
    {
        BK3000_GPIO_10_CONFIG = 0x70;
        BK3000_GPIO_11_CONFIG = 0x7C;
        (*(volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+0x20*4)) |= (0x1<<10)|(0x1<<11); //gpio_pconf2

        BK3000_GPIO_59_CONFIG |= (1 << 10) | (1 << 11);
        BK3000_GPIO_45_CONFIG &= ~(1 << 13);
    }

}

void uart_send (unsigned char *buff, unsigned int len) {
    
#ifdef CONFIG_APP_DATA_CAPTURE

	if( !app_aec_get_data_capture_mode() ) {
		while (len--) {
			while(!UART_TX_WRITE_READY);
                UART_WRITE_BYTE(*buff++);
		}
	}
    
#else

    while (len--) {
        while(!UART_TX_WRITE_READY);
            UART_WRITE_BYTE(*buff++);
    }
    
#endif

}

#if (CONFIG_DEBUG_PCM_TO_UART == 1)
void uart_send_ppp(unsigned char *buff, unsigned char fid,unsigned short len)
{
    unsigned char xsum = 0;
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.

    while(!UART_TX_WRITE_READY);
    UART_WRITE_BYTE(0x7E);
    while(!UART_TX_WRITE_READY);
    UART_WRITE_BYTE(fid);
    xsum ^= fid;
    while (len--)
    {
        xsum ^= *buff;
        if(0x7E == *buff)
        {   while(!UART_TX_WRITE_READY);
            UART_WRITE_BYTE(0x7D);
            while(!UART_TX_WRITE_READY);
            UART_WRITE_BYTE(0x5e);
        }
        else if(0x7D == *buff)
        {
            while(!UART_TX_WRITE_READY);
            UART_WRITE_BYTE(0x7D);
            while(!UART_TX_WRITE_READY);
            UART_WRITE_BYTE(0x5D);
        }
        else
        {
            while(!UART_TX_WRITE_READY);
            UART_WRITE_BYTE(*buff);
        }
        buff++;
    }
    if(0x7E == xsum)
    {   while(!UART_TX_WRITE_READY);
        UART_WRITE_BYTE(0x7D);
        while(!UART_TX_WRITE_READY);
        UART_WRITE_BYTE(0x5e);
    }
    else if(0x7D == xsum)
    {
        while(!UART_TX_WRITE_READY);
        UART_WRITE_BYTE(0x7D);
        while(!UART_TX_WRITE_READY);
        UART_WRITE_BYTE(0x5D);
    }
    else
    {
        while(!UART_TX_WRITE_READY);
        UART_WRITE_BYTE(xsum);
    }
    while(!UART_TX_WRITE_READY);
    UART_WRITE_BYTE(0x7E);
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.

}
#endif
#define PRINT_BUF_PREPARE(rc, buf, fmt)             \
    int rc;                                         \
    va_list args;                                   \
    va_start(args, fmt);                            \
    rc = vsnprintf(buf, sizeof(buf), fmt, args);    \
    va_end(args);                                   \
    buf[sizeof(buf) - 1] = '\0';
#define PRINT_BUF_SIZE 0X100
#if (CONFIG_DEBUG_PCM_TO_UART == 1)
static int32 os_printf_cnt = 0;
int32 os_printf(const char *fmt, ...) {
    char buf[PRINT_BUF_SIZE];

    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.
#if 1
    if(os_printf_cnt < 1)
    {
        PRINT_BUF_PREPARE(rc, buf, fmt);
        uart_send((unsigned char *)&buf[0], rc);
    }
    else
    {
        delay_us(1);
    }
    os_printf_cnt++;
#endif
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
    return 0;
}
#else
int32 os_printf(const char *fmt, ...) {
#if 1
#if (BT_HOST_MODE == JUNGO_HOST)
    char buf[PRINT_BUF_SIZE];

    //unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    //set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.

    PRINT_BUF_PREPARE(rc, buf, fmt);
    uart_send((unsigned char *)&buf[0], rc);

    //set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
    return rc;
 #else
    //unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    //set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.
    delay_us(10);
    //set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
    return 0;
 #endif
 #endif
}
#endif
int32 os_null_printf(const char *fmt, ...) {
    return 0;
}

void clear_uart_buffer(void) {
    uart_rx_index = 0;
    uart_rx_done = FALSE;
    memset(uart_rx_buf, 0, sizeof(uart_rx_buf)); /**< Clear the RX buffer */
    memset(uart_tx_buf, 0, sizeof(uart_tx_buf)); /**< Clear the TX buffer */
}



static void app_debug_showstack(void) {
    extern uint32 _sbss_end;
    extern uint32 _stack;
    uint32 count;
    uint32 *ptr;
    uint32 i;

    count = (((uint32)&_stack - (uint32)&_sbss_end) >> 2) - 2;
    ptr = (uint32 *)((uint32)&_sbss_end  & (~3));

    os_printf("ShowStack:%p:%p\r\n",  &_sbss_end, &_stack);
    for(i = 0; i < count; i ++)
        os_printf("0x%x:%p\r\n", &ptr[i], ptr[i]);
}

#if (DEBUG_AGC_MODE_CHANNEL_ASSESSMENT == 1)
extern void _LSLCacc_Read_AGC_Param(void);
#endif
extern void hfp_app_ptr_debug_printf(void);

extern int hf_sco_handle_process(int oper);
#define HF_SCO_CONN 0
#define HF_SCO_DISCONN 1

//extern void app_bt_sdp_connect(void);
extern void app_led_dump(void);
#ifdef CONFIG_APP_EQUANLIZER
extern void app_set_eq_gain(int8 gain);
#endif

extern uint32 XVR_reg_0x24_save;
extern u_int8 edr_tx_edr_delay, edr_rx_edr_delay;


extern result_t spp_send( char *buff, uint8_t len );
int32 os1_printf(const char *fmt, ...) {

    char buf[PRINT_BUF_SIZE];

    //unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    //set_spr(SPR_VICMR(0), 0x00);                     //mask all/low priority interrupt.

    PRINT_BUF_PREPARE(rc, buf, fmt);
    uart_send((unsigned char *)&buf[0], rc);

    //set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
    return rc;

}

void uart_rx_data_handle(void){


	os_printf("rx_data=%s,datalen=%d\n",uart_rx_buf,uart_rx_index);
	if((strncmp(uart_rx_buf,"AT+GETNAME",strlen("AT+GETNAME")))==0){

		os1_printf("AT_BLE_BT_NAME=%s\n",(uint8_t *)app_env_local_bd_name());

	
	}
	else if((strncmp(uart_rx_buf,"AT+GETNADDR",strlen("AT+GETNADDR")))==0){

		int i=0;
		uint8_t bluetooth_addr[6];
		memcpy((uint8_t *)&bluetooth_addr,(uint8_t *)app_env_local_bd_addr(),6);
		
		os1_printf("AT_BLE_BT_ADDR=");
		for(i=0;i<6;i++){

			os1_printf("%02x",bluetooth_addr[i]);
		}
		os1_printf("\n");
		
	}

	else if((strncmp(uart_rx_buf,"AT+SETNAME=",strlen("AT+SETNAME=")))==0){


		uint8_t btaddr_name[32];


		memset(btaddr_name,'\0',32);
		memcpy(btaddr_name,&uart_rx_buf[11],uart_rx_index-strlen("AT+SETNAME=")-2);
		os1_printf("BT_SET_NAME=%s\n",btaddr_name);
		flash_erase_sector(RPINTER_ENV_ADDR, FLASH_ERASE_4K);
		flash_write_data((uint8*)&btaddr_name, (uint32)RPINTER_ENV_ADDR,uart_rx_index-strlen("AT+SETNAME=")-2);
		
		
	}
	else if((strncmp(uart_rx_buf,"AT+RESET",strlen("AT+RESET")))==0){



		 flash_erase_sector(RPINTER_ENV_ADDR, FLASH_ERASE_4K);
		 os1_printf("AT+RESET_OK\n");
		
	}
	else if((strncmp(uart_rx_buf,"AT+BLESEND=",strlen("AT+BLESEND=")))==0){


			app_ff03_send_data(&uart_rx_buf[11],uart_rx_index-strlen("AT+SETNAME=")-2);
			os1_printf("AT+BLESEND=OK\n");
			
	}else if((strncmp(uart_rx_buf,"AT+SPPSEND=",strlen("AT+SPPSEND=")))==0){


			spp_send(&uart_rx_buf[11],uart_rx_index-strlen("AT+SETNAME=")-2);
			os1_printf("AT+SPPSEND==OK\n");


	}
	clear_uart_buffer();
		

}



void uart_handler(void) {
    u_int32 status;
    //u_int8 value;

    status = REG_APB3_UART_INT_FLAG;
    if(status & (bit_UART_INT_RX_NEED_READ|bit_UART_INT_RX_STOP_END)) {
        while (UART_RX_READ_READY) {
           // value=UART_READ_BYTE();
    		uart_rx_buf[uart_rx_index++]=UART_READ_BYTE();
		   	if(uart_rx_index==200){

				uart_rx_index=0;

			}
        }
    }
    if(status &bit_UART_INT_RX_STOP_END){
	uart_rx_data_handle();
    }
    REG_APB3_UART_INT_FLAG = status;
}
