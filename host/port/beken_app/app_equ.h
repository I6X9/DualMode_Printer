#ifndef _APP_EQU_H_
#define _APP_EQU_H_
#include "app_env.h"

//audio equalizer
typedef struct
{
    int16 x[2][3];
    int16 y[2][2];
    int    a[2];
    int    b[3];
    uint8 flag_enable;
}__PACKED_POST__ aud_equ_t;

//int aud_apply_equalizer( int index, aud_equ_conf_t *conf );
void aud_euqalizer_enable( int index, uint8 enable );
void app_equalizer_calc( int16 samplel, int16 sampler, int16 *outl, int16 *outr );
int16 app_equalizer_calc_hfp( int16 sample );
void app_equalizer_a2dp_init( app_eq_para_t * app_a2dp_equ );
int app_equalizer_get_enable_count( void );
void app_equalizer_show( void );
void app_equalizer_a2dp_apply(void);
void app_equalizer_hfp_apply(void);
void app_equalizer_init(void);
void app_equalizer_recover_enable( void );

#endif
