#ifndef _DRIVER_RINGBUFF_H_
#define _DRIVER_RINGBUFF_H_


typedef struct _driver_ringbuff_s
{
    uint8 *  buffp;
    uint16 buffer_len;
    uint16 buffer_fill;
    uint16   wptr;
    uint16   rptr;
#if (CONFIG_AUDIO_TRANSFER_DMA == 1)
    uint16 dma_mode;
    uint16 reserved;
#endif
}driver_ringbuff_t;
enum
{
    NONE_FILL = 0,
    AEC_RX_FILL = 1,
    AEC_TX_FILL = 2,
    ADC_FILL  = 3,
    CVSD_FILL  = 4,
    WAV_FILL = 5,
    SBC_FILL = 6,
    LINEIN_FILL = 7,
    DAC_FILL_WITH_COMP = 8
};

//API
/* calc the used size in the rb. */
uint16 rb_get_used_size(driver_ringbuff_t *rb);
int16 rb_get_buffer_size(driver_ringbuff_t *rb);
int16 rb_fill_buffer( driver_ringbuff_t *rb, uint8 *buff, uint16 size, int debug );
void rb_init( driver_ringbuff_t *rb, uint8 *pbuff, uint16 dma_mode, uint16 len );
DRAM_CODE int rb_get_buffer_with_length( driver_ringbuff_t *rb, uint8 *buff, uint16 len );
int rb_get_one_sample( driver_ringbuff_t *rb, uint16 *out );

int16 rb_get_buffdata_size(driver_ringbuff_t *rb);//add by zjw for mp3decode
int rb_get_one_sample_mp3( driver_ringbuff_t *rb, uint16 *out );
int rb_get_buffer_mp3(driver_ringbuff_t *rb, uint8 *buff, uint16 len );
int rb_get_buffer_pcm(driver_ringbuff_t *rb, uint8 *buff, uint16 len );

int16  rb_fill_buffer_MP3( driver_ringbuff_t *rb, uint8 *buff, uint16 size, int debug );
#if (CONFIG_AUDIO_TRANSFER_DMA == 1)
uint16 __INLINE__ rb_get_dma_src_cur_ptr(void);
void __INLINE__ rb_set_dma_src_ptr(uint32 ptr);
#endif
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
int32 rb_get_sample_number(driver_ringbuff_t *rb);
#endif

#endif
