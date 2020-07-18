#ifndef _DRIVER_SDADC_H_
#define _DRIVER_SDADC_H_

#define SDADC_VOLUME_MAX        124 //95    // all 48db, 0.5db per step

void sdadc_init( void );
void sdadc_enable( uint8 enable );
void sdadc_isr( void );
void sdadc_volume_adjust( uint8 volume );
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE

extern void adc_int_open(void);
extern void adc_int_close(void);
#endif
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
void sdadc_enable( uint8 enable );
#endif
#endif

// EOF
