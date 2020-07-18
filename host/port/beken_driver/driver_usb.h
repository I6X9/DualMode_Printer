#ifndef _DRIVER_USB_H
#define _DRIVER_USB_H


#define USB_RET_OK          0
#define USB_RET_ERROR       1
#define USB_RET_CONNECT     2
#define USB_RET_DISCONNECT  3
#define USB_RET_READ_OK     4
#define USB_RET_WRITE_OK    5


typedef enum        // by gwf
{
    USB_HOST_MODE   = 0,
    USB_DEVICE_MODE = 1
} USB_MODE;

typedef struct __driver_udisk_s
{
    uint32_t    dwBlockSize;
    uint32_t    dwBlockCountLo;
    uint32_t    dwBlockCountHi;
    uint16      InitFlag;
} driver_udisk_t;

void usb_init(USB_MODE usb_mode);
void usb_uninit(void);
void usb_reinit_to_mode(USB_MODE usb_mode);
void usb_reset(void);
void usb_reinit(USB_MODE usb_mode);
void pre_usb_init(void);
void post_usb_init(void);
uint8 udisk_is_attached(void);
uint8 udisk_init(void);
void udisk_uninit(void);
uint32 udisk_get_size(void);
void usb_isr(void);
int udisk_rd_blk_sync(uint32 first_block, uint32 block_num, uint8 *dest);

#endif
