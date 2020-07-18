#ifndef _APP_SBC_H_
#define _APP_SBC_H_

#include "driver_beken_includes.h"
#include "jheaders.h"
#include "jos_mbuf.h"
#include "sbc.h"
#define SBC_FRAME_LENGTH_MAX    120 /* shall not be changed. */
#ifdef CONFIG_TWS
    #include <bluetooth\core\hci_internal.h>
    #define SBC_ERR_FRM_PROC			0
    //#define SBC_FIRST_DISCARD_ENABLE    0    //discard overflow data avoid "pop"
    #define SBC_ENCODE_BUFFER_LEN       (SBC_MEM_POOL_MAX_NODE * SBC_FRAME_LENGTH_MAX) //15600  10800 //8192  
    #define SBC_MEM_POOL_MAX_NODE       153  //100 130
    #define SBC_FIRST_TRY_TIME          100 // 75
#else
    #define SBC_ERR_FRM_PROC			1
    #define SBC_FIRST_DISCARD_ENABLE    1    //discard overflow data avoid "pop"
    #ifdef CONFIG_APP_AEC
    #define SBC_ENCODE_BUFFER_LEN       (SBC_MEM_POOL_MAX_NODE * SBC_FRAME_LENGTH_MAX) // 10800 //8192
    #define SBC_MEM_POOL_MAX_NODE       153 //70
    #define SBC_FIRST_TRY_TIME           120 //40
    #else
    #define SBC_ENCODE_BUFFER_LEN       (SBC_MEM_POOL_MAX_NODE * SBC_FRAME_LENGTH_MAX) //10800 //4096
    #define SBC_MEM_POOL_MAX_NODE       130
    #define SBC_FIRST_TRY_TIME          105
    uint8 sbc_encode_buff[SBC_ENCODE_BUFFER_LEN];
    #endif
#endif

#define SBC_DEBUG

typedef struct _sbc_mem_node_s
{
    uint8 *data_ptr;
    struct _sbc_mem_node_s *next;
}sbc_mem_node_t;


typedef struct _sbc_mem_list_s
{
    sbc_mem_node_t *head;
    sbc_mem_node_t *tail;
}sbc_mem_list_t;

typedef struct _sbc_mem_pool_s
{
    sbc_mem_list_t freelist;
    sbc_mem_list_t uselist;
    int  node_num;
    uint32_t  has_node_cnt;
}sbc_mem_pool_t;

typedef struct _app_sbc_s
{
    uint8 *             sbc_encode_buffer; // for save SBC encoded NODE. not for save adc pcm raw data.
    uint8 *             sbc_output;     // for save decoded pcm data. not for save inverted pcm data in the function sbc_encode_mediapacket();


    sbc_mem_pool_t      sbc_mem_pool;
    sbc_t *             sbc_ptr; //for decode.
    uint32              freq;
    uint16              timer_cnt;
    uint8               sbc_target_initial;
    sbc_t *             sbc_encode_ptr; //for encode.
    uint8               sbc_first_try;
    uint8               sbc_clk_mode;

#ifdef BT_ONE_TO_MULTIPLE
    uint8               use_count;
#endif
#ifdef CONFIG_TWS
    int8               src_pkt_num;
    uint8				sync_flags;
#endif
    uint16				decode_cnt;
    uint16               sbc_ecout; /* the sbc_encoded_frame number in the avdtp_pkt before decode. - node number in the app_sbc.sbc_mem_pool.uselist */
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
    uint32	in_frame_len;
    uint32	out_frame_len;
    uint16	payload_len;
    uint16	frames_per_packet;
    uint16	sequence;
    uint32	samples;
    //uint8	SAMPLE_ALIGN sbc_input[512];
    uint8	*sbc_output_buffer;
    uint8 *adc_pcm_raw_buff; // for sbc encode. for save adc pcm raw data.
    uint8 *adc_pcm_mid_buff;     // for sbc encode. for save mid pcm data in the function sbc_encode_mediapacket();
#endif
}app_sbc_t;

#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
typedef struct rtp_header
{
    unsigned cc:4;
    unsigned x:1;
    unsigned p:1;
    unsigned v:2;

    unsigned pt:7;
    unsigned m:1;

    uint16_t sequence_number;
    uint32_t timestamp;
    uint32_t ssrc;
}rtp_header_t;

typedef struct media_packet_sbc
{
    rtp_header_t hdr;
    //	rtp_payload_t payload;//1byte
    uint8  frame_count;
    uint8  data[1];
}media_packet_sbc_t;

#endif

/******************
 * beken defination
 */
void sbc_target_init( sbc_t *sbc );
void sbc_mem_free(void);
void sbc_target_deinit( void );
void sbc_fill_encode_buffer( struct mbuf *m, int len, int frames );
#ifdef CONFIG_TWS
void sbc_fill_encode_buffer_tws(struct mbuf *m, int len, int frames, uint32 btclk);
#endif
void sbc_do_decode( void );
void sbc_stream_start_init( uint32 freq );
void sbc_mem_pool_init( int framelen );
void sbc_mem_pool_deinit( void );

void sbc_target_init_malloc_buff( void );//add by zjw
void sbc_target_deinit_jfree_buff(void);

void sbc_init_adjust_param(void);
void sbc_discard_uselist_node(void);

uint32 sbc_buf_get_use_count(void);
void sbc_buf_increase_use_count(void);
void sbc_buf_decrease_use_count(void);
uint16 sbc_buf_get_node_count(void);
uint8_t sbc_node_buff_monitor(void);
void sbc_dac_clk_tracking(uint32 step);
void *sbc_get_sbc_ptr(void);

void sbc_first_play_init(uint8 enable,uint8 cnt);

#ifdef CONFIG_TWS
#define FRAME_REDUCE_NUM   6
void sbc_src_num_oper( uint8 oper, uint8 num );
void sbc_notify_num( hci_link_t *link, uint8 num );
int sbc_src_num( void );
void clean_all_sbc_frame_node(void);
void sbc_stream_start_clear(void);
void sbc_src_num_set(uint8 num );
uint16 get_sbc_mem_pool_node_left(void);
uint8 get_sbc_clk_mode(void);
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
//RAM_CODE uint32 sbc_encode(sbc_t *sbc, const void *input, int input_len,
//			void *output, int output_len, int *written);
result_t RAM_CODE  sbc_do_encode( void );
void linein_sbc_encode_init(void);
void linein_sbc_alloc_free(void);
uint8 *get_linein_buffer_base(void);
uint32 get_linein_buffer_length(void);
void sbc_encode_buff_free(void); // for sbc encode.
#endif

extern int stereo_mode_slave_latency;
#endif

#ifdef CONFIG_SBC_PROMPT
void app_wave_file_sbc_fill(void);
//void app_wave_file_sbc_stop(void);

#endif
uint8 get_flag_sbc_buffer_play(void);
void set_flag_sbc_buffer_play(uint8 flag);

#ifdef CONFIG_TWS

#define SAMPLES_PER_SBC_FRAME       (128)

#define SAMPLES_PER_MSBC_FRAME       (120)

extern uint32 sample_num_per_sbc_frame(void);
extern uint32 pcm_bytes_per_sbc_frame(void);

#define OUT_PUT_LEN 512

RAM_CODE void sbc_mem_pool_freeNode( sbc_mem_list_t  *list,sbc_mem_node_t *node );
RAM_CODE sbc_mem_node_t * sbc_mem_pool_getNode( sbc_mem_list_t *list);
void sbc_mem_pool_free_first_node(void);

#define SBC_NODE_LEFT_NULL_ERR          0xFF
#define SBC_NODE_LEFT_NOT_STEREO        0x00
uint8 sbc_encode_buff_malloc(void);
uint8 sbc_decode_buff_malloc(void);
uint8 *get_ram_buff(void);
#endif
#ifdef __cplusplus
}
#endif

#endif /* __SBC_H */
