/**
 **************************************************************************************
 * @file    tws_sco_sync.h
 * @brief   For hfp tws function.
 * 
 * @author  Jianjun.Ye
 * @version V1.0.0
 *
 * &copy; 2018-11-19 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef TWS_SCO_SYNC_H_
#define TWS_SCO_SYNC_H_

#include "types.h"
#include "config.h"
#include "bluetooth.h"
#include <jos/jos_mbuf.h>
#include "app_sbc.h"

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#if TWS_HFP_ENABLE

/* TWS_SCO_SYNC FLAG DEFINE BEGIN */
#define TWS_FLAG_                     (0x01UL << 0) //0x00000001 

#if TWS_SCO_AVOID_MSBC_NODE_LEVEL_TOO_LOW_ENABLE
#define TWS_FLAG_BASE_MSBC_FREQ_ADJ_PPM_BUSY           (0x01UL << 1) // only for TWS_M.
#endif

extern void set_tws_sco_sync_flag(uint32 flag);
extern void unset_tws_sco_sync_flag(uint32 flag);
extern uint32 get_tws_sco_sync_flag(uint32 flag);
/* TWS FLAG DEFINE END */



/******************************************************** 
For tws sync_mechanism.
Begin:
********************************************************/


// TWS-S SYNC MACRO begin.
#if 1
#define AMPLYFY_TIMES  1

#define M_DAC_SYNC_SLAVE_TH1        (1 * AMPLYFY_TIMES)  // 1 ticks = 0.3125 ms 
#define M_DAC_SYNC_SLAVE_TH2        (1 * AMPLYFY_TIMES)  //           0.3125 ms
#define M_DAC_SYNC_SLAVE_TH3        (2 * AMPLYFY_TIMES)  //           0.625  ms
#define M_DAC_SYNC_SLAVE_TH4        (3 * AMPLYFY_TIMES)  // 3 ticks = 0.9375 ms 
#endif

#if 1 // MAX ADJUST 100ppm

//#define DAC_ADJ_AMPLYFY_TIMES  5
#define DAC_ADJ_AMPLYFY_TIMES  4 // 2 // for msbc
//#define DAC_ADJ_AMPLYFY_TIMES  6 // 2

#define M_DAC_SYNC_DAC_ADJUST_PPM_1 (25 * DAC_ADJ_AMPLYFY_TIMES)
#define M_DAC_SYNC_DAC_ADJUST_PPM_2 (35 * DAC_ADJ_AMPLYFY_TIMES)
#define M_DAC_SYNC_DAC_ADJUST_PPM_3 (75 * DAC_ADJ_AMPLYFY_TIMES)
#define M_DAC_SYNC_DAC_ADJUST_PPM_4 (100 * DAC_ADJ_AMPLYFY_TIMES)

#define M_TWS_SEC_DAC_FREQ_441K_MAX       (77283736) //((AUDIO_DIV_441K + ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 + 0x049B2368 * 100 / 1000000
// 77283735.6008

#define M_TWS_SEC_DAC_FREQ_441K_MIN       (77268280) //((AUDIO_DIV_441K - ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 - 0x049B2368 * 100 / 1000000
// 77268280.3992

#define M_TWS_SEC_DAC_FREQ_48K_MAX        (71004432) //((AUDIO_DIV_48K + ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 + 0x043B5554 / 1000000 * 100
// 71004431.7332

#define M_TWS_SEC_DAC_FREQ_48K_MIN        (70990232) //((AUDIO_DIV_48K - ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 - 0x043B5554 / 1000000 * 100
// 70990232.2668

#else // MAX ADJUST 200ppm

#define M_DAC_SYNC_DAC_ADJUST_PPM_1 (50)
#define M_DAC_SYNC_DAC_ADJUST_PPM_2 (80)
#define M_DAC_SYNC_DAC_ADJUST_PPM_3 (150)
#define M_DAC_SYNC_DAC_ADJUST_PPM_4 (200)

#define M_TWS_SEC_DAC_FREQ_16K_MAX       (106517299) /* hex2dec('6590000') + hex2dec('6590000')*200/1000000, [format long g] */
#define M_TWS_SEC_DAC_FREQ_16K_MIN       (106474700) /* hex2dec('6590000') - hex2dec('6590000')*200/1000000 */

//#define TWS_SEC_DAC_FREQ_441K_MAX       (0x49F4198) 
#define M_TWS_SEC_DAC_FREQ_441K_MAX       (77291463) //((AUDIO_DIV_441K + ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 + 0x049B2368 * 200 / 1000000
// 77291463.2016

#define M_TWS_SEC_DAC_FREQ_441K_MIN       (77260553) //((AUDIO_DIV_441K - ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 - 0x049B2368 * 200 / 1000000
// 77260552.7984

#define M_TWS_SEC_DAC_FREQ_48K_MAX        (71011531) //((AUDIO_DIV_48K + ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 + 0x043B5554 / 1000000 * 200
// 71011531.4664

#define M_TWS_SEC_DAC_FREQ_48K_MIN        (70983133) //((AUDIO_DIV_48K - ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 - 0x043B5554 / 1000000 * 200
// 70983132.5336
#endif
// TWS-S SYNC MACRO end.


// TWS-M SYNC MACRO begin.
#define M_SBC_NODE_SYNC_MASTER_LEVEL     (SBC_MEM_POOL_MAX_NODE / 2)
#define M_SBC_NODE_SYNC_MASTER_TH1       (SBC_NODE_SYNC_MASTER_LEVEL * 25 / 100)		// 13, 38-64
#define M_SBC_NODE_SYNC_MASTER_TH2       (SBC_NODE_SYNC_MASTER_LEVEL * 30 / 100)		// 15
#define M_SBC_NODE_SYNC_MASTER_TH3       (SBC_NODE_SYNC_MASTER_LEVEL * 35 / 100)	    // 18
#define M_SBC_NODE_SYNC_MASTER_TH4       (SBC_NODE_SYNC_MASTER_LEVEL * 40 / 100)	    // 20, 31-71

#if 1
#define M_TWS_PRI_DAC_ADJ_STEP1_PPM    (5)
#define M_TWS_PRI_DAC_ADJ_STEP2_PPM    (10)
#define M_TWS_PRI_DAC_ADJ_STEP3_PPM    (15)
#define M_TWS_PRI_DAC_ADJ_STEP4_PPM    (20)
#endif

#if 1 // MAX ADJUST 100ppm
#define M_TWS_PRI_DAC_FREQ_441K_MAX       (77283736) //((AUDIO_DIV_441K + ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 + 0x049B2368 * 100 / 1000000
// 77283735.6008

#define M_TWS_PRI_DAC_FREQ_441K_MIN       (77268280) //((AUDIO_DIV_441K - ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 - 0x049B2368 * 100 / 1000000
// 77268280.3992

#define M_TWS_PRI_DAC_FREQ_48K_MAX        (71004432) //((AUDIO_DIV_48K + ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 + 0x043B5554 / 1000000 * 100
// 71004431.7332

#define M_TWS_PRI_DAC_FREQ_48K_MIN        (70990232) //((AUDIO_DIV_48K - ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 - 0x043B5554 / 1000000 * 100
// 70990232.2668

#else // MAX ADJUST 200ppm

#define M_TWS_PRI_DAC_FREQ_16K_MAX       (106517299) /* hex2dec('6590000') + hex2dec('6590000')*200/1000000, [format long g] */
#define M_TWS_PRI_DAC_FREQ_16K_MIN       (106474700) /* hex2dec('6590000') - hex2dec('6590000')*200/1000000 */

//#define TWS_PRI_DAC_FREQ_441K_MAX       (0x49F4198) 
#define M_TWS_PRI_DAC_FREQ_441K_MAX       (77291463) //((AUDIO_DIV_441K + ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 + 0x049B2368 * 200 / 1000000
// 77291463.2016

#define M_TWS_PRI_DAC_FREQ_441K_MIN       (77260553) //((AUDIO_DIV_441K - ((AUDIO_DIV_441K * NODE_SYNC_PPM_4) / 1000000)))
//0x049B2368 - 0x049B2368 * 200 / 1000000
// 77260552.7984

#define M_TWS_PRI_DAC_FREQ_48K_MAX        (71011531) //((AUDIO_DIV_48K + ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 + 0x043B5554 / 1000000 * 200
// 71011531.4664

#define M_TWS_PRI_DAC_FREQ_48K_MIN        (70983133) //((AUDIO_DIV_48K - ((AUDIO_DIV_48K * NODE_SYNC_PPM_4) / 1000000)))
//0x043B5554 - 0x043B5554 / 1000000 * 200
// 70983132.5336
#endif

// TWS-M SYNC MACRO end.

boolean tws_sco_sync_stage_one(void);
boolean tws_sco_sync_stage_two(void);
boolean tws_sco_sync_dac_clk_adjust(uint32 msbc_frame_out_btclk);
void tws_sco_sync_engine(int32 const diff_level, int32 const direction);

int32 tws_sco_sync_jump_level(int32 const diff_level, int32_t const cur_direction);
int32 tws_sco_sync_above_last_diff_1(int32 last_diff_1);
int32 tws_sco_sync_below_last_diff_1(int32 last_diff_1);

/********************************************************
For tws sync_mechanism.
End
********************************************************/

#endif /* CONFIG_TWS */


//#define NO_SYNC_BTCLK       0xFFFFFFFF

#define FREQ_OF_MSBC_ON_ESCO       (16000)

uint32_t tws_sco_sync_calc_msbc_frame_left_time(void);

uint32_t tws_sco_sync_calc_pcm_left_time(void);

uint32_t tws_sco_calc_msbc_play_time(void);


#define FIRST_MSBC_PLAY_DELAY_TICKS       (320)   /* 320 X 312.5us = 100ms. */
#define MSBC_FREAM_LEVEL_LOWEST              (15)   

void tws_sco_sync_set_is_first_msbc_frame_node(void);

#if TWS_SCO_AVOID_MSBC_NODE_LEVEL_TOO_LOW_ENABLE
uint32 tws_sco_base_msbc_div(void);

int32 tws_sco_base_msbc_freq_adj_ppm_get(void);

/***************************************************
*  It's only for TWS_S.
****************************************************/
void tws_slave_sco_base_msbc_freq_adj_ppm_set(int32 base_msbc_freq_adj_ppm);


/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_sco_send_base_msbc_freq_adj_ppm_to_twsS(void);

/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_sco_base_msbc_freq_make_self_slow(void);

/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_sco_base_msbc_freq_make_self_fast(void);

/***************************************************
*  It's only for TWS_M.
****************************************************/
void tws_master_sco_base_msbc_freq_adj_ppm_set(int const ppm);

void tws_sco_base_msbc_freq_adj_enter(void);
void tws_sco_base_msbc_freq_adj_chk(void);

#define TWS_SCO_MSBC_BASE_16K_ADJ_ENABLE     1

#if TWS_SCO_MSBC_BASE_16K_ADJ_ENABLE
uint32 tws_sco_msbc_base_16k(void);

#endif

#endif

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif /* TWS_H_ */

