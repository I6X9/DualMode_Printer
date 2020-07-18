/* Jungo Confidential, Copyright (c) 2012 Jungo Ltd.  http://www.jungo.com */
#ifndef __CONFIG_H__
#define __CONFIG_H__
#include "config_debug.h"
#include "version.h"

#define CONFIG_ARM_COMPILER                         1
#define CONFIG_BYTE_ORDER                       CPU_LITTLE_ENDIAN
#define CONFIG_PORT                             beken_no_os
#define CONFIG_SINGLE_THREADED                   1
#define CONFIG_NATIVE_UINT64                        1
#define CONFIG_JOS_MBUF                                 1
#define CONFIG_JOS_BUS                          1
#define CONFIG_JOS_UTILS                        1
#define CONFIG_JOS_SECURE_PTR                   1
#define CONFIG_COMMON_STR_UTILS                 1
#define CONFIG_MEMPOOL                          1
#define CONFIG_MEMPOOL_DMABLE                   1
#define CONFIG_BLUETOOTH                        1
#define CONFIG_BLUETOOTH_HCI_UART               1
#define CONFIG_BLUETOOTH_SDP_SERVER             1
#define CONFIG_BLUETOOTH_A2DP                   1
#define CONFIG_BLUETOOTH_A2DP_SINK              1
#define CONFIG_BLUETOOTH_AVRCP                  1
#define CONFIG_BLUETOOTH_AVRCP_CT               1
#define CONFIG_BLUETOOTH_AVRCP_TG              1
#define CONFIG_BLUETOOTH_AVDTP                  1
#define CONFIG_BLUETOOTH_AVDTP_SCMS_T     1
#define CONFIG_BLUETOOTH_AVCTP                  1
#define CONFIG_BLUETOOTH_HFP                    1
#define CONFIG_BLUETOOTH_SDP_HFP                1
#define CONFIG_BLUETOOTH_HSP                    1
#define CONFIG_BLUETOOTH_SDP_HSP                1
#define CONFIG_BLUETOOTH_SPP                    1
#define CONFIG_BLUETOOTH_SCO                    1
#define CONFIG_BLUETOOTH_APP                    1
#define CONFIG_BLUETOOTH_AUDIO_APP_STANDALONE   1
#define CONFIG_AUDIO_OUT_INTERFACE              1
#define CONFIG_PKG                              1
#define CONFIG_PKG_SBC                          1
#define CONFIG_FILE_LIST                        file_list_beken
#define UWVER_STR                               "4.0.33.5"
#define CONFIG_APP_MP3PLAYER                    1
#define CONFIG_AUDIO_TRANSFER_DMA               1
#define LOWBAT_DET_ENABLE						 1

#define CONFIG_DEBUG_PCM_TO_UART                   0
#define CONFIG_CPU_CLK_OPTIMIZATION               1
#define CONFIG_APP_AEC                          1

#define CONFIG_BLUETOOTH_SSP
#define CONFIG_BLUETOOTH_HID
/* #define CONFIG_DRIVER_I2S */
#define CONFIG_DRIVER_DAC
//#define CONFIG_DRIVER_ADC
#define CONFIG_CHARGE_EN						1

#define CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE      1
#define CONFIG_UART_IN_SNIFF					0

//注意： 音箱方案配置时请将以下宏CONFIG_AUDIO_MIC_DIFF_EN/CONFIG_EXT_AUDIO_PA_EN打开， SYS_CFG_BUCK_ON关闭

#define CONFIG_TX_CALIBRATION_EN                1

#define CHIP_PACKAGE_TSSOP_28                     0
#if (CHIP_PACKAGE_TSSOP_28 == 1)
#define CONFIG_AUDIO_DAC_ALWAYSOPEN			1
#define CONFIG_AUDIO_DAC_RAMP_EN				0
#define CONFIG_AUDIO_MIC_DIFF_EN				1
#define CONFIG_EXT_AUDIO_PA_EN   				1
#define CONFIG_EXT_PA_DIFF_EN                              1
#define SYS_CFG_BUCK_ON                                         0
#define BT_ONE2MULTIPLE_AS_SCATTERNET		1
#ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    #undef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
#endif
#else
#define BT_ONE2MULTIPLE_AS_SCATTERNET		0
#define CONFIG_AUDIO_DAC_ALWAYSOPEN			0
#define CONFIG_AUDIO_DAC_RAMP_EN				1
#define CONFIG_AUDIO_MIC_DIFF_EN				0
#define CONFIG_EXT_AUDIO_PA_EN   				1
#define CONFIG_EXT_PA_DIFF_EN                              0
#define SYS_CFG_BUCK_ON                                         1
#endif//CHIP_PACKAGE_TSSOP_28

#if (CONFIG_CHARGE_EN == 1)
#define CHARGE_HARDWARE         0
#define CHARGE_SOFTWARE          1
#define CHARGE_EXTERNAL           2
#define CONFIG_CHARGE_MODE    CHARGE_HARDWARE
#endif

/* #define CONFIG_APP_HALFDUPLEX */
//#define CONFIG_APP_EQUANLIZER

#define CONFIG_DAC_DELAY_OPERATION                   0
#define CONFIG_ANA_DAC_CLOSE_IN_IDLE			1


//#define CONFIG_IRDA 
/* #define WROK_AROUND_DCACHE_BUG */

// PWM not sleep when no connection
//#define CONFIG_PWM_NOT_SLEEP


//#define CONFIG_ACTIVE_SSP
#define CONFIG_SFT_INTR_TRIGGER         0

#define BT_MODE_1V1 (1<<0)
#define BT_MODE_1V2 (1<<1)
#define BT_MODE_TWS (1<<2)

#define BT_MODE		           BT_MODE_1V1



#if( BT_MODE==BT_MODE_1V1)
	#ifndef BT_ONE_TO_MULTIPLE
	#define BT_ONE_TO_MULTIPLE
	#endif
	#ifndef BT_ONLY_ONE_BT
	#define BT_ONLY_ONE_BT
	#endif
	#ifdef CONFIG_TWS
	#undef CONFIG_TWS
	#endif
#elif( BT_MODE==BT_MODE_1V2)
	#ifndef BT_ONE_TO_MULTIPLE
	#define BT_ONE_TO_MULTIPLE
	#endif
	#ifdef BT_ONLY_ONE_BT
	#undef BT_ONLY_ONE_BT
	#endif
	#ifdef CONFIG_TWS
	#undef CONFIG_TWS
	#endif
    #define CONFIG_AUTO_CONN_AB  // 上电回连 A ,B 手机交叉循环连接
#elif( BT_MODE==BT_MODE_TWS)
	#ifdef BT_ONE_TO_MULTIPLE
	#undef BT_ONE_TO_MULTIPLE
	#endif
	#ifdef BT_ONLY_ONE_BT
	#undef BT_ONLY_ONE_BT
	#endif
	#ifndef CONFIG_TWS
	#define CONFIG_TWS
	#endif
#endif


#ifdef CONFIG_TWS
    #ifdef BT_ONE_TO_MULTIPLE
    #undef BT_ONE_TO_MULTIPLE
    #endif
    #ifdef BT_ONLY_ONE_BT
    #undef BT_ONLY_ONE_BT
    #endif
    #ifdef CONFIG_DAC_DELAY_OPERATION
    #undef CONFIG_DAC_DELAY_OPERATION
    #define CONFIG_DAC_DELAY_OPERATION 1
    #endif
    #ifdef INQUIRY_ALWAYS
    #undef INQUIRY_ALWAYS
    #endif
    #ifdef CONFIG_AUDIO_TRANSFER_DMA
    #undef CONFIG_AUDIO_TRANSFER_DMA
	#define CONFIG_AUDIO_TRANSFER_DMA 1
    #endif
    #ifdef CONFIG_CPU_CLK_OPTIMIZATION
    #undef CONFIG_CPU_CLK_OPTIMIZATION
    #define CONFIG_CPU_CLK_OPTIMIZATION 0
    #endif

    #define TWS_CONFIG_LINEIN_BT_A2DP_SOURCE

    #define CONFIG_TWS_BOARD    // for calibrating the frequency deviation of the tws board 128202A-Y224

    #define TWS_POWER_ON_MECHANISM_ENABLE   1 /* Don't set soft power down flag [APP_ENV_SYS_FLAG_SW_POWERON] if enabled it. */

    #define TWS_CONN_STEREO_ALL_TIME        1

    #define TWS_CONN_MECHANISM_V2           1

    #define TWS_M_S_2DH5_ENABLE             1
    
    #define TWS_MATCH_CLEAR_ENABLE          1

    #define TWS_VENDOR_DEP_LINEIN_CMD     		0x22
    #define TWS_VENDOR_DEP_LINEIN_UNMUTE     	0
    #define TWS_VENDOR_DEP_LINEIN_MUTE     	1
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE	
    #define TWS_VENDOR_DEP_LINEIN_ATTACH     	2
    #define TWS_VENDOR_DEP_LINEIN_DETACH     	3
    #define CONFIG_ADC_DMA          			1
#else
    #define CONFIG_ADC_DMA          			0
#endif	

    #define TWS_VENDOR_DEP_SHAREME_CMD     		       0x23
    #define TWS_VENDOR_DEP_SHAREME_LEFT			0
    #define TWS_VENDOR_DEP_SHAREME_RIGTH		       1
    #define TWS_VENDOR_DEP_SHAREME_STEREO		2
    #define TWS_PAIR_TIMEOUT_ENABLE				0
    #define TWS_PAIR_MAX_TIME					30000//30s

    #define CONFIG_TWS_SUPPORT_OLD_OPERATE		0 // support old operation mode: one key:slave,another:master


    #define TWS_HFP_ENABLE           			              0

    #define SBC_OUTPUT_NEED_MALLOC_ENABLE   	       0
    #define EXTERNAL_LNA_ENABLE           		              0  // external low-noise amplifier.
    #define CONFIG_TWS_AUTOCONNECT                            0
    #define CONFIG_TWS_KEY_MASTER				       1

    #define CONFIG_CLEAR_MEMORY_EN				       0
    #define TWS_A2DP_AVOID_POP_ENABLE       	              0
    #define TWS_SCO_AVOID_POP_ENABLE       		       0

#if (CONFIG_TWS_AUTOCONNECT == 1)
    #ifdef CONFIG_TWS_KEY_MASTER
    #undef CONFIG_TWS_KEY_MASTER
	#define CONFIG_TWS_KEY_MASTER				0
    #endif
#endif

#if (TWS_HFP_ENABLE == 1)
	#define MASTER_DECODE_MSBC_IN_TASK			1
	#define HF_PRINTF_ENABLE					       0
	#define HF_RING_TWS_ENABLE					1
	#define TWS_SCO_AVOID_MSBC_NODE_LEVEL_TOO_LOW_ENABLE	   1
	#define TWS_A2DP_AVOID_SBC_NODE_LEVEL_TOO_LOW_ENABLE	   1
#else
	#define MASTER_DECODE_MSBC_IN_TASK			0
	#define HF_PRINTF_ENABLE					       0
	#define HF_RING_TWS_ENABLE					0
	#define TWS_SCO_AVOID_MSBC_NODE_LEVEL_TOO_LOW_ENABLE	   0
	#define TWS_A2DP_AVOID_SBC_NODE_LEVEL_TOO_LOW_ENABLE	   0
#endif

#if (CONFIG_TWS_SUPPORT_OLD_OPERATE == 1)
	#define DIAC_LOWEST_BYTE    0x11
    	#define CONFIG_TWS_AUTOCONNECT                          0
    #ifdef CONFIG_TWS_AUTOCONNECT
        #undef CONFIG_TWS_AUTOCONNECT
        #define CONFIG_TWS_AUTOCONNECT				0
    #endif
    #ifdef CONFIG_TWS_KEY_MASTER
        #undef CONFIG_TWS_KEY_MASTER
        #define CONFIG_TWS_KEY_MASTER				0
    #endif
#else
	#define DIAC_LOWEST_BYTE    0x33//0x9E8B11 ((lap > 0x9e8b3f) || (lap < 0x9e8b00))
#endif

#endif

//#define CONFIG_PRODUCT_TEST_INF
#define CONFIG_MEMPOOL_SIZE                    (38 * 1024) // RAM:112K:  36*1024    RAM:128K:  40*1024

#ifdef BT_ONE_TO_MULTIPLE
    #define A2DP_PATCH_FOR_AVRCP
    #define OTT_STRETIGG_LINK_COEXIST
#ifdef BT_ONLY_ONE_BT
    #define BT_MAX_AG_COUNT                                             1
    #define CONFIG_SELECT_PREV_A2DP_PLAY		         0
    #define CONFIG_A2DP_PLAYING_SET_AG_FLOW		  0
    #define CONFIG_AS_SLAVE_ROLE                  0

    #define INQUIRY_ALWAYS
    //#define CONN_WITH_MUSIC		
    #define NEED_SNIFF_DEVICE_COUNT                              1
    //#define IPHONE_MUSIC_STATE_COME_LATE_REPAIR     //for iphone PLAYING come before MA having comed,we need stream_start manually on avrcp notify coming
#else
    #define BT_MAX_AG_COUNT                                             2
    #define CONFIG_A2DP_PLAYING_SET_AG_FLOW		  1
    #define CONFIG_SELECT_PREV_A2DP_PLAY		         1
    #define CONFIG_AS_SLAVE_ROLE                  1
    //#define INQUIRY_ALWAYS
    #define CONN_WITH_MUSIC
    #define NEED_SNIFF_DEVICE_COUNT                              2
    #if(CONFIG_SELECT_PREV_A2DP_PLAY == 0)
    	#define IPHONE_MUSIC_STATE_COME_LATE_REPAIR     //for iphone PLAYING come before MA having comed,we need stream_start manually on avrcp notify coming
    #endif
#endif
//    #define NO_SCAN_WHEN_WORKING
#else
    #define A2DP_PATCH_FOR_AVRCP
    #define BT_MAX_AG_COUNT                                             1
    #define CONFIG_A2DP_PLAYING_SET_AG_FLOW		  0
    #define CONFIG_SELECT_PREV_A2DP_PLAY		         0
    #ifdef CONFIG_TWS
        #define NEED_SNIFF_DEVICE_COUNT                          2
    #else
        #define NEED_SNIFF_DEVICE_COUNT                          1
    #endif
    #define CONFIG_AS_SLAVE_ROLE                  0

    #define INQUIRY_ALWAYS
#endif
#if (CONFIG_DEBUG_PCM_TO_UART == 1)
    #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
        #undef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    #endif
#endif

#define CONFIG_HFP17_MSBC_SUPPORTED   1
#define IPHONE_BAT_DISPLAY

/* #define INCOMINGCALL_HF_TRANSFER_SCO */
//#define INCOMING_CALL_RING //play remote number wave first, and then internal ring whatever remote support inband ring

#define CONFIG_AUD_FADE_IN_OUT              0
//#if(CONFIG_AUD_FADE_IN_OUT == 1)
#define AUD_FADE_SCALE_MIN                  (0)

#define AUD_FADE_SCALE_LEFT_SHIFT_BITS    7 //  10 //(8)     // 128
#define AUD_FADE_SCALE_MAX                  (1 << AUD_FADE_SCALE_LEFT_SHIFT_BITS)

#define AUD_FADE_STEP                       (1)     // about 640ms =  10ms * 128/2
//#endif

//#define UPDATE_LOCAL_AFH_ENGINE_AFTER_ACK
//#define LMP_LINK_L2CAL_TIMEOUT_ENABLE

#define CONFIG_SBC_DECODE_BITS_EXTEND		0
#define CONFIG_SBC_PROMPT      				1
#define A2DP_SBC_DUMP_SHOW
#define CONFIG_SW_SWITCH_KEY_POWER			1
//芯片常供电拨动开关控制开关机

#ifdef CONFIG_BLUETOOTH_SPP
//#define SUPPORT_SPP_128BITS_UUID                               /*supported 128 UUID spp*/
#endif

/*****************Dual Mode with RW_BLE config, Yangyang, 2018/6/4********************/
#define BT_DUALMODE_RW                                /*Dual mode BT + BLE(RW kenel), when enable this macro*/
#ifdef BT_DUALMODE_RW
#define RW_BLE_WITH_ESCO_ENSURE_VOICE_FLUENCY         /*When RW_BLE conflict with esco, schedule to ensure voice fluency*/
#define RW_BLE_WITH_MUSIC_ENSURE_MUSIC_FLUENCY        /*When RW_BLE conflict with music, schedule to ensure music fluency*/
#define BT_DUALMODE_RW_SLEEP                          /*BT_DUALMODE_RW_SLEEP active if enbale*/
//#define BT_DUALMODE_RW_BLE                      /*Single mode BLE(RW kernel), effective only BT_DUALMODE_RW enabled*/
#endif
/***********************************END***********************************************/

/**************************OTA support, Yangyang, 2019/8/26**************************/
//#define BEKEN_OTA                                     /* OTA enable */
#ifdef BEKEN_OTA
#ifdef BT_DUALMODE_RW 
//#define BEKEN_OTA_BLE							      /* OTA supported by BLE(RW) */
#endif
//#define BEKEN_OTA_SPP							      /* OTA supported by SPP */
#endif
/***********************************END***********************************************/


/*************************************************************************
* control version upgraded.
*************************************************************************/
#define UPGRADED_VERSION                1
/*************************************************************************  
* just only for pts testing, it should be closed in release version.
*************************************************************************/
#define PTS_TESTING 0
// for BQB Test
#define CONFIG_CTRL_BQB_TEST_SUPPORT    1


/*************************************************************************  
* just only for MP3 player, it should be closed in release version.
*************************************************************************/
#if (CONFIG_APP_MP3PLAYER == 1)
#ifdef CONFIG_APP_SDCARD
    #undef CONFIG_APP_SDCARD
#endif
    #define CONFIG_APP_SDCARD
    #define CONFIG_APP_USB       // U盘模式时，不支持SNIFF功能
    
#ifdef CONFIG_APP_SDCARD  
//	#define CONFIG_APP_SDCARD_4_LINE
#ifdef CONFIG_APP_SDCARD_4_LINE
    #undef CONFIG_APP_SDCARD_4_LINE
#endif
#ifndef CONFIG_APP_SDCARD_4_LINE
    #define CONFIG_SDCARD_BY_MULTIBLOCK_WAY
#endif
#endif    
    
    #define  SYS_VOL_SAVE		0 // save vol
    #define CALC_PLAY_TIME     0

#if 0//ndef LE_SLEEP_ENABLE
#ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    #undef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE 
#endif
#ifdef CONFIG_LINE_SD_SNIFF
    #undef CONFIG_LINE_SD_SNIFF //全功能版本LINEIN/SD，无双模
#endif
#endif

//#define FM_ENABLE   
#ifdef FM_ENABLE
#define FM_IN_LINEIN
#define FM_BK1080H
#define DATA_PIN  	11
#define CLK_PIN   10
#endif
#define CONFIG_NOUSE_RESAMPLE 		1

#define CONFIG_BK_QFN56_DEMO    1

#endif

#define ADC_CHANNEL_1		                1 // Virtual GPIO2(Physical GPIO6)
#define ADC_CHANNEL_2  	                       2 // Virtual GPIO3(Physical GPIO7)
#define ADC_CHANNEL_4		                4 // Virtual GPIO11(Physical GPIO18)
#define ADC_CHANNEL_6  	                       6 // Virtual GPIO13(Physical GPIO20)
#define ADC_CHANNEL_7  	                       7 // Virtual GPIO14(Physical GPIO21)
#define ADC_CHANNEL_7  	                       7 // Virtual GPIO14(Physical GPIO21)
#define ADC_CHANNEL_10  	                10 // Virtual GPIO15(Physical GPIO22)
#define ADC_CHANNEL_NULL                    0xff


#ifndef CONFIG_TWS
//#define CONFIG_BLUETOOTH_COEXIST      // bluetooth coexist, can connect bluetooth in other modes.   应用层可直接开关  暂不支持TWS下的共存
#endif


/***********************************************************************************
 * Trace32 debug use for only, while "-flto" must be unselected and the target shoud
 * download this Trace32 debug bin firstly by other download tool
 */
//#define TRACE32_DEBUG
#ifdef TRACE32_DEBUG
#ifdef	CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
#undef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
#endif
#ifdef CONFIG_CPU_CLK_OPTIMIZATION
#undef CONFIG_CPU_CLK_OPTIMIZATION
#define CONFIG_CPU_CLK_OPTIMIZATION             0
#endif
#endif
/**********************************************************************************/
#define CONFIG_APP_RECONNECT_MATCH	0

#define DEBUG_WATCHDOG_RESET_ISSUE_ENABLE                       0
#define CONFIG_AUDIO_USED_MCU 						0// Line in function --1:MCU   0:DSP
#define CALI_BY_PHONE_BIT (1<<0)
#define CALI_BY_8852_BIT (1<<1)
#define CALI_BY_JZHY_BIT (1<<2)
#define CONFIG_RF_CALI_TYPE         (CALI_BY_8852_BIT|CALI_BY_JZHY_BIT)  //(CALI_BY_8852_BIT|CALI_BY_JZHY_BIT)  (CALI_BY_PHONE_BIT)



#define CONFIG_APP_TOOLKIT_5           1
#if (CONFIG_APP_TOOLKIT_5 == 1)
#define POWERKEY_5S_PARING		1
#if 0//def BT_MAX_AG_COUNT
#undef BT_MAX_AG_COUNT
#define BT_MAX_AG_COUNT (app_check_bt_mode(BT_MODE_1V2)?2:1)
#endif
#endif

#endif

