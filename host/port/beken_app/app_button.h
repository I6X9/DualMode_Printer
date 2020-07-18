#ifndef _APP_BUTTON_H_
#define _APP_BUTTON_H_

/* #define BTN_DEBUG */

#ifdef BTN_DEBUG
    #define BTN_PRT      os_printf
#else
    #define BTN_PRT      os_null_printf
#endif


#define BUTTON_CLICK                                    0x80000000
#define BUTTON_DOUBLE_CLICK                     0x40000000
#define BUTTON_REPEAT                                 0x08000000
#define BUTTON_LONG_PRESS                         0x20000000
#define BUTTON_VLONG_PRESS                       0x04000000
#define BUTTON_VVLONG_PRESS                     0x01000000
#define BUTTON_LONG_PRESS_UP                    0x02000000 
#define BUTTON_VLONG_PRESS_UP                   0x10000000 
#define BUTTON_VVLONG_PRESS_UP                  0x00800000 


#define BUTTON_TYPE_MASK                          0x0000001c // hfp.a2dp.all,twc

#define BUTTON_TYPE_NON_HFP                    0x00000004
#define BUTTON_TYPE_HFP                            0x00000008
#define BUTTON_TYPE_TWC              	        0x00000010
#define BUTTON_TYPE_ALL                             0x0000001c


#define BUTTON_GPIO_MASK                           0x01FFFFE3 //0x7FFFFF//0x7DFFE3

#define BUTTON_MODE_ADC_CHAN_SHIFT              19 //配置工具通道
#define BUTTON_MODE_ADC_SUM_SHIFT               15 //配置工具分段数
#define BUTTON_MODE_ADC_KEY_SHIFT               5
#define BUTTON_IS_ADC_MASK                      0x01C00000//(7<<22) ADC_key的使能标志
#define BUTTON_AD_CHANNEL_MASK                  0x00380000//(7<<19) ADC_key的channel
#define BUTTON_MODE_ADC_SUM_MASK                0x00078000 // ADC_key的分段总数
#define BUTTON_MODE_ADC_KEY_MASK                0x00007FE0 // ADC_key的具体bit



enum
{
    BUTTON_PRESS_NONE        = 0,
    BUTTON_PRESS_DOWN        = 1,
    BUTTON_PRESS_UP          = 2,
    BUTTON_DOUBLE_PRESS_DOWN = 3,
    BUTTON_DOUBLE_PRESS_UP   = 4,
    BUTTON_PRESS_DOWN_LONG1  = 5,
    BUTTON_PRESS_DOWN_LONG2  = 6
};

typedef uint8_t APP_BUTTON_STATE;
typedef int (*button_action_func)(void) ;

extern void button_action(uint32 commit_info);
extern void button_scanning(void);
void buttonInit(void);
#endif // _APP_BUTTON_H_

