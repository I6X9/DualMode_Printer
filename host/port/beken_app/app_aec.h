#ifndef _APP_AEC_H
#define _APP_AEC_H

#include "types.h"

void app_aec_init(int sample_rate);
void app_aec_uninit(void);
void app_aec_fill_rxbuff(uint8_t* buf, uint8_t fid, uint32_t len);
void app_aec_fill_txbuff(uint8_t* buf, uint32_t len);
void app_aec_swi(void);

void aec_fill_buffer(uint8_t *buf,uint32_t len);
void aec_read_buffer(uint8_t *buf,uint32_t len);

void app_set_aec_para(uint8 *para);
void app_aec_set_mic_delay(int8 decay_ime);

#endif
