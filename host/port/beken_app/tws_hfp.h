/**
 **************************************************************************************
 * @file    tws_hfp.h
 * @brief   For hfp tws function.
 * 
 * @author  Jianjun.Ye
 * @version V1.0.0
 *
 * &copy; 2018-10-29 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __TWS_HFP_H__
#define __TWS_HFP_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus
#include <jos.h>
#include <bluetooth.h>
#include <bt_security.h>
#include <bt_sdp.h>
#include "app_beken_includes.h"
#include "bt_app_internal.h"
#include "app_sbc.h"

#include <jos/jos_mbuf.h>
#include <bt_l2cap_proto.h>
#ifdef CONFIG_TWS

//#define TWS_HFP_PKT_HEADER_LEN      8
//#define TWS_HFP_PKT_HEADER_LEN      9
#define TWS_HFP_PKT_HEADER_LEN      (10)

#define TWS_HFP_PKT_TYPE_CMD             (0XF0)
#define TWS_HFP_PKT_TYPE_MSBC_FRAME      (0XF1)
#define TWS_HFP_PKT_TYPE_CVSD_FRAME      (0xF2)

result_t tws_hfp_init(void);
void tws_hfp_connect(void);
result_t tws_hfp_cmd_connect(char *params, unsigned int len);
result_t tws_hfp_send_cmd(uint8_t *buff, uint32_t len);

void tws_hfp_recv_data_in_controller(uint8_t *buff, uint16_t len);

/* TWS_HFP FLAG DEFINE BEGIN */
#define TWS_HFP_FLAG_SCO_CONN                                           (0x01UL << 0)  
#define TWS_HFP_FLAG_DECODE_FIRST_MSBC                                  (0x01UL << 1)  
#define TWS_HFP_FLAG_HFACK_HAPPEN                                       (0x01UL << 2)  
#define TWS_HFP_FLAG_TAKE_MSBC_FRAME                                    (0x01UL << 3)  
#define TWS_HFP_FLAG_AUDIO_INIT                                         (0x01UL << 4)  
#define TWS_HFP_FLAG_AIRMODE_IS_CVSD                                    (0x01UL << 5)
#define TWS_HFP_FLAG_W4_SCO_CONNECT                                     (0x01UL << 6)
#define TWS_HFP_FLAG_STREAM_CONNECT                                     (0x01UL << 7)

/* TWS_HFP FLAG DEFINE END */

inline void set_tws_hfp_flag(uint32 flag);
inline void unset_tws_hfp_flag(uint32 flag);
inline uint32 get_tws_hfp_flag(uint32 flag);

//#define MSBC_MEM_POOL_MAX_NODE       (70 * 1) 
#define MSBC_MEM_POOL_MAX_NODE       (60 * 1) 
   
#define MSBC_FREAM_ENCODE_LEN           60

#if 1
#define MAX_MSBC_FREAM_ENCODE_CNT   2 /* MAX is 8. ->good_frame_bit_map is uint8 type.  */
#else
#define MAX_MSBC_FREAM_ENCODE_CNT   3
#endif

#define MAX_MSBC_FREAM_BUFF_LEN   ((MSBC_FREAM_ENCODE_LEN) * (MAX_MSBC_FREAM_ENCODE_CNT))

#define TWS_HFP_CMD_SCO_CONNECTED               0xB0
#define TWS_HFP_CMD_SCO_DISCONNECTED          0xB1
#define TWS_HFP_CMD_ACK                                    0xB2
#define TWS_HFP_CMD_REJECT                               0xB3
#define TWS_HFP_CMD_REDIAL                               0xB4
#define TWS_HFP_CMD_VOICE_DIAL                        0xB5
#define TWS_HFP_CMD_HF_TRANSFER                     0xB6

#define TWS_HFP_CMD_SET_HFP_VOLUME              0xB7

#if TWS_SCO_AVOID_MSBC_NODE_LEVEL_TOO_LOW_ENABLE
#define TWS_SCO_CMD_SET_BASE_MSBC_FREQ_ADJ_PPM              0xB8
#endif

#if TWS_A2DP_AVOID_SBC_NODE_LEVEL_TOO_LOW_ENABLE
#define TWS_A2DP_CMD_SET_BASE_SBC_FREQ_ADJ_PPM              0xB9
#endif

#ifdef CONFIG_BLUETOOTH_AVDTP_SCMS_T
#define TWS_A2DP_CMD_SET_CONTENT_PROTECT_FLAG              0xBA
#endif

typedef struct tws_hfp_frame_buff {
    uint8_t buf[MAX_MSBC_FREAM_BUFF_LEN];
    uint8_t frame_cnt;
    uint8_t good_frame_bit_map;
    uint8_t good_frame_cnt;
}__PACKED_POST__ tws_hfp_frame_buff_t;
result_t tws_hfp_frame_buff_send(const uint8_t *const buff, uint32_t const len, uint32_t const play_bt_clk);

#define TWS_HFP_LOWEST_MALLOC_MEM   (1024 * 2)
//#define TWS_HFP_LOWEST_MALLOC_MEM   (1024 * 1)

void tws_msbc_decode_prapare(void);
void tws_msbc_decode_task(void);
void tws_hfp_slave_msbc_decode_task(void);
void msbc_mem_pool_init(int framelen);
void msbc_mem_pool_deinit(void) ;

//inline sbc_mem_node_t *tws_hfp_get_first_msbc_node(void);
void tws_hfp_free_first_msbc_node(uint32 diff);

uint32 tws_hfp_get_msbc_node_left(void);

#define BAD_MSBC_FRAME  0x02 /* shall not be 0x01. cause good msbc frame has 60 bytes, its begin 3 bytes are 0x01 0x*8 0xAD. */
#define FILLED_MSBC_FRAME  0x03 

result_t tws_hfp_master_fill_msbc_frame(const uint8_t *const buff, uint32_t const len, uint32_t const play_bt_clk);
void tws_hfp_master_msbc_decode_task(void);

void tws_hfp_msbc_decode_init(void);
void tws_hfp_msbc_decode_deinit(void);

void tws_hfp_slave_prepare_sco_connected(uint8 freq);
void tws_hfp_slave_prepare_sco_disconnected(void);



#define TICKS_PER_SLOT                                     (2)          /* 1 tick = 312.5us, 1 slot = 2 ticks. */
#define MSBC_FRAME_OCCUPY_SLOTS                  (12)        /* 7.5ms */
#define TWS_HFP_DELAY_MSBC_FRAME_NUM       (8)         /* msbc frame numbers. = number * 7.5 ms. */
//#define TWS_HFP_DELAY_MSBC_FRAME_NUM       (5)         /* msbc frame numbers. = number * 7.5 ms. */
//#define TWS_HFP_DELAY_MSBC_FRAME_NUM       (7)         /* msbc frame numbers. = number * 7.5 ms. */
#define TWS_HFP_DELAY_PLAY   (TWS_HFP_DELAY_MSBC_FRAME_NUM*MSBC_FRAME_OCCUPY_SLOTS*TICKS_PER_SLOT)   /* tick number. */

#define BTCLK_OCCUPY_BYTES                            (4)          /* 4 bytes. */
//#define MSBC_CNT_OCCUPY_BYTES                     (4)          /* 4 bytes. */

#if TWS_HFP_ENABLE
#define MSBC_FREAM_PLAY_SYNC_ENABLE                      1
#else
#define MSBC_FREAM_PLAY_SYNC_ENABLE                      0
#endif
uint32_t tws_hfp_get_msbc_has_cnt(void);
void tws_hfp_avoid_sco_empty(void);
void tws_hfp_hfack_happen(void);
void tws_hfp_hfack_happen_timeout(void);
void tws_hfp_set_msbc_cnt(uint32 cnt);
uint32 tws_hfp_get_msbc_cnt(void);
uint32 tws_hfp_add_msbc_cnt(uint32 cnt);
uint32 tws_hfp_subtract_msbc_cnt(uint32 cnt);
void tws_hfp_check_msbc_cnt(uint32 cnt_of_master, uint32 cnt_in_pkt);
#if TWS_HFP_ENABLE
#define TWS_HFP_GET_MSBC_PER_7_5MS_ENABLE           1
#define TWS_HFP_GIVE_SLAVE_MORE_TX_CHANCE_ENABLE           1
#else
#define TWS_HFP_GET_MSBC_PER_7_5MS_ENABLE           0
#define TWS_HFP_GIVE_SLAVE_MORE_TX_CHANCE_ENABLE           0
#endif
result_t tws_hfp_sco_packet_input(const uint8_t *const buff, uint32_t const len);

#define DEBUG_FOR_MSBC_SYNC_ENABLE              1

#if TWS_SCO_AVOID_POP_ENABLE
uint32 tws_sco_get_avoid_pop_sound_cnt(void);
void tws_sco_avoid_pop_sound(void);
void tws_sco_avoid_pop_sound_cnt_chk(void);
#endif
void tws_hfp_w4_slave_reconnect(void);
#endif

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__DRV_MSG_H__
