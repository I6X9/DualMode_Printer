
#include "bk3000_reg.h"
#include "driver_efuse.h"

uint8 eFuse_write(uint8* data,uint8 addr,uint8 len)
{
    uint8 i;
    uint32 reg=0;
    uint32 delay = 0xffffff;
    if(addr+len>32)
        return 0;
    for(i=0;i<len;i++)
    {
        reg = (addr&0x1f) << 8;
        reg |= (data[i]&0xff)<<16;
        reg |= 2;  // write
        BK3000_EFUSE_CONFIG = reg;
        reg |= 1; // write enable
        BK3000_EFUSE_CONFIG = reg;
//        uart_send(&data[i],1);
        while(BK3000_EFUSE_CONFIG&1)  //!(BK3000_EFUSE_READ_REG&0x100)
        {
            delay--;
            if(delay==1)
                break;
        }
        if(BK3000_EFUSE_CONFIG&1)
        {
            return 0;
        }
        delay = 0xffffff;            
        addr++;
    }
    return 1;
}

uint8 eFuse_read(uint8* data,uint8 addr,uint8 len) 
{
    uint8 i;
    uint32 reg=0;
    uint32 delay = 0xffffff;
    if(addr+len>32)
        return 0;
    for(i=0;i<len;i++)
    {
        data[i]= 0xff;
        reg = (addr&0x1f) << 8;
        reg |= 1; // read enable
        BK3000_EFUSE_CONFIG = reg;
        delay = 0xffffff;
        while((BK3000_EFUSE_CONFIG&1))
        {
            delay--;
            if(delay==1)
                break;
        }        
        if(BK3000_EFUSE_READ_REG&0x100)
        {
            data[i] = BK3000_EFUSE_READ_REG&0xff;
            //os_printf("read efuse %d, value:0x%x\r\n",addr,data[i]);
        }
        else
        {
            os_printf("read efuse %d ERROR\r\n",addr);
            return 0;
        }
        addr++;
    }
    return 1;
}

