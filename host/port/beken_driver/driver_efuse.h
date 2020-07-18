#ifndef _DRIVER_EFUSE_H_
#define _DRIVER_EFUSE_H_

uint8 eFuse_write(uint8* data,uint8 addr,uint8 len);
uint8 eFuse_read(uint8* data,uint8 addr,uint8 len);

#endif
