#include "config.h"
#include "app_ota.h"
#include "app_beken_includes.h"
#include "lmp_utils.h"
/* RW ble include*/
#include "rwip_config.h"           // SW configuration
#include <string.h>
#include "app_ota.h"                // Bracese Application Module Definitions
#include "app.h"                     // Application Definitions
#include "app_task.h"                // application task definitions
#include "otas.h"
#include "otas_task.h"               // health thermometer functions
#include "co_bt.h"
#include "prf_types.h"               // Profile common types definition
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "lld_evt.h"
#include "uart.h"
#include "app.h"
#include "gattc.h"

#ifdef BEKEN_OTA
#define HI_UINT16(a)             (((a) >> 8) & 0xFF)
#define LO_UINT16(a)             ((a) & 0xFF)

#define OTA_VID                  0x1000
#define OTA_PID                  0x2000

#define OTA_BUFFER_SIZE          1000
#define OTA_PKT_ADDR_LEN         4
#define OTA_ADDR_OFFSET          0x20

typedef enum
{
    INQUIRY_INFO_REQ_CMD       = 0x01,
    INQUIRY_INFO_RESP_CMD      = 0x02,
    START_REQ_CMD              = 0x03,
    START_RESP_CMD             = 0x04,
    DATA_SEND_CMD              = 0x05,
    DATA_ERROR_CMD             = 0x06,
    END_REQ_CMD                = 0x07,
    END_RESP_CMD               = 0x08,
    UPDATE_SIZE_REQ_CMD        = 0x09,
    UPDATE_SIZE_RESP_CMD       = 0x0A,
    REBOOT_CMD                 = 0x0B
}__PACKED_POST__ app_ota_cmd_s;

enum app_ota_result
{
    OTA_SUCCESS                = 0x00,
    OTA_FAIL                   = 0x01
};

enum app_ota_location
{
    OTA_LOCATION_A             = 0x01,
    OTA_LOCATION_B             = 0x02
};

typedef struct
{
    uint16 flag;
    uint16 vid;
    uint16 pid;
    uint16 ver;
    uint32 len;
    uint16 crc;
    uint8 reserved[18];
}__PACKED_POST__ app_ota_head_s;

typedef struct
{
    uint8 cmd;
    uint8 frame_seq;
    uint16 length;
    uint8 data[0];
}__PACKED_POST__ app_ota_pkt_s;

typedef struct
{
    uint8  update_flag;
    uint8  flash_protect_flag;
    uint8  tx_arqn_nak_flag;
    uint16 crc;
    uint32 flash_addr;
    uint32 flash_offset; 
    uint32 data_addr;
    uint8  data[OTA_BUFFER_SIZE];
    uint16 data_len;
    uint8  frame_seq;
}__PACKED_POST__ app_ota_param_s;

#ifdef BEKEN_OTA_SPP
static uint8 ota_spp_buff[OTA_BUFFER_SIZE] = {0};
static uint16 ota_spp_buff_cnt = 0;
#endif

static app_ota_param_s app_ota_param;

extern unsigned char spp_is_connected(void);
extern result_t spp_send( char *buff, uint8_t len );
extern void set_flash_protect(uint8 all);
void app_ota_ble_send(uint8_t* pvalue, uint16_t length);

uint8 app_ota_is_ongoing(void)
{
    return app_ota_param.update_flag; 
}

uint8 app_ota_tx_arqn_nak_flag_get(void)
{
    return app_ota_param.tx_arqn_nak_flag;
}

void app_ota_tx_arqn_nak_flag_set(uint8 value)
{
    app_ota_param.tx_arqn_nak_flag = value;
}

uint16 app_ota_get_version(uint8 flag)
{
    uint16 version = 0xFFFF;
    
    if((IMAGE_MCU == flag) || (IMAGE_MCU_ADDR == flag))
        flash_read_data((uint8*)&version, OTA_MCU_ADDR + 8, sizeof(version));
    else if(IMAGE_DSP == flag)
        flash_read_data((uint8*)&version, OTA_DSP_ADDR + 8, sizeof(version));
    
    return version;
}

uint16 app_ota_get_mark(uint32 addr)
{
    uint16 mark = 0x1234;
    
    flash_read_data((uint8*)&mark, addr, sizeof(mark));
    
    return mark;
}

uint16 gen_crc16(uint16 crc, uint8* data, uint32 len) 
{        
    uint32 i;
    for (i = 0; i < len; i++) 
    {        
         crc = ((crc >> 8) | (crc << 8)) & 0xFFFF;   
         crc ^= (data[i] & 0xFF);// byte to int, trunc sign    
         crc ^= ((crc & 0xFF) >> 4);      
         crc ^= (crc << 12) & 0xFFFF;   
         crc ^= ((crc & 0xFF) << 5) & 0xFFFF;   
    }       
    return (crc & 0xFFFF);
}

uint16 app_ota_calc_crc(void)
{
    uint8 data[500];
    uint16 crc = 0xFFFF;
    uint16 len = 0;
    uint32 read_addr = app_ota_param.flash_addr;
    uint32 end_addr = app_ota_param.flash_addr + app_ota_param.flash_offset;

    while(read_addr < end_addr)
    {
        if((read_addr + sizeof(data)/sizeof(data[0])) <= end_addr)
            len = sizeof(data)/sizeof(data[0]);
        else
            len = end_addr - read_addr;
        
        flash_read_data(data, read_addr, len);
        crc = gen_crc16(crc, data, len);
        read_addr += len; 
    }
    return crc;
}

void app_ota_erase_flash(void)
{
    uint32 start_addr = 0;
    uint32 end_addr = 0;
    uint32 addr = 0;

    if(OTA_MARK_INIT != app_ota_get_mark(OTA_BACKUP_ADDR))
    {
        start_addr = OTA_BACKUP_ADDR;
        end_addr   = OTA_END_ADDR - 0x1000;        /* 4K calibration data */
    }  

    if(end_addr > start_addr)
    {
        os_printf("---OTA erase flash addr:0x%x, Please Waiting...\r\n\r\n", start_addr); 
        for(addr = end_addr; addr > start_addr; )  /* erase addr from high to low, ensure ota_mark erased last */
        {
            if((((addr - 0x10000) % 0x10000) == 0) && ((addr - 0x10000) >= start_addr))
            {
                addr -= 0x10000;
                flash_erase_sector(addr, FLASH_ERASE_64K);
            }
            else if((((addr - 0x8000) % 0x8000) == 0) && ((addr - 0x8000) >= start_addr))
            {
                addr -= 0x8000;
                flash_erase_sector(addr, FLASH_ERASE_32K);
            }
            else
            {
                addr -= 0x1000;
                flash_erase_sector(addr, FLASH_ERASE_4K);
            }
            CLEAR_WDT;
        }
    }
}

void app_ota_write_flash(void)
{
    if(app_ota_param.flash_protect_flag == 0x01)         //flash write unprotect
    {
        set_flash_protect(0);
        app_ota_param.flash_protect_flag = 0;
    }
    else if(app_ota_param.flash_protect_flag == 0x02)    //flash write protect
    {
        set_flash_protect(1);
        app_ota_param.flash_protect_flag = 0;
    }

    if(app_ota_is_ongoing() && app_ota_param.data_len)
    {
        flash_write_data(app_ota_param.data, app_ota_param.flash_addr + app_ota_param.flash_offset, app_ota_param.data_len);
        app_ota_param.flash_offset += app_ota_param.data_len;
        app_ota_param.data_len = 0;
    }
}

void app_ota_pdu_send(uint8 *pValue, uint16 length)
{
#ifdef BEKEN_OTA_SPP
    if(spp_is_connected())
    {
        spp_send((char*)pValue, length);
    }
    else
#endif
#ifdef BEKEN_OTA_BLE
    if(appm_ble_is_connection())
    {
        app_ota_ble_send(pValue, length);
    }
    else
#endif
    {
        os_printf("app_ota_pdu_send error!!!\r\n");
    }
}

void app_ota_pkt_encode(uint8 cmd, uint8 frame_seq, uint8 *pValue, uint16 length)
{
    uint8 data[30];
    app_ota_pkt_s* app_ota_pkt = (app_ota_pkt_s*)data;

    if((length + sizeof(app_ota_pkt_s)) > sizeof(data)/sizeof(data[0]))
    {
        os_printf("app_ota_pkt_encode error!!!:%x, %x\r\n", length + sizeof(app_ota_pkt_s), sizeof(data)/sizeof(data[0]));
        return;
    }
    
    app_ota_pkt->cmd = cmd;
    app_ota_pkt->frame_seq = frame_seq;
    app_ota_pkt->length = length;
    memcpy(&app_ota_pkt->data, pValue, length);
    
    app_ota_pdu_send(data, length + sizeof(app_ota_pkt_s));
}

void app_ota_inquiry_req_handler(uint8 *pValue, uint16 length)
{
    app_ota_pkt_s* app_ota_pkt = (app_ota_pkt_s*)pValue;
    uint8 data[7];
    uint8 location = OTA_LOCATION_A;
    uint16 version = app_ota_get_version(IMAGE_MCU);

    os_printf("app_ota_inquiry_req_handler\r\n");

    memset((uint8*)&app_ota_param, 0, sizeof(app_ota_param_s));
    
    data[0] = LO_UINT16(OTA_VID);
    data[1] = HI_UINT16(OTA_VID);
    data[2] = LO_UINT16(OTA_PID);
    data[3] = HI_UINT16(OTA_PID);
    data[4] = LO_UINT16(version);
    data[5] = HI_UINT16(version);
    data[6] = location;

    app_ota_pkt_encode(INQUIRY_INFO_RESP_CMD, app_ota_pkt->frame_seq, data, sizeof(data)/sizeof(data[0]));
}

void app_ota_start_req_handler(uint8 *pValue, uint16 length)
{
    app_ota_pkt_s* app_ota_pkt = (app_ota_pkt_s*)pValue;
    app_ota_head_s* head_info = (app_ota_head_s*)app_ota_pkt->data;
    uint16 local_ver = app_ota_get_version(head_info->flag);
    uint16 data_size = sizeof(app_ota_param.data)/sizeof(app_ota_param.data[0]) - sizeof(app_ota_pkt_s) - OTA_PKT_ADDR_LEN;
    uint8 data[11];
    
    os_printf("app_ota_start_req_handler\r\n");
    os_printf("flag:0x%x, len:0x%x, crc:0x%x\r\n", head_info->flag, head_info->len, head_info->crc);
    os_printf("[VID:PID:version]: local[0x%x:0x%x:0x%x], update[0x%x:0x%x:0x%x]\r\n", OTA_VID, OTA_PID, local_ver, head_info->vid, head_info->pid, head_info->ver);

    memset(data, 0, sizeof(data)/sizeof(data[0]));
    if((OTA_VID == head_info->vid) && (OTA_PID == head_info->pid) /*&& (head_info->ver > local_ver)*/ && (OTA_MARK_INIT == app_ota_get_mark(OTA_BACKUP_ADDR)))
    {
        uint32 addr = 0;
        uint32 len = 0;
        
#ifdef BEKEN_OTA_BLE
        if(appm_ble_is_connection())
        {
            appm_update_param(16, 20, 0, 600); /* BLE interval:20ms~25ms, no latency, timeout:6s */
            
            data_size = gattc_get_mtu(0) - sizeof(app_ota_pkt_s) - OTA_PKT_ADDR_LEN;
            if(data_size > ((sizeof(app_ota_param.data)/sizeof(app_ota_param.data[0]))/2))
                data_size = (sizeof(app_ota_param.data)/sizeof(app_ota_param.data[0]))/2;
        }
#endif

        addr = OTA_ADDR_OFFSET;
        len = head_info->len;
        
        app_ota_param.crc = head_info->crc;
        app_ota_param.data_addr = addr;
        app_ota_param.frame_seq = app_ota_pkt->frame_seq;
        app_ota_param.update_flag = 1;
        app_ota_param.flash_protect_flag = 0x01;         //flash write unprotect
        app_ota_param.flash_addr = OTA_BACKUP_ADDR;

        data[0] = OTA_SUCCESS;
        memcpy(&data[1], (uint8*)&addr, 4);
        memcpy(&data[5], (uint8*)&len, 4);
        memcpy(&data[9], (uint8*)&data_size, 2);

        if(1)
        {
            app_handle_t sys_hdl = app_get_sys_handler();
            
            bt_unit_set_scan_enable(sys_hdl->unit, HCI_NO_SCAN_ENABLE);          //set scan disable
            app_bt_write_sniff_link_policy(bt_sniff_get_handle_from_idx(0), 0);  //exit bt sniff mode
        }

        os_printf("app_ota_start_OK!!!\r\n");
    }
    else
    {
        data[0] = OTA_FAIL;
        os_printf("app_ota_start_FAIL!!!\r\n");
    }
    app_ota_pkt_encode(START_RESP_CMD, app_ota_pkt->frame_seq, data, sizeof(data)/sizeof(data[0]));
}

void app_ota_data_send_handler(uint8 *pValue, uint16 length)
{
    app_ota_pkt_s* app_ota_pkt = (app_ota_pkt_s*)pValue;
    uint32 addr = LMutils_Get_Uint32(app_ota_pkt->data);
    uint8* data_ptr = app_ota_pkt->data + sizeof(app_ota_pkt_s);
    uint16 data_len = app_ota_pkt->length - OTA_PKT_ADDR_LEN;
        
    if(((uint8)(app_ota_param.frame_seq + 1) == app_ota_pkt->frame_seq) && (app_ota_param.data_addr == addr))
    {
        if((app_ota_param.data_len + data_len) > (sizeof(app_ota_param.data)/sizeof(app_ota_param.data[0])))
        {
            while(app_ota_param.data_len)
            {
                app_ota_write_flash();
                os_printf("app_ota_data overflow!!!\r\n");
            }
        }
        memcpy(app_ota_param.data + app_ota_param.data_len, data_ptr, data_len);
        app_ota_param.data_len += data_len;
        app_ota_param.frame_seq++;
        app_ota_param.data_addr += data_len;
    }
    else
    {
        app_ota_pkt_encode(DATA_ERROR_CMD, app_ota_param.frame_seq, (uint8*)&app_ota_param.data_addr, sizeof(app_ota_param.data_addr));
        os_printf("app_ota_data_error:%x,%x\r\n", app_ota_param.frame_seq, app_ota_param.data_addr);
    }
}

void app_ota_end_req_handler(uint8 *pValue, uint16 length)
{
    app_ota_pkt_s* app_ota_pkt = (app_ota_pkt_s*)pValue;
    uint8 data[1];
    uint16 crc;
    uint16 ota_mark = 0xFFFF;

    os_printf("app_ota_end_req_handler\r\n");
    
    while(app_ota_param.data_len)
    {
        app_ota_write_flash();         //ensure last data write to flash
    }
    
    crc = app_ota_calc_crc();
    
    os_printf("crc: 0x%x, 0x%x\r\n", crc, app_ota_param.crc);
    if(crc == app_ota_param.crc)
    {
        data[0] = OTA_SUCCESS;
        ota_mark = OTA_MARK_SUCC;
        os_printf("OTA_SUCCESS!!!\r\n");
    }
    else
    {
        data[0] = OTA_FAIL;
        ota_mark = OTA_MARK_FAIL;
        os_printf("OTA_FAIL!!!\r\n");
    }
    flash_write_data((uint8*)&ota_mark, app_ota_param.flash_addr, sizeof(ota_mark));     

    app_ota_param.flash_protect_flag = 0x02;        //flash write protcet
    app_ota_pkt_encode(END_RESP_CMD, app_ota_pkt->frame_seq, data, sizeof(data)/sizeof(data[0]));
}

void app_ota_update_size_req(uint16 size)
{
    uint16 max_size = sizeof(app_ota_param.data)/sizeof(app_ota_param.data[0]) - sizeof(app_ota_pkt_s) - OTA_PKT_ADDR_LEN;

    os_printf("app_ota_update_size_req:%x\r\n",size);
    
    if(size < max_size)
        app_ota_pkt_encode(UPDATE_SIZE_REQ_CMD, 0xFF, (uint8*)&size, sizeof(uint16));
    else
        os_printf("app_ota_update_size error:%x,%x\r\n", size, max_size);
}

void app_ota_update_size_resp_handler(uint8 *pValue, uint16 length)
{
    app_ota_pkt_s* app_ota_pkt = (app_ota_pkt_s*)pValue;
    
    if(app_ota_pkt->frame_seq == 0xFF)
        os_printf("app_ota_update_size_resp_handler\r\n");
}

void app_ota_reboot_handler(uint8 *pValue, uint16 length)
{
    os_printf("app_ota_reboot_handler\r\n\r\n");
    memset((uint8*)&app_ota_param, 0, sizeof(app_ota_param_s));
    BK3000_wdt_reset();
}

void app_ota_pkt_decode(uint8 *pValue, uint16 length)
{
    switch(pValue[0])
    {
        case INQUIRY_INFO_REQ_CMD:
            app_ota_inquiry_req_handler(pValue, length);
            break;
            
        case START_REQ_CMD:
            app_ota_start_req_handler(pValue, length);
            break; 
            
        case DATA_SEND_CMD:
            app_ota_data_send_handler(pValue, length);
            break;
            
        case END_REQ_CMD:
            app_ota_end_req_handler(pValue, length);
            break;
            
        case UPDATE_SIZE_RESP_CMD:
            app_ota_update_size_resp_handler(pValue, length);
            break;
            
        case REBOOT_CMD:
            app_ota_reboot_handler(pValue, length);
            break;

        default:
            os_printf("app_ota_pkt_decode error!!!\r\n");
            break;
    }
}

#ifdef BEKEN_OTA_BLE
void app_otas_init(void)
{
    memset((uint8*)&app_ota_param, 0, sizeof(app_ota_param_s));
}

void app_ota_add_otas(void)
{
    struct otas_db_cfg *db_cfg;
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM, TASK_APP,
                                                             gapm_profile_task_add_cmd, sizeof(struct otas_db_cfg));
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = 0;//PERM(SVC_AUTH, ENABLE);
    req->prf_task_id = TASK_ID_OTAS;
    req->app_task = TASK_APP;
    req->start_hdl = 0; 

	// Set parameters
    db_cfg = (struct otas_db_cfg* ) req->param;
    // Sending of notifications is supported
    db_cfg->features = OTAS_NTF_SUP;
    
	//UART_PRINTF("app_oad_add_otas d = %x,s = %x\r\n", TASK_GAPM,TASK_APP);
    ke_msg_send(req);
}

void app_ota_ble_send(uint8 *pValue, uint16_t length)
{
    struct otas_tx_pdu * tx_pdu = KE_MSG_ALLOC( OTAS_TX,
                                                prf_get_task_from_id(TASK_ID_OTAS),
                                                TASK_APP,
                                                otas_tx_pdu);

    // Fill in the parameter structure
    tx_pdu->length = length;
    memcpy((uint8*)&tx_pdu->data, pValue, length);

    // Send the message
    ke_msg_send(tx_pdu);
}

int app_ota_ble_pkt_decode(ke_msg_id_t const msgid, struct otas_rx_pdu const *param,
                                    ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    app_ota_pkt_decode(param->data, param->length);
    return (KE_MSG_CONSUMED);
}

const struct ke_msg_handler app_otas_msg_handler_list[] =
{   
    {OTAS_RX,                 (ke_msg_func_t)app_ota_ble_pkt_decode},	
};

const struct ke_state_handler app_otas_table_handler =
                              {&app_otas_msg_handler_list[0], (sizeof(app_otas_msg_handler_list)/sizeof(struct ke_msg_handler))};

#endif

#ifdef BEKEN_OTA_SPP
void app_ota_spp_pkt_reframe(uint8 *pValue, uint16 length)
{
    uint16 copy_len = 0;
    uint16 pkt_len = 0;
    
    do
    {
        if((ota_spp_buff_cnt + length) <= sizeof(ota_spp_buff)/sizeof(ota_spp_buff[0]))
            copy_len = length;
        else
            copy_len = sizeof(ota_spp_buff)/sizeof(ota_spp_buff[0]) - ota_spp_buff_cnt;
        
        memcpy(&ota_spp_buff[ota_spp_buff_cnt], pValue, copy_len);
        ota_spp_buff_cnt += copy_len;
        pValue += copy_len;
        length -= copy_len;

        while(ota_spp_buff_cnt >= sizeof(app_ota_pkt_s))
        {
            memcpy((uint8*)&pkt_len, &ota_spp_buff[2], 2);
            pkt_len += sizeof(app_ota_pkt_s);
            if(pkt_len <= ota_spp_buff_cnt)
            {
                app_ota_pkt_decode(ota_spp_buff, pkt_len);
                memcpy(ota_spp_buff, &ota_spp_buff[pkt_len], ota_spp_buff_cnt - pkt_len);
                ota_spp_buff_cnt -= pkt_len;
            }
            else
            {
                break;
            }
        }
    }while(length);
}
#endif

#else
uint8 app_ota_is_ongoing(void)
{
    return 0; 
}

uint8 app_ota_tx_arqn_nak_flag_get(void)
{
    return 0;
}

void app_ota_tx_arqn_nak_flag_set(uint8 value)
{
}

#endif
