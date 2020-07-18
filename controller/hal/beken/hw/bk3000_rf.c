/**
 * @file   bk3000_rf.c
 * @author  <Lab@beken>
 * @date   Tue Nov 18 09:04:52 2014
 *
 * @brief
 *
 *
 */

#include "sys_types.h"
#include "bk3000_reg.h"

#if (CONFIG_TX_CALIBRATION_EN == 1)
#include "bk3266_calibration.h"
#endif
#include "app_beken_includes.h"

u_int32 XVR_analog_reg_save[16]={0};
u_int32 XVR_reg_0x24_save = 0;
void BK3000_RF_Initial(void) 
{    /**< Thu Nov 13 11:42:33 2014 */
    app_env_handle_t  env_h = app_env_get_handle();
    /* 20150929 11:50 modified */
	/* 20160531 11:00 modified */
    /* 20170830 13:35 modified for BK3266 */
 	BK3000_XVR_REG_0x23                          = 0x7B13B14B;// 0x2E276276;
    BK3000_XVR_REG_0x24 = XVR_reg_0x24_save      = 0x00F41802;// 0x000005A9; for +PA 0x0129 0x00140C02
    BK3000_XVR_REG_0x27                          = 0x0008F100;// 0x000A0000;

    BK3000_XVR_REG_0x00 =XVR_analog_reg_save[0]  = 0x8DDF2010;//0xBCDF2010;//0xA15F2010;
    BK3000_XVR_REG_0x01 =XVR_analog_reg_save[1]  = 0x7C805A27;//0x7C805A27;
    BK3000_XVR_REG_0x02 =XVR_analog_reg_save[2]  = 0x40924005;
    BK3000_XVR_REG_0x03 =XVR_analog_reg_save[3]  = 0x00272833;//0x00272831;//0x00272830;
    //BK3000_XVR_REG_0x04 =XVR_analog_reg_save[4]  = 0x00000000;
    //BK3000_XVR_REG_0x05 =XVR_analog_reg_save[5]  = 0x00000000;
    BK3000_XVR_REG_0x06 =XVR_analog_reg_save[6]; //waring: xvr_reg_6 setting at DCO; //  = 0x02C80400;//0x0210C078;
    BK3000_XVR_REG_0x07 =XVR_analog_reg_save[7]  = 0xFEEE7800;//0xFEEE7C20; //0xFEE27C20;//0xFEE27C20;
    BK3000_XVR_REG_0x08 =XVR_analog_reg_save[8]  = 0xD2D05028;//0xD2D05028;//0xD01C5000;
    BK3000_XVR_REG_0x09 =XVR_analog_reg_save[9]  = 0x2BDFF304;//0x2BDFF304;
    BK3000_XVR_REG_0x0A =XVR_analog_reg_save[10] = 0x420C5F55;//0x720C5F55;// 0x420C5F55;//0x48005555;//0x44805FD7;//0x42805FD7;//0x42805FD7;
    BK3000_XVR_REG_0x0B =XVR_analog_reg_save[11] = 0x80F46B8F;//0x80F4E78F;//0x80F4EFFF;
    BK3000_XVR_REG_0x0C =XVR_analog_reg_save[12] = 0x00060339;//0x01060273;
    if( env_h->env_cfg.used == 0x01 )
    {
        if(env_h->env_data.offset_bak<0x3f)
            env_h->env_cfg.system_para.frq_offset = env_h->env_data.offset_bak;
        BK3000_XVR_REG_0x0D =XVR_analog_reg_save[13] = 0x81010100|(env_h->env_cfg.system_para.frq_offset&0x3f);
    }
    else
    {
        BK3000_XVR_REG_0x0D =XVR_analog_reg_save[13] = 0x81010123;//0x8101011F  <5:0>0x23 -> cl = 9pF;
    }
    BK3000_XVR_REG_0x0E =XVR_analog_reg_save[14] = 0x00B09350;
    BK3000_XVR_REG_0x0F =XVR_analog_reg_save[15] = 0x3B13B13B;


    BK3000_XVR_REG_0x20                          = 0x446475C6;// 0x00A7A940;
    BK3000_XVR_REG_0x21                          = 0x4E431080;// 0x00000000;
    BK3000_XVR_REG_0x22                          = 0xCB082409;// 0xC12D06DA;CB082409

    BK3000_XVR_REG_0x26							 = 0x08000000;
    BK3000_XVR_REG_0x27                          = 0x0008F100;// 0x000A0000;
    BK3000_XVR_REG_0x28                          = 0x03132333;// 0x03132333;
    BK3000_XVR_REG_0x29                          = 0x0D041100;// 0x00001100;
    BK3000_XVR_REG_0x2A                          = 0x01504042;// 0x01504042;
    BK3000_XVR_REG_0x2B                          = 0x404B110F;
    BK3000_XVR_REG_0x2D							 = 0x00284841;

    /* XVR 27 For software define */

    /* XVR 27 REG
      * bit 0-7:   AFH threshold; typical = 0x20;
      * bit 8-11:  edr_rx_edr_delay typical = 0x01;
      * bit 12-15: edr_tx_edr_delay typical = 0x04;
      */
    BK3000_XVR_REG_0x27                         = 0x000004120;



	// TX Modulate
    BK3000_XVR_REG_0x30                          = 0xA832981B;// 0xA832941B;//0xA828A41C;//0x00011010;// 0x00011010;0xA828941C
    BK3000_XVR_REG_0x31                          = 0x0002326E;// 0x0002326e;
    BK3000_XVR_REG_0x32                          = 0xFFFFFFFF;// 0xFFFF8080;
    BK3000_XVR_REG_0x33                          = 0x00001300;// 0x00000224;
    BK3000_XVR_REG_0x34                          = 0x00000000;// 0x00000000;
    BK3000_XVR_REG_0x35                          = 0x00000000;// 0x00000000;
    BK3000_XVR_REG_0x36                          = 0x00000000;// 0x00000000;
    BK3000_XVR_REG_0x37                          = 0x00000000;// 0x00000000;
                                                                   
    // RX Demodulate                                               
    BK3000_XVR_REG_0x38                          = 0x2E1603FF;// 0xAE158011;
    BK3000_XVR_REG_0x39                          = 0xA4802860;// 0x0000C000;
    BK3000_XVR_REG_0x3A                          = 0x00000000;// 0x00000000;
    BK3000_XVR_REG_0x3B                          = 0x09345288;//0x49345288;// 0x09345288;49345288
    BK3000_XVR_REG_0x3C                          = 0x308403D1;//0x1E8403B1;// 0x108403B1;
    BK3000_XVR_REG_0x3D                          = 0x626A0027;// 0x626A0027;

    
	BK3000_XVR_REG_0x40                          = 0x01000000;// 0x00000000;

	BK3000_XVR_REG_0x25                          = 0x80000000;


    //delay1ms
    //buck off,bit18 triggle
    //BK3000_A2_CONFIG = 0x7E16AD9C;//0x7E16A99C;
    BK3000_A2_CONFIG |= (1<<18);
    delay_us(1250);


#if 1
    uint32 xvr_reg;
    BK3000_XVR_REG_0x24                          = 0x00F41808;
    BK3000_XVR_REG_0x25                          = 0x80002400;
    BK3000_XVR_REG_0x09 = XVR_analog_reg_save[9] | (0x01 << 0);
    delay_us(1000);
    BK3000_XVR_REG_0x09 = XVR_analog_reg_save[9] & ~(0x01 << 0);
    delay_us(1000);
    xvr_reg = BK3000_GPIO_60_CONFIG; //gpio_3c;
    xvr_reg &= (0x1f << 6);
    xvr_reg >>= 6;
    if(xvr_reg < 26)
        xvr_reg += 6;
    os_printf("===xvr_IF calibration:%x\r\n",xvr_reg);
    BK3000_XVR_REG_0x09 = XVR_analog_reg_save[9] = XVR_analog_reg_save[9] | (0x01 << 2);
    XVR_analog_reg_save[9] &= ~(0x1f << 4);
    XVR_analog_reg_save[9] |= (xvr_reg << 4);
    BK3000_XVR_REG_0x09 = XVR_analog_reg_save[9];

    BK3000_XVR_REG_0x03 = XVR_analog_reg_save[3] | (0x01 << 3);
    delay_us(1000);
    BK3000_XVR_REG_0x03 = XVR_analog_reg_save[3] & ~(0x01 << 3);
    delay_us(1000);
    
    BK3000_XVR_REG_0x01 = XVR_analog_reg_save[1] | (0x01 << 10);
    delay_us(1000);
    BK3000_XVR_REG_0x01 = XVR_analog_reg_save[1] & ~(0x01 << 10);
    delay_us(3000);

	BK3000_XVR_REG_0x25                          = 0x80000000;
#endif

#if (CONFIG_TX_CALIBRATION_EN == 1) //TX calibration
//    BK3000_WDT_CONFIG = 0x5A0000;
//    BK3000_WDT_CONFIG = 0xA50000;
	CLEAR_WDT;
	os_printf("[XVR]: TX calibration ...... ");
	calib_dbg_enable(0);
	tx_calibration_init();
	tx_dc_calibration();
	tx_q_const_iqcal_p_calibration();
	tx_iq_gain_imbalance_calibration();
	tx_iq_phase_imbalance_calibration();
    /* tx output power is not used */
	//tx_output_power_calibration();
	tx_calibration_deinit();
	//rx_rssi_calibration();
	os_printf("OK\r\n");
	CLEAR_WDT;
#endif

}