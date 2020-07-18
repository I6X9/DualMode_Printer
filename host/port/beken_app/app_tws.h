/*
 * app_tws.h
 *
 *  Created on: 2017-12-20
 *      Author: jianjun.ye
 */

#ifndef TWS_H_
#define TWS_H_
#include <string.h>
#include "types.h"
#include "config.h"
#include "bluetooth.h"
#include <jos/jos_mbuf.h>
#include "app_sbc.h"

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#ifdef CONFIG_TWS

#define TWS_PRIM_SEC_UNDEFINED  1
#define TWS_PRIM_SEC_PRIMARY    2
#define TWS_PRIM_SEC_SECOND     3//slave

#define TWS_POWER_ON_TO         5 /* unit: about 10ms. */
#define TWS_ENTER_MATCH_TO      400 /* unit: about 10ms. */

#define TWS_PAGE_TIME_INTERVAL	7000
typedef struct _app_tws_s
{
    uint8 role;
    uint8 piconet_role;
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE	
    uint8 linein;
#endif
    uint8 shareme;
} APP_TWS_T, *app_tws_t;

void tws_launch_diac_pairing(void);
void tws_launch_diac_scan(void);

extern uint8 get_tws_prim_sec(void);
void set_tws_prim_sec(uint8 t_p_s);

boolean get_tws_launch_giac_pairing(void);
void set_tws_launch_giac_pairing(boolean tws_giac_pairing);

boolean is_peer_tws(const btaddr_t * const raddr_peer);
boolean is_peer_tws_primary(const btaddr_t * const raddr_peer);

void tws_status_show(void);
void tws_launch_auto_connect(void);

void tws_stereo_acl_disconn_wrap(uint8 reason, btaddr_t *raddr);
void tws_page_time_out_cb(void *btaddr);

#define BT_CLK_TICKS_PER_SECOND     (3200)

extern uint8 tws_piconet_role_switch_decision(void *bd_addr);

boolean tws_judge_piconet_role(void *peer_addr);

void tws_power_down(void);
#if (TWS_PAIR_TIMEOUT_ENABLE == 1)
void tws_pairing_timeout(void);
#endif

/* TWS FLAG DEFINE BEGIN */
#define TWS_FLAG_POWER_DOWN                     (0x01UL << 0) //0x00000001 

#define TWS_FLAG_SYNC_BY_OS_TIMER               (0x01UL << 1) //0x00000002

#define TWS_FLAG_NEED_FREE_MEMORY               (0x01UL << 2) //0x00000004
    
#define TWS_FLAG_FLUSH_SLAVE_BUFF               (0x01UL << 3) //0x00000008

#define TWS_FLAG_SLAVE_PREPARE_RE_SYNC			(0x01UL << 4) //     for TWS_S.
#define TWS_FLAG_SLAVE_STARTED_RE_SYNC          (0x01UL << 5) //    for TWS_S.

#define TWS_FLAG_MASTER_STARTED_RE_SYNC			(0x01UL << 6) // for TWS_M.
#define TWS_FLAG_MASTER_PREPARE_RE_SYNC			(0x01UL << 7) // for TWS_M.

#define TWS_FLAG_MASTER_LINEIN_CLOSE_ENCODE     (0x01UL << 8) // for TWS_M in LINEIN mode.

#define TWS_FLAG_MASTER_NEED_CONNECT_BACK_PHONE     (0x01UL << 9) // for TWS_M connect back phone.

#define TWS_FLAG_NEED_LAUNCH_TWS_PAIRING     	(0x01UL << 10) // for TWS PAIRING

#define TWS_FLAG_CONNECTING_SLAVE     			(0x01UL << 11)
#define TWS_FLAG_LAUNCH_TWS_INQUIRY_AND_PAGE    (0x01UL << 12) // for TWS CONNECTING
#define TWS_FLAG_STEREO_PAIRING    				(0x01UL << 13)

#define TWS_FLAG_TWS_ROLE_SWITCH_FAIL           (0x01UL << 14) // for TWS PAIRING FAIL.
#if TWS_A2DP_AVOID_SBC_NODE_LEVEL_TOO_LOW_ENABLE
#define TWS_FLAG_BASE_SBC_FREQ_ADJ_PPM_BUSY           (0x01UL << 15) // only for TWS_M.
#endif
#ifdef CONFIG_BLUETOOTH_AVDTP_SCMS_T
#define TWS_FLAG_A2DP_CONTENT_PROTECT                       (0x01UL << 16) // only for TWS_S.
#endif

#define TWS_FLAG_W4_SLAVE_CONNECT                       (0x01UL << 17)
#define TWS_FLAG_W4_SLAVE_AVDTP_START_RSP                     (0x01UL << 18)
#define TWS_FLAG_W4_SLAVE_AVDTP_SUSPEND_RSP                     (0x01UL << 19)
#define TWS_FLAG_MASTER_SLAVE_A2DP_STREAM_CONNECTED     			(0x01UL << 20)


#define TWS_W4_SLAVE_CONNECT_TIMEOUT                    7000 //ms

#define TWS_SYNC_BY_OS_TIMER                0

extern void set_tws_flag(uint32 flag);
extern void unset_tws_flag(uint32 flag);
extern uint32 get_tws_flag(uint32 flag);

extern void tws_set_free_mem_flag(void);
extern void tws_unset_free_mem_flag(void);
extern uint32 tws_get_free_mem_flag(void);

/* TWS FLAG DEFINE END */

void tws_slave_page(void);

/******************************************************** 
For tws sync_mechanism.
Begin:
********************************************************/

#define TWS_SECOND_SYNC_WITH_PRIMARY_ENABLE        1  //
#define TWS_PRIMARY_SYNC_WITH_PHONE_ENABLE         0  //

#define TWS_PRIMARY_DELAY         0  //

//#define TWS_PRIMARY_DELAY       (-7)  // TWS_SECOND delay 2ms = 6.4 ticks.
//#define TWS_PRIMARY_DELAY         (-4)  //

// TWS-S SYNC MACRO begin.
#if 1
#define DAC_SYNC_SLAVE_TH1        (1)  // 1 ticks = 0.3125 ms 
#define DAC_SYNC_SLAVE_TH2        (1)  //           0.3125 ms
#define DAC_SYNC_SLAVE_TH3        (2)  //           0.625  ms
#define DAC_SYNC_SLAVE_TH4        (3)  // 3 ticks = 0.9375 ms 
#endif

#if 1 // MAX ADJUST 100ppm
#define ADJ_AMPLYFY_TIMES  4  // 6 // 2  // for a2dp
#define DAC_SYNC_DAC_ADJUST_PPM_1 (25 * ADJ_AMPLYFY_TIMES)
#define DAC_SYNC_DAC_ADJUST_PPM_2 (35 * ADJ_AMPLYFY_TIMES)
#define DAC_SYNC_DAC_ADJUST_PPM_3 (75 * ADJ_AMPLYFY_TIMES)
#define DAC_SYNC_DAC_ADJUST_PPM_4 (100 * ADJ_AMPLYFY_TIMES)

#define TWS_SEC_DAC_FREQ_441K_MAX       (77283736) //((AUDIO_DIV_441K + ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 + 0x049B2368 * 100 / 1000000
// 77283735.6008

#define TWS_SEC_DAC_FREQ_441K_MIN       (77268280) //((AUDIO_DIV_441K - ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 - 0x049B2368 * 100 / 1000000
// 77268280.3992

#define TWS_SEC_DAC_FREQ_48K_MAX        (71004432) //((AUDIO_DIV_48K + ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 + 0x043B5554 / 1000000 * 100
// 71004431.7332

#define TWS_SEC_DAC_FREQ_48K_MIN        (70990232) //((AUDIO_DIV_48K - ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 - 0x043B5554 / 1000000 * 100
// 70990232.2668

#else // MAX ADJUST 200ppm

#define DAC_SYNC_DAC_ADJUST_PPM_1 (50)
#define DAC_SYNC_DAC_ADJUST_PPM_2 (80)
#define DAC_SYNC_DAC_ADJUST_PPM_3 (150)
#define DAC_SYNC_DAC_ADJUST_PPM_4 (200)

//#define TWS_SEC_DAC_FREQ_441K_MAX       (0x49F4198) 
#define TWS_SEC_DAC_FREQ_441K_MAX       (77291463) //((AUDIO_DIV_441K + ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 + 0x049B2368 * 200 / 1000000
// 77291463.2016

#define TWS_SEC_DAC_FREQ_441K_MIN       (77260553) //((AUDIO_DIV_441K - ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 - 0x049B2368 * 200 / 1000000
// 77260552.7984

#define TWS_SEC_DAC_FREQ_48K_MAX        (71011531) //((AUDIO_DIV_48K + ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 + 0x043B5554 / 1000000 * 200
// 71011531.4664

#define TWS_SEC_DAC_FREQ_48K_MIN        (70983133) //((AUDIO_DIV_48K - ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 - 0x043B5554 / 1000000 * 200
// 70983132.5336
#endif
// TWS-S SYNC MACRO end.


// TWS-M SYNC MACRO begin.
#define SBC_NODE_SYNC_MASTER_LEVEL     (SBC_MEM_POOL_MAX_NODE / 2)
#define SBC_NODE_SYNC_MASTER_TH1       (SBC_NODE_SYNC_MASTER_LEVEL * 25 / 100)		// 13, 38-64
#define SBC_NODE_SYNC_MASTER_TH2       (SBC_NODE_SYNC_MASTER_LEVEL * 30 / 100)		// 15
#define SBC_NODE_SYNC_MASTER_TH3       (SBC_NODE_SYNC_MASTER_LEVEL * 35 / 100)	    // 18
#define SBC_NODE_SYNC_MASTER_TH4       (SBC_NODE_SYNC_MASTER_LEVEL * 40 / 100)	    // 20, 31-71

#if 1
#define TWS_PRI_DAC_ADJ_STEP1_PPM    (5)
#define TWS_PRI_DAC_ADJ_STEP2_PPM    (10)
#define TWS_PRI_DAC_ADJ_STEP3_PPM    (15)
#define TWS_PRI_DAC_ADJ_STEP4_PPM    (20)
#endif

#if 1 // MAX ADJUST 100ppm
#define TWS_PRI_DAC_FREQ_441K_MAX       (77283736) //((AUDIO_DIV_441K + ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 + 0x049B2368 * 100 / 1000000
// 77283735.6008

#define TWS_PRI_DAC_FREQ_441K_MIN       (77268280) //((AUDIO_DIV_441K - ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 - 0x049B2368 * 100 / 1000000
// 77268280.3992

#define TWS_PRI_DAC_FREQ_48K_MAX        (71004432) //((AUDIO_DIV_48K + ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 + 0x043B5554 / 1000000 * 100
// 71004431.7332

#define TWS_PRI_DAC_FREQ_48K_MIN        (70990232) //((AUDIO_DIV_48K - ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 - 0x043B5554 / 1000000 * 100
// 70990232.2668

#else // MAX ADJUST 200ppm

//#define TWS_PRI_DAC_FREQ_441K_MAX       (0x49F4198) 
#define TWS_PRI_DAC_FREQ_441K_MAX       (77291463) //((AUDIO_DIV_441K + ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 + 0x049B2368 * 200 / 1000000
// 77291463.2016

#define TWS_PRI_DAC_FREQ_441K_MIN       (77260553) //((AUDIO_DIV_441K - ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 - 0x049B2368 * 200 / 1000000
// 77260552.7984

#define TWS_PRI_DAC_FREQ_48K_MAX        (71011531) //((AUDIO_DIV_48K + ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 + 0x043B5554 / 1000000 * 200
// 71011531.4664

#define TWS_PRI_DAC_FREQ_48K_MIN        (70983133) //((AUDIO_DIV_48K - ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 - 0x043B5554 / 1000000 * 200
// 70983132.5336
#endif
// TWS-M SYNC MACRO end.


#if 0
#define TWS_SEC_DAC_FREQ_441K_MAX       (77430560) //((AUDIO_DIV_441K + ((AUDIO_DIV_441K * DAC_SYNC_DAC_ADJUST_PPM_4) / 1000000)))
//0x049B2368 + 0x049B2368 * 2000 / 1000000
// 77430560.016

#define TWS_SEC_DAC_FREQ_441K_MIN       (77121456) //((AUDIO_DIV_441K - ((AUDIO_DIV_441K * DAC_SYNC_DAC_ADJUST_PPM_4) / 1000000)))
//0x049B2368 - 0x049B2368 * 2000 / 1000000
// 77121455.984


#define TWS_SEC_DAC_FREQ_48K_MAX        (71139327) //((AUDIO_DIV_48K + ((AUDIO_DIV_48K * DAC_SYNC_DAC_ADJUST_PPM_4) / 1000000)))
//0x043B5554 + 0x043B5554 / 1000000 * 2000
// 71139326.664

#define TWS_SEC_DAC_FREQ_48K_MIN        (70855337) //((AUDIO_DIV_48K - ((AUDIO_DIV_48K * DAC_SYNC_DAC_ADJUST_PPM_4) / 1000000)))
//0x043B5554 - 0x043B5554 / 1000000 * 2000
// 70855337.336

#endif

void tws_clear_a2dp_buffer(void);

extern void tws_set_call_exist(boolean exist);
extern boolean tws_get_call_exist(void);

#define TWS_ENABLE_SYNC_ENGINE 1

#define AVERAGE(a, b)  (((a)+(b)) >> 1)

extern app_tws_t app_get_tws_handler(void);

#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
void app_bt_slave_linein_set(uint8 value);
uint8 app_bt_slave_linein_get(void);
#endif

//void app_tws_processing(void);

/********************************************************
For tws sync_mechanism.
End
********************************************************/


#define TWS_SLAVE_SUPERVISION_TIMEOUT          1638  // 1638 * 625us  = 1023750 us 

#define TWS_MEM_FOR_SLAVE_LOWEST                (1024 * 3) // 3k memory is the lowest, cann't lower than 3k free memory.
//#define TWS_MEM_FOR_SLAVE_LOWEST                (1024 * 4) 
//#define TWS_MEM_FOR_SLAVE_LOWEST                (1024 * 5) 

#define TWS_M_RE_SYNC_DELAY_TIME         400 //MS

void tws_master_linein_encode_open(void);
void tws_master_linein_encode_close(void);

// void tws_slave_del_pkt_check(void);


#endif /* CONFIG_TWS */

#define ADD_TO_EIR_UUID_NUM    (1)
#define TWS_UUID    (0xA789)

uint8 *tws_get_beken_info_in_eir_data(void);

#define NO_SYNC_BTCLK       0xFFFFFFFF

#define TWS_SBC_MAX_BITPOOL     48
//#define TWS_SBC_MAX_BITPOOL     32
//#define TWS_SBC_MAX_BITPOOL     16

uint8 tws_sbc_max_bitpool_set(void);
//inline boolean tws_is_filled_sbc_node(sbc_mem_node_t *node);
void tws_stereo_autoconn_cnt_set(uint8 value);
uint8 tws_stereo_autoconn_cnt_get(void);

#define TWS_DEV_DEBUG  0

#define TWS_A2DP_RE_SYNC_ENABLE  0

#define TWS_INQUIRY_AND_PAGE_TIMEOUT           (2)   // unit: second. 10 seconds

uint8 tws_get_retry_count(void);
void tws_increase_retry_count(void);
void tws_set_retry_count(uint8 cnt);

void tws_slave_conn_back_stage_two(void);
void tws_slave_prepare_conn_back_stage_two(void);
uint8 tws_hci_eir_type_beken(uint8 type);

#define TWS_IN_EIR    				0x68
#define HCI_EIR_TYPE_BEKEN			0xFF
#define HCI_EIR_TYPE_BEKEN_SIZE     6      /* BEKEN VENDOR DEPENDENT TYPE SIZE*/

#if TWS_A2DP_AVOID_POP_ENABLE
uint32 tws_a2dp_get_avoid_pop_sound_cnt(void);
void tws_a2dp_avoid_pop_sound(void);
void tws_a2dp_avoid_pop_sound_cnt_chk(void);
#endif

#if TWS_A2DP_AVOID_SBC_NODE_LEVEL_TOO_LOW_ENABLE

uint32 tws_a2dp_base_sbc_dac_div(void);

int32 tws_a2dp_base_sbc_freq_adj_ppm_get(void);

/***************************************************
*  It's only for TWS_S.
****************************************************/
void tws_slave_a2dp_base_sbc_freq_adj_ppm_set(int32 base_sbc_freq_adj_ppm);

/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_a2dp_send_base_sbc_freq_adj_ppm_to_twsS(void);

/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_a2dp_base_sbc_freq_make_self_slow(void);

/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_a2dp_base_sbc_freq_make_self_fast(void);

/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_master_a2dp_base_sbc_freq_adj_ppm_set(int const ppm);


void tws_a2dp_base_sbc_freq_adj_enter(void);
void tws_a2dp_base_sbc_freq_adj_chk(void);

#endif


#ifdef CONFIG_BLUETOOTH_AVDTP_SCMS_T
/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_a2dp_send_content_protect_flag_to_twsS(uint8 flag);
#endif



#ifdef  __cplusplus
}
#endif//__cplusplus

#endif /* TWS_H_ */

