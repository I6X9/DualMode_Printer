#ifndef __G711_H__
#define __G711_H__
int16_t linear2alaw(int16_t pcm_val);        
int16_t alaw2linear(int16_t a_val);	
int16_t linear2ulaw(int16_t pcm_val);
int16_t ulaw2linear(int16_t u_val);
#endif

