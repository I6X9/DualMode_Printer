/**
 **************************************************************************************
 * @file    transparent_l2cap.h
 * @brief   For hfp tws function.
 * 
 * @author  Jianjun.Ye
 * @version V1.0.0
 *
 * &copy; 2018-10-29 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __TRANSPARENT_L2CAP_H__
#define __TRANSPARENT_L2CAP_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include <jos.h>
#include <bluetooth.h>
#include <bt_security.h>
#include <bt_sdp.h>
#include "app_beken_includes.h"
#include "bt_app_internal.h"

#include <jos/jos_mbuf.h>
#include <bt_l2cap_proto.h>
#include "bt_mini_sched.h"
#ifdef CONFIG_TWS

#define TRANS_L2CAP_HEADER_LEN      1

#define TRANS_L2CAP_FLAG_IN_USE                     (0x01UL << 0)  /* tansparent_l2cap is in use or not. */ 

typedef struct l2cap_control{
u_int8 device_index;
u_int16 l2cap_chn_id_local; // local l2cap channel id.
u_int16 l2cap_chn_id_remote; // remote l2cap channel id.
} l2cap_control_t;

struct tansparent_l2cap_conn;

typedef struct tansparent_l2cap_server {
    uint32_t                 flag;

    l2cap_channel_h         chan_l;
    l2cap_channel_h         chan_r;
    btaddr_t                l_btaddr;
    btaddr_t                r_btaddr;
    struct tansparent_l2cap_conn *tansparent_l2cap_con; 

    const struct btproto        *st_proto;
    
    l2cap_control_t tl_l2cap_ctl;
} tansparent_l2cap_server_t;

typedef struct tansparent_l2cap_conn {
    l2cap_channel_h         chan_tansparent_l2cap;
    const struct btproto 	*proto;
    struct tansparent_l2cap_server       *srv;
} tansparent_l2cap_conn_t;

typedef struct tansparent_l2cap_hdr {
    uint8_t msg_param:4;
    uint8_t msg_type:4;
} __PACKED_POST__ tansparent_l2cap_hdr_t;

inline void set_trans_l2cap_flag(uint32 flag);
inline void unset_trans_l2cap_flag(uint32 flag);
inline uint32 get_trans_l2cap_flag(uint32 flag);

result_t tansparent_l2cap_init(void);
void tansparent_l2cap_stream_alloc(const struct btproto *proto);
void tansparent_l2cap_uninit(void);
//void tansparent_l2cap_conn_close(tansparent_l2cap_conn_t *tansparent_l2cap_conn);

result_t tansparent_l2cap_connect(btaddr_t *laddr, btaddr_t *raddr);
result_t tansparent_l2cap_conn_disconnect(void);

result_t send_tansparent_l2cap_data(uint8_t *buf, int len);

void tansparent_l2cap_set_device_index(uint8_t d_idx);
uint8_t tansparent_l2cap_get_device_index(void);

#define DATA4_IS_TWS_M_SCO_DATA  0xA0

#define DATA5_TWS_M_SCO_CONN  0xB0
#define DATA5_TWS_M_SCO_DISCONN  0xB1

void tansparent_l2cap_send_data_in_controller(uint8_t *buff, uint16_t len, uint8_t tws_sco_state);
void tansparent_l2cap_recv_data_in_controller(uint8_t *buff, uint16_t len);

uint16 get_tansparent_l2cap_lcid(void);
uint16 get_tansparent_l2cap_rcid(void);

uint32 get_tansparent_l2cap_txq_num(void);

result_t tansparent_l2cap_stream_flow_control(struct l2cap_channel *chan, uint32_t max_num);
#endif

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__DRV_MSG_H__
