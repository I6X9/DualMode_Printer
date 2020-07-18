/**
 **************************************************************************************
 * @file    audio_sync.h
 * @brief   Audio synchronization
 * 
 * @author  Aixing.Li
 * @version V2.0.0
 *
 * &copy; 2017-2019 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __AUDIO_SYNC_H__
#define __AUDIO_SYNC_H__

#include "types.h"

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief  Audio synchronization initialization
 * @return NULL
 */
void audio_sync_init(void);

/**
 * @brief  Prepare for DAC open
 * @param  t the playing time for the received stream packet
 */
void audio_sync_prepare_for_dac_open(uint32_t);

/**
 * @brief  Process for DAC open
 * @return NULL
 */
void audio_sync_process_for_dac_open(void);

/**
 * @brief  Calculate received stream packet playing time
 * @return The playing time for the received stream packet
 */
uint32_t audio_sync_calc_stream_packet_play_time(void);

/**
 * @brief  Audio synchronization process
 * @param  tpm the received node frame expected playing time
 * @return 0: should not discard or add a node frame,
 *         -1: should discard a node frame
 *          1: should add a node frame
 */
int32_t audio_sync_process(uint32_t tp);

/**
 * @brief  Show some debug information for audio synchronization
 * @return NULL
 */
void audio_sync_show_info(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__AUDIO_SYNC_H__
