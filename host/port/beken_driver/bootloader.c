#include <string.h>
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
     
/*******************BK3268 16M bit Flash mapping *******************************
000000H        --------------------------------
                |                            |
                |     Bootloader(8K)         |
                |                            |
002000H         |----------------------------|------
                |                            |   |
                |     BLE NVDS(4K)           |   |
                |                            |   |
003000H         |----------------------------|   |
                |                            |  0xFF
                |     BT_LINKKEY(4K)         |   |
                |                            |   |
004000H         |----------------------------|   |
                |                            |   |
                |     BT_Name & Addr(4K)     |   |
                |                            |   |
005000H         |----------------------------|------
                |     MCU_Info(16B)          |   |
005016H         |----------------------------|   |
                |                            |   |
                |     ENV_CFG(4K)            |   |
                |                            |   |
006028H         |----------------------------|   |
                |                            |  MCU Code
                |                            |   |
                |     Program & Wav(754K)    |   |
                |                            |   |
                |                            |   |
0C2000H         |----------------------------|------
                |     DSP_Info(16B)          |   |
0C2018H         |----------------------------|   |
                |                            |   |
                |                            |  DSP Code
                |     DSP Code(512K)         |   |
                |                            |   |
                |                            |   |
142000H         |----------------------------|------
                |     OTA_Info(16B)          |   |
142010H         |----------------------------|   |
                |                            |   |
                |                            |   |
                |     OTA Data(754K)         |  OTA Backup
                |                            |   |
                |                            |   |
1FF000H         |----------------------------|------
                |                            |
                |  Chip Calibration Data(4K) |
                |                            |
200000H        --------------------------------

1、ToolKit检测相对地址0x107.bit6 == 1时，则将cfg保存在0x4B60（绝对地址为0x005016）起始位置，否则继续按照以前的方式保存

2、ToolKit检测相对地址0x110 - 0x113: 4字节，Program length(no crc)决定wav所放位置，如果值为0xFFFFFFFF，由ToolKit决定长度

3、ToolKit完成配置后将 Program & Wav length(with crc)写回相对地址0x114 - 0x118

4、MCU_INFO/DSP_INFO: OTA_mark(2B):flag(2B):length(4B):version(2B):reserved(6B)

5、OTA_INFO:          OTA_mark(2B):flag(2B):length(4B):crc(2B):reserved(6B)

*************************************************************************/

typedef struct
{
    uint16 mark;
    uint16 flag;
    uint32 len;
    uint16 crc;
    uint8 reserved[6];
}__PACKED_POST__ ota_info_s;

typedef struct
{
    uint16 mark;
    uint16 flag;
    uint32 len;
    uint16 ver;
    uint8 reserved[6];
}__PACKED_POST__ mcu_dsp_info_s;

BOOT_CODE uint32 Boot_flash_read_mID(void) 
{
    unsigned int temp0;
    uint32 flash_id;

    while(reg_FLASH_OPERATE_SW & 0x80000000);
    temp0 = reg_FLASH_OPERATE_SW;
    reg_FLASH_OPERATE_SW = (  (temp0             &  SET_ADDRESS_SW)
                            | (FLASH_OPCODE_RDID << BIT_OP_TYPE_SW)
                            | (0x1               << BIT_OP_SW)
                            | (temp0             &  SET_WP_VALUE));
    while(reg_FLASH_OPERATE_SW & 0x80000000);

    flash_id = reg_FLASH_RDID_DATA_FLASH;
    return (flash_id & 0xFFFFFF);
}

BOOT_CODE void Boot_flash_set_line_mode(uint8 mode) 
{
    if(mode == FLASH_LINE_2) 
    {       
        reg_FLASH_CONF &= (~(7<<BIT_MODE_SEL));
        reg_FLASH_CONF |= (1<<BIT_MODE_SEL);  
    }
}

BOOT_CODE void Boot_flash_set_protect(uint8 all) 
{
    unsigned int temp0;

    while(reg_FLASH_OPERATE_SW & 0x80000000){}

    temp0 = reg_FLASH_CONF; //配置WRSR Status data
    temp0 &= 0xfffe0fff;    // set [BP4:BP0] = 0
    if((Boot_flash_read_mID() == 0xc84015) || (Boot_flash_read_mID() == 0x0b4015))
    {
        if(all == 1)
            reg_FLASH_CONF = (  (temp0 & SET_FLASH_CLK_CONF)
                              | (0x10011 << BIT_WRSR_DATA)); 
        else
            reg_FLASH_CONF = (  (temp0 & SET_FLASH_CLK_CONF)
                              | (0x10000 << BIT_WRSR_DATA));  //protect none
        //Start WRSR
        temp0 = reg_FLASH_OPERATE_SW;
        reg_FLASH_OPERATE_SW = (  (temp0 & SET_ADDRESS_SW)
                                | (FLASH_OPCODE_WRSR << BIT_OP_TYPE_SW)
                                | (0x1               << BIT_OP_SW)
                                | (0x1               << BIT_WP_VALUE)); // make WP equal 1 not protect SRP
        while(reg_FLASH_OPERATE_SW & 0x80000000);    
    }   
    while(reg_FLASH_OPERATE_SW & 0x80000000);   
}

BOOT_CODE void Boot_flash_erase_sector(uint32 address, uint8 erase_size) 
{
    unsigned int temp0;
    uint32 flash_opcode;

    if(erase_size == FLASH_ERASE_32K)
        flash_opcode = FLASH_OPCODE_BE1;
    else if(erase_size == FLASH_ERASE_64K)
        flash_opcode = FLASH_OPCODE_BE2;
    else
        flash_opcode = FLASH_OPCODE_SE;

    while(reg_FLASH_OPERATE_SW & 0x80000000);
    temp0 = reg_FLASH_OPERATE_SW;
    reg_FLASH_OPERATE_SW = (  (address      << BIT_ADDRESS_SW)
                            | (flash_opcode << BIT_OP_TYPE_SW)
                            | (0x1          << BIT_OP_SW)
                            | (temp0        &  SET_WP_VALUE));
    while(reg_FLASH_OPERATE_SW & 0x80000000);
}

BOOT_CODE void Boot_flash_read_data(uint8 *buffer, uint32 address, uint32 len) 
{
    uint32 i, reg_value;
    uint32 addr = address&(~0x1F);
    uint32 buf[8];
    uint8 *pb = (uint8 *)&buf[0];

    while(reg_FLASH_OPERATE_SW & 0x80000000);
    while(len) 
    {
        reg_value = reg_FLASH_OPERATE_SW;
        reg_FLASH_OPERATE_SW = (  (addr              << BIT_ADDRESS_SW)
                                | (FLASH_OPCODE_READ << BIT_OP_TYPE_SW)
                                | (0x1               << BIT_OP_SW)
                                | (reg_value         &  SET_WP_VALUE));
        while(reg_FLASH_OPERATE_SW & 0x80000000);
        
        addr += 32;

        for(i = 0; i < 8; i++)
            buf[i] = reg_FLASH_DATA_FLASH_SW;

        for(i = (address & 0x1F); i < 32; i++) 
        {
            *buffer++ = pb[i];
            address++;
            len--;
            if(len == 0)
                break;
        }
    }
}

BOOT_CODE void Boot_flash_write_data(uint8 *buffer, uint32 address, uint32 len) 
{
    uint32 i, reg_value;
    uint32 addr = address&(~0x1F);
    uint32 buf[8];
    uint8 *pb = (uint8 *)&buf[0];
 
    if(address & 0x1F)
        Boot_flash_read_data(pb, addr, 32);

    while(reg_FLASH_OPERATE_SW & 0x80000000);
    while(len) 
    {
        for(i = (address & 0x1F); i < 32; i++)
        {
            if(len)
            {
                pb[i] = *buffer++;
                address++;
                len--;
            }
            else
                pb[i] = 0xFF;
        }

        for(i = 0; i < 8; i++)
            reg_FLASH_DATA_SW_FLASH = buf[i];

        reg_value = reg_FLASH_OPERATE_SW;
        reg_FLASH_OPERATE_SW = (  (addr            << BIT_ADDRESS_SW)
                                | (FLASH_OPCODE_PP << BIT_OP_TYPE_SW)
                                | (0x1             << BIT_OP_SW)
                                | (reg_value       &  SET_WP_VALUE));
        while(reg_FLASH_OPERATE_SW & 0x80000000);
        addr += 32;
    }
}

BOOT_CODE void Boot_erase_code(uint32 start_addr, uint32 end_addr)
{
    while(start_addr < end_addr)
    {
        if(((start_addr % 0x10000) == 0) && ((start_addr + 0x10000) < end_addr))
        {
            Boot_flash_erase_sector(start_addr, FLASH_ERASE_64K);
            start_addr += 0x10000;
        }
        else if(((start_addr % 0x8000) == 0) && ((start_addr+ 0x8000) < end_addr))
        {
            Boot_flash_erase_sector(start_addr, FLASH_ERASE_32K);
            start_addr += 0x8000;
        }
        else
        {
            Boot_flash_erase_sector(start_addr, FLASH_ERASE_4K);
            start_addr += 0x1000;
        }
    }
}

BOOT_CODE uint16 Boot_calc_crc(uint32 start_addr, uint32 end_addr)
{
    uint8 data[500];
    uint16 crc = 0xFFFF;
    uint16 len = 0;
    uint32 i = 0;
    
    while(start_addr < end_addr)
    {
        i = 0;
        len = ((start_addr + sizeof(data)) <= end_addr) ? sizeof(data) : (end_addr - start_addr);
        Boot_flash_read_data(data, start_addr, len);
        while(i < len) 
        {        
             crc = ((crc >> 8) | (crc << 8)) & 0xFFFF;   
             crc ^= (data[i++] & 0xFF);   
             crc ^= ((crc & 0xFF) >> 4);      
             crc ^= (crc << 12) & 0xFFFF;   
             crc ^= ((crc & 0xFF) << 5) & 0xFFFF;   
        } 
        start_addr += len; 
    }
    
    return crc;
}

BOOT_CODE void Boot_wd_reset(void)
{
    BK3000_WDT_CONFIG = 0x5A0001|0x3fff;
    BK3000_WDT_CONFIG = 0xA50001|0x3fff;
}

BOOT_CODE void Boot_loader(void)
{
#ifdef BEKEN_OTA
    ota_info_s ota_info;

    /* Stop watchdog */
    BK3000_WDT_CONFIG = 0x5A0000;
    BK3000_WDT_CONFIG = 0xA50000;
    
    Boot_flash_read_data((uint8*)&ota_info, OTA_BACKUP_ADDR, sizeof(ota_info_s));
    
    if(OTA_MARK_SUCC == ota_info.mark)
    {
        uint8  data[500];
        uint16 mark = OTA_MARK_FAIL;
        uint32 len = 0;
        uint32 offset = 0;
        uint32 read_addr = OTA_BACKUP_ADDR + 16;
        uint32 write_addr;

        Boot_flash_set_protect(0);
        Boot_flash_set_line_mode(FLASH_LINE_2); 
        
        do
        {
            if(IMAGE_MCU == ota_info.flag)        
            {
                Boot_erase_code(OTA_MCU_ADDR, OTA_DSP_ADDR);
                write_addr = OTA_MCU_ADDR;
            }
            else if(IMAGE_MCU_ADDR == ota_info.flag)  
            {
                Boot_erase_code(OTA_NVDS_ADDR, OTA_DSP_ADDR);
                write_addr = OTA_MCU_ADDR;
            }
            else if(IMAGE_DSP == ota_info.flag)  
            {
                Boot_erase_code(OTA_DSP_ADDR, OTA_BACKUP_ADDR);
                write_addr = OTA_DSP_ADDR;
            }
            
            offset = 0;
            
            while(offset < ota_info.len)
            {
                len = ((offset + sizeof(data)) <= ota_info.len) ? sizeof(data) : (ota_info.len - offset);
                
                Boot_flash_read_data(data, read_addr + offset, len);
                Boot_flash_write_data(data, write_addr + offset, len); 
                
                offset += len;
            }
            
            if(ota_info.crc == Boot_calc_crc(write_addr, write_addr + offset))
            {
                mark = OTA_MARK_SUCC;
            }

            Boot_flash_write_data((uint8*)&mark, write_addr, 2);
        }while(OTA_MARK_SUCC != mark);

        mark = OTA_MARK_FAIL;
        Boot_flash_write_data((uint8*)&mark, OTA_BACKUP_ADDR, 2);     /* Invalid ota backup data when boot success */

        Boot_flash_set_protect(1);  
    }
#endif
}


