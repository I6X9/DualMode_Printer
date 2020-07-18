#include <string.h>
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
#include "beken_external.h"
#endif
#ifdef CONFIG_TWS
#include "audio_sync.h"
#include "../../../bluetooth/profile/a2dp/bt_sbc.h"
#endif

#ifdef CONFIG_TWS
app_sbc_t  app_sbc;
#else
static app_sbc_t  app_sbc;
#endif
static sbc_mem_node_t  *sbc_mem_node = NULL;

static uint8 flag_sbc_buffer_play = 0;
int encode_buffer_full = 0;

#ifdef BEKEN_DEBUG
int encode_pkts = 0;
int decode_pkts = 0;
int encode_buffer_empty = 0;
#endif

#if (SBC_ERR_FRM_PROC == 1)
static uint8 MidFilter_Flag[AUDIO_BUFF_LEN >> 9];
#define SBC_MUTE_THR	6
#endif

#ifdef SBC_FIRST_DISCARD_ENABLE 
uint8 sbc_first_discrad_flag = 0;
uint8 sbc_first_discard_count = 0;
uint8 sbc_first_discard_data = 0;
#endif


#ifdef CONFIG_TWS
extern t_clock HW_Get_Bt_Clk_Avoid_Race(void);
extern t_clock HW_get_bt_clk(void);
extern inline t_clock get_btclk_as_piconet_slave(void);
extern inline t_clock get_btclk_as_piconet_master(void);

inline uint32 sample_num_per_sbc_frame(void)
{
#if 0
    return get_sbc_frame_samples(app_sbc.sbc_ptr, SBC_MODE);
#else    
    return SAMPLES_PER_SBC_FRAME;
#endif    
}

inline uint32 pcm_bytes_per_sbc_frame(void)
{
    return get_pcm_bytes_per_sbc_frame(app_sbc.sbc_ptr, SBC_MODE);
}
#endif

uint8 get_flag_sbc_buffer_play(void)
{
    return flag_sbc_buffer_play;
}

void set_flag_sbc_buffer_play(uint8 flag)
{
    flag_sbc_buffer_play = flag;
}

void sbc_init_adjust_param(void)
{
    app_sbc.sbc_ecout = 0;
    app_sbc.sbc_clk_mode = 0;
}

RAM_CODE void sbc_mem_pool_freeNode( sbc_mem_list_t  *list,
     sbc_mem_node_t *node )
{
    if( list->head == NULL )
    {
        list->head = list->tail = node;
        node->next = NULL;
    }
    else
    {
        list->tail->next = node;
        list->tail = node;
        node->next = NULL;
    }
}

RAM_CODE sbc_mem_node_t * sbc_mem_pool_getNode( sbc_mem_list_t *list)
{
    sbc_mem_node_t *node = NULL;

    if(NULL == list->head)
    {
        return NULL;
    }

    node = list->head;
    list->head = list->head->next;
    
    if(NULL == list->head)
    {
        list->tail = NULL;
    }

    return node;
}
#ifdef CONFIG_TWS
void sbc_mem_pool_free_first_node(void)
{
    //os_printf("sbc_mem_pool_free_first_node(%d)\n", app_sbc.sbc_ecout);
    
    uint32 interrupts_info, mask;

    VICMR_disable_interrupts(&interrupts_info, &mask);
    
    sbc_mem_node_t *node = sbc_mem_pool_getNode(&app_sbc.sbc_mem_pool.uselist);
    if(node != NULL)
    {
        sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
        app_sbc.sbc_ecout--;
    }
    else
    {
    }
    
    VICMR_restore_interrupts(interrupts_info, mask);
    
}

void clean_all_sbc_frame_node(void)
{
    //os_printf("clean_all_sbc_frame_node()\r\n");
    sbc_discard_uselist_node();   
}
#endif
void sbc_mem_pool_init(int framelen)
{
    uint8 *mem_base = app_sbc.sbc_encode_buffer;
    int   node_len = SBC_FRAME_LENGTH_MAX; //( (framelen >> 2) + 1 ) << 2; // alignment
    int   node_num = SBC_ENCODE_BUFFER_LEN / node_len;
    int  i = 0;

    if( node_num > SBC_MEM_POOL_MAX_NODE )
        node_num = SBC_MEM_POOL_MAX_NODE;
    
    app_sbc.sbc_mem_pool.node_num = node_num;
//#ifdef CONFIG_TWS
    if(NULL != sbc_mem_node)
    {
        jfree(sbc_mem_node);
        //os_printf("000---jfree---sbc_mem_node\r\n");
        sbc_mem_node = NULL;
    }
//#endif
    sbc_mem_node = (sbc_mem_node_t *)jmalloc(sizeof(sbc_mem_node_t)*SBC_MEM_POOL_MAX_NODE, M_ZERO);
    //sbc_mem_node = (sbc_mem_node_t *)jmalloc(sizeof(sbc_mem_node_t) * node_num, M_ZERO);
    //os_printf("[sbc buffer pool init: node num: %d, node_len : %d\n", node_num, node_len );
    for( i = 0; i < node_num; i++)
    {
        sbc_mem_node[i].data_ptr = mem_base + i*node_len;
        sbc_mem_pool_freeNode( &app_sbc.sbc_mem_pool.freelist, &sbc_mem_node[i] );
        //os_printf("node[%d] memptr: %x\n", i, sbc_mem_node[i].data_ptr);
    }
#ifdef CONFIG_TWS
    app_sbc.sbc_ecout = 0;

    dac_init_clk();
    //	show_dac_clk(1);
#endif
}

void sbc_mem_pool_deinit( void )
{
    sbc_mem_node_t *node;
    os_printf("sbc_mem_pool_deinit\r\n");
    do
    {
        node = sbc_mem_pool_getNode( &app_sbc.sbc_mem_pool.uselist );
    #ifdef BEKEN_DEBUG
        decode_pkts++;
    #endif
    }while( node != NULL );

    do
    {
        node = sbc_mem_pool_getNode(&app_sbc.sbc_mem_pool.freelist);
    } while(node != NULL);

    app_sbc.sbc_mem_pool.node_num = 0;
    app_sbc.sbc_target_initial = 0;
    if (NULL != sbc_mem_node) 
    {
        jfree(sbc_mem_node);
        //os_printf("222---jfree---sbc_mem_node\r\n");
        sbc_mem_node = NULL;
    }
    
#ifdef CONFIG_TWS
    app_sbc.sbc_ecout = 0;
    app_sbc.decode_cnt = 0;
    app_sbc.src_pkt_num = 0;
    tws_clear_a2dp_buffer(); // It must be excuted after [set_flag_sbc_buffer_play(0)].
#endif
    set_flag_sbc_buffer_play(0);

}

uint32 sbc_buf_get_use_count(void)
{
#ifdef BT_ONE_TO_MULTIPLE
    return app_sbc.use_count;
#else
    return 0;
#endif
}

void sbc_buf_increase_use_count(void)
{
#ifdef BT_ONE_TO_MULTIPLE
    app_sbc.use_count += 1;
#endif
}

void sbc_buf_decrease_use_count(void)
{
#ifdef BT_ONE_TO_MULTIPLE
    app_sbc.use_count -= 1;
#endif
}

uint16 sbc_buf_get_node_count(void)
{
    return app_sbc.sbc_ecout;
}
/************************************************************
*
* Its region in [_sbcmem_begin, _sbcmem_end], has 16k.
*
************************************************************/
uint8 *get_ram_buff(void)
{
    extern uint32 _sbcmem_begin;
    return (uint8 *)((uint32)&_sbcmem_begin); 
}

#ifdef CONFIG_TWS	
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
uint8 sbc_encode_buff_malloc(void)
{
    os_printf("sbc_encode_buff_malloc:0x%x\r\n", bt_flag1_is_set(APP_FLAG_LINEIN));

    if(bt_flag1_is_set(APP_FLAG_LINEIN))
    {
        if (NULL != app_sbc.adc_pcm_raw_buff)
        {
            jfree(app_sbc.adc_pcm_raw_buff);
            app_sbc.adc_pcm_raw_buff = NULL;
        }
        
        if (NULL != app_sbc.adc_pcm_mid_buff)
        {
            jfree(app_sbc.adc_pcm_mid_buff);
            app_sbc.adc_pcm_mid_buff = NULL;
        }    
        
        #if CONFIG_ADC_DMA
        #else
        app_sbc.adc_pcm_raw_buff = (uint8 *)jmalloc(get_linein_buffer_length() * sizeof(uint8), M_ZERO);
        #endif
        
        app_sbc.adc_pcm_mid_buff = (uint8 *)jmalloc(512 * sizeof(uint8), M_ZERO);

        return TRUE;
    }
    
    return FALSE;        
}
#endif
uint8 sbc_decode_buff_malloc(void) // for sbc decode
{
    os_printf("sbc_decode_buff_malloc: %p-%p\r\n", app_sbc.sbc_encode_buffer, app_sbc.sbc_output);
    
#if 0
    os_printf("sbc_A%p-%p\r\n", app_sbc.sbc_encode_buffer, app_sbc.sbc_output);
    if(app_sbc.sbc_encode_buffer)
    {
        jfree(app_sbc.sbc_encode_buffer);
        app_sbc.sbc_encode_buffer = NULL;
    }
#endif
    
    if (NULL != app_sbc.sbc_output)
    {
        jfree(app_sbc.sbc_output);
        app_sbc.sbc_output = NULL;
    }

    app_sbc.sbc_encode_buffer = get_ram_buff(); 
//    app_sbc.sbc_encode_buffer = (uint8 *)jmalloc(SBC_ENCODE_BUFFER_LEN * sizeof(uint8), M_ZERO);
    //os_printf("333---jmalloc---sbc_encode_buffer\r\n");
    app_sbc.sbc_output = (uint8 *)jmalloc(512 * sizeof(uint8), M_ZERO);
    //os_printf("444---jmalloc---sbc_output\r\n");
    //os_printf("sbc_B%p-%p\r\n", app_sbc.sbc_encode_buffer, app_sbc.sbc_output);
    //os_printf("%s, %p-%p\r\n", __func__, app_sbc.sbc_encode_buffer, app_sbc.sbc_output);

    return TRUE;        
}

#else
void sbc_target_init_malloc_buff(void)
{
#if defined(BT_ONE_TO_MULTIPLE) && (CONFIG_APP_AEC  == 0)
    app_sbc.sbc_encode_buffer =(uint8 *) &sbc_encode_buff[0];
#else
    //if(NULL == app_sbc.sbc_encode_buffer)
    {
        //app_sbc.sbc_encode_buffer = (uint8 *)jmalloc(SBC_ENCODE_BUFFER_LEN * sizeof(uint8), M_ZERO);
        app_sbc.sbc_encode_buffer = get_ram_buff(); 
    }
#endif

    if(NULL == app_sbc.sbc_output) 
    {
        app_sbc.sbc_output = (uint8 *)jmalloc(512 * sizeof(uint8), M_ZERO);
    }
}
#endif

void sbc_target_deinit_jfree_buff(void)
{
    os_printf("sbc_target_deinit_jfree_buff:%p,%p\r\n",app_sbc.sbc_output,app_sbc.sbc_encode_buffer);
    if(app_sbc.sbc_output)
    {
        jfree(app_sbc.sbc_output);
        app_sbc.sbc_output = NULL;
        //os_printf("555---jfree---sbc_output\r\n");
    }
#if defined(BT_ONE_TO_MULTIPLE) && (CONFIG_APP_AEC  == 0)
    app_sbc.sbc_encode_buffer = NULL;
#else
#if 0
    if(app_sbc.sbc_encode_buffer)
    {
        jfree(app_sbc.sbc_encode_buffer);
        app_sbc.sbc_encode_buffer = NULL;
        os_printf("666---jfree---sbc_encode_buffer\r\n");
    }
#endif
#endif
}
#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
void sbc_encode_buff_free(void) // for sbc encode.
{
    os_printf("sbc_encode_buff_free:%p\r\n",app_sbc.sbc_encode_ptr);
    sbc_t *sbc = NULL;

    if (NULL != app_sbc.sbc_encode_ptr)
    {
        sbc = app_sbc.sbc_encode_ptr;
        if (NULL != sbc->priv)
        {
            jfree(sbc->priv);
            sbc->priv = NULL;
        }
        
        memset(app_sbc.sbc_encode_ptr, 0, sizeof(sbc_t));
        jfree(app_sbc.sbc_encode_ptr);
        app_sbc.sbc_encode_ptr = NULL;
    }
    
    if (NULL != app_sbc.adc_pcm_raw_buff)
    {
        jfree(app_sbc.adc_pcm_raw_buff);
        app_sbc.adc_pcm_raw_buff = NULL;        
    }
    
    if (NULL != app_sbc.adc_pcm_mid_buff)
    {
        jfree(app_sbc.adc_pcm_mid_buff);
        app_sbc.adc_pcm_mid_buff = NULL;
    }
        
    if (NULL != app_sbc.sbc_output_buffer)
    {
        jfree(app_sbc.sbc_output_buffer);
        app_sbc.sbc_output_buffer = NULL;
    }
    
}
#endif

void *sbc_get_sbc_ptr(void)
{
    return (void *)app_sbc.sbc_ptr;
}

void sbc_target_init( sbc_t *sbc )
{
    app_sbc.sbc_ptr = sbc;
    app_sbc.sbc_target_initial = 0;
#ifdef CONFIG_TWS
    app_sbc.src_pkt_num = 0;
#endif

    /* sbc_target_init_malloc_buff(); */
//os_printf("sbc_target_init()\r\n");
    return;
}

void sbc_target_deinit( void )
{
    sbc_mem_pool_deinit();
    sbc_target_deinit_jfree_buff();//modify by zjw for malloc space
#ifndef CONFIG_TWS	
    app_sbc.sbc_ptr = NULL;
#endif
}
void sbc_mem_free(void)
{
    os_printf("sbc_mem_free()\r\n");
    sbc_mem_pool_deinit();
    sbc_target_deinit_jfree_buff();
}
void sbc_stream_start_init( uint32 freq )
{
    app_sbc.sbc_first_try = 0;
    app_sbc.freq = freq;
    app_sbc.timer_cnt = 0;
}

/* sbc input node count monitor
input: 3/4-1/2 MAX_NODE

output:
000    0: 44.1 clk is slow;
001    1: 44.1 clk is norm;
010    2: 44.1clk is fast;
100    4: 48 clk is slow;
101    5: 48 clk is norm;
110    6: 48 clk is fast;

*/
uint8_t sbc_node_buff_monitor(void)
{
    uint8_t freq_b = 4 *(app_sbc.freq == 48000);
    if(app_sbc.sbc_ecout > SBC_FIRST_TRY_TIME)
    {
        return freq_b;          	  // Current local CLK is slow;
    }
    else if(app_sbc.sbc_ecout < (SBC_MEM_POOL_MAX_NODE >> 1))
    {
        return freq_b + 2;           // Current local CLK is fast;
    } 
    else
    {
        return freq_b + 1;           // Current local CLK is normal;
    }         
}
void sbc_dac_clk_tracking(uint32 step)
{
    if(get_flag_sbc_buffer_play()) //a2dp stream started...
    {
        app_sbc.timer_cnt += step;
        if(app_sbc.timer_cnt > 5*100)   // 20's
        {
            app_sbc.timer_cnt = 0;
            audio_dac_clk_process();
        }
    }
}

/*
 * status = sbc_encode_buffer_status(device_index),where:
 * <value> = 0: normal;
 * <value> = 1: host layer sbc node buffer is nearly full;
 * <value> = 2: host layer sbc node buffer is nearly empty;         
 */
uint8 RAM_CODE sbc_encode_buffer_status(void)
{
    uint8 status = 0;
    if(bt_flag1_is_set(APP_FLAG_HFP_CALLSETUP|APP_FLAG_SCO_CONNECTION))
        return 0;

    if (!bt_flag1_is_set(APP_FLAG_MUSIC_PLAY))
    	return 0;

    if((SBC_MEM_POOL_MAX_NODE - app_sbc.sbc_ecout) < 10)
        status = 1;  /* SBC node buffer nearly full !!! */
    else if(app_sbc.sbc_ecout < 10)
        status = 2; /* SBC node buffer nearly empty !!! */
    return status; 
        
}

uint8 RAM_CODE sbc_encode_and_audio_low(void)
{
    if (!bt_flag1_is_set(APP_FLAG_MUSIC_PLAY))
    	return 0;

    if ((app_sbc.sbc_ecout < 20) || (aud_get_buffer_size() > 512))
        return 1;

    return 0;
}

#ifdef CONFIG_TWS
static __INLINE__ int16_t smpl_saturate(int32_t s)
{
	if(s > 32767)
		return 32767;
	else if(s < -32768)
		return -32768;
	else
		return s;
}

void sbc_fill_encode_buffer_tws(struct mbuf *m, int len, int frames, uint32 btclk)
{
    int i = 0;
    int sbc_frame_len = len / frames;
    sbc_mem_node_t *node = NULL;

#ifndef BT_ONE_TO_MULTIPLE
    if( !bt_flag1_is_set( APP_FLAG_MUSIC_PLAY ) )
    {
        os_printf("out1 \n");
        return;
    }
#endif
    
#ifdef CONFIG_TWS
    if(bt_flag1_is_set(APP_FLAG_SCO_CONNECTION))
    {
        os_printf("out2 \n");
        return;
    }
#endif

    if (0 == app_sbc.sbc_target_initial)
    {
        sbc_decode_buff_malloc();
        sbc_mem_pool_init(sbc_frame_len);
        encode_buffer_full = 0;
        app_sbc.sbc_target_initial = 1;
        app_sbc.sbc_ecout = 0;
        app_sbc.sbc_clk_mode = 0;
    }
	 
#ifdef CONFIG_SBC_PROMPT
    if (!app_wave_playing())
#endif
    {
        uint8* psrc = (uint8*)&btclk;
        uint8* pdst;

        for(i = 0; i < frames; i++)
        {
            node = sbc_mem_pool_getNode(&app_sbc.sbc_mem_pool.freelist);
            if(NULL == node)
            {
                encode_buffer_full++;
                //os_printf("sbc full:%d\r\n",app_sbc.sbc_ecout);
                break;
            }

            #ifdef BEKEN_DEBUG
            encode_pkts++;
            #endif

            pdst = node->data_ptr + SBC_FRAME_LENGTH_MAX - 4;

            m_copydata( m, i*sbc_frame_len, sbc_frame_len, (void *)node->data_ptr);

            *pdst++ = psrc[0];
            *pdst++ = psrc[1];
            *pdst++ = psrc[2];
            *pdst++ = psrc[3];

            sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.uselist, node);

            btclk += 9;
            app_sbc.sbc_ecout++;
        }

        //if(get_tws_prim_sec()==TWS_PRIM_SEC_PRIMARY)
        //os_printf("E:%d\n",app_sbc.sbc_ecout );
    }
}
#else
void RAM_CODE sbc_fill_encode_buffer( struct mbuf *m, int len, int frames )
{
    int i;
    int sbc_frame_len = len/frames;
    sbc_mem_node_t *node = NULL;

#ifndef BT_ONE_TO_MULTIPLE
    if( !bt_flag1_is_set( APP_FLAG_MUSIC_PLAY ) )
        return;
#endif

    if( app_sbc.sbc_target_initial == 0 )
    {
        sbc_target_init_malloc_buff();
        sbc_mem_pool_init(  sbc_frame_len );
	 encode_buffer_full = 0;
        app_sbc.sbc_target_initial = 1;
        app_sbc.sbc_ecout = 0;
        app_sbc.sbc_clk_mode = 0;
    }
#ifdef CONFIG_SBC_PROMPT
    if(!app_wave_playing())
#endif
    {
        for( i = 0; i < frames; i++)
        {
            node = sbc_mem_pool_getNode( &app_sbc.sbc_mem_pool.freelist );
            if( node == NULL )
            {
                //os_printf("f:%d\r\n", encode_buffer_full);
                encode_buffer_full++;
                break;
            }
         #ifdef BEKEN_DEBUG
            encode_pkts++;
         #endif

            m_copydata( m, i*sbc_frame_len, sbc_frame_len, (void *)node->data_ptr);
            sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.uselist, node);
        }

        app_sbc.sbc_ecout += i;
    }
    if(flag_sbc_buffer_play)
    {
		//sbc_dac_clk_adjust();
        //audio_dac_clk_process();
    }
}
#endif

//void aud_src_from_bt_audio(void * arg);
#if (SBC_ERR_FRM_PROC == 1)
uint8 RAM_CODE mid_filter(uint8 *output,uint8 *filt_flag)
{
    //	int16 *smpl = (int16 *)output;
    uint16 *smplr = (uint16 *)output;
    int16 k;
    uint32 smpl_amp_sum = 0;
    uint16 flag_sum = 0;

    for(k=0; k<256; k++)
    {
        //smpl_amp_sum += ABS(*(smpl+2*k));
        //smpl_amp_sum += ABS(*(smpl+2*k+1));
        if((*smplr)!=0)
        {
            smpl_amp_sum = 1;
            break;
        }
        smplr ++;
    }
    for(k=1; k<(AUDIO_BUFF_LEN>>9); k++)
    {
        flag_sum += filt_flag[k];
        filt_flag[k-1] = filt_flag[k];
    }
    filt_flag[(AUDIO_BUFF_LEN>>9)-1] = (smpl_amp_sum > 0);
    flag_sum += filt_flag[(AUDIO_BUFF_LEN>>9)-1];
    if(flag_sum > SBC_MUTE_THR)
        return 1;
    else
        return 0;
}
#endif
#ifdef A2DP_SBC_DUMP_SHOW
void sbc_encode_frame_info(void)
{
    if(app_sbc.sbc_ptr != NULL)
    {
        os_printf("------SBC FRM INFO-----\r\n");
        os_printf("|     freq:%d\r\n",app_sbc.sbc_ptr->frequency);
        os_printf("|   blocks:%d\r\n",app_sbc.sbc_ptr->blocks);
        os_printf("| subbands:%d\r\n",app_sbc.sbc_ptr->subbands);
        os_printf("|     mode:%d\r\n",app_sbc.sbc_ptr->mode);
        os_printf("|  bitpool:%d\r\n",app_sbc.sbc_ptr->bitpool);
        os_printf("----------------------------\r\n");
    }
}
#endif
void RAM_CODE sbc_do_decode( void )
{
    int decode_frms, i, frame_len;
    sbc_mem_node_t *node = NULL;

    if(bt_flag1_is_set(APP_FLAG_POWERDOWN))
        return;
    if((app_sbc.sbc_ptr == NULL)|| (app_sbc.sbc_target_initial == 0))
    {
        return;
    }

    if(bt_flag1_is_set(APP_FLAG_SCO_CONNECTION))  //for the bug:the sbc still has remained data to fill in buff while sco has connection
    {
        do
        {
            node = sbc_mem_pool_getNode( &app_sbc.sbc_mem_pool.uselist );
            if(node != NULL)
            {
                sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
                app_sbc.sbc_ecout --;
            }
        }while(node != NULL);
        return;
    }
#ifdef CONFIG_TWS
    if (bt_flag1_is_set(APP_FLAG_HFP_OUTGOING))
        return;

    if (bt_flag2_is_set(APP_FLAG2_HFP_INCOMING))
        return;
#endif
                
    //sbc buffer cache, avoid "POP"
    if(0 == get_flag_sbc_buffer_play())
    {
        if(bt_flag1_is_set(APP_FLAG_WAVE_PLAYING))
        {
            do
            {
                node = sbc_mem_pool_getNode( &app_sbc.sbc_mem_pool.uselist );
                if(node != NULL)
                {
                    sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
                    app_sbc.sbc_ecout --;
                }
            }while(node != NULL);
            return;
        }

    #ifdef CONFIG_TWS
        if(bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
        {
            if(app_sbc.sbc_ecout == 0)
                return;
        }
        else
        {
             if(app_sbc.sbc_ecout < SBC_MEM_POOL_MAX_NODE / 2) 
                return;
        }
    #else
        if(app_sbc.sbc_ecout < SBC_FIRST_TRY_TIME) 
            return;
    #endif

    #ifndef CONFIG_TWS
    #ifdef SBC_FIRST_DISCARD_ENABLE
        /*discrad SBC begin data due to the data flow overflow*/
        if(sbc_first_discrad_flag == 1)
        {
            sbc_first_discard_count++;
            if(sbc_first_discard_count <= sbc_first_discard_data) //discard sbc_first_discard_data*45 note SBC data
            {
                for(i = 0; i < SBC_FIRST_TRY_TIME; i++)
                {
                    node = sbc_mem_pool_getNode( &app_sbc.sbc_mem_pool.uselist );
                    if(node != NULL)
                    {
                        sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
                        app_sbc.sbc_ecout --;
                    }
                }
                return;
            }
            sbc_first_discrad_flag = 0;
            sbc_first_discard_count = 0;
            sbc_first_discard_data = 0;
        }
    #endif
    #endif

        set_flag_sbc_buffer_play(1);
    #ifdef CONFIG_TWS	
        os_printf("===sbc node:%d,%d,tws:%d\r\n",get_sbc_mem_pool_node_left(),app_sbc.sbc_target_initial,get_tws_prim_sec());
    #else
        os_printf("===sbc cache:%d,%d\r\n",app_sbc.sbc_ecout,app_sbc.sbc_target_initial);
    #endif
    #if(CONFIG_AUD_FADE_IN_OUT == 1)
	#ifdef CONFIG_TWS
        if (!bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
	#endif
        {
        	set_aud_fade_in_out_state(AUD_FADE_IN);
        }
    #endif
	 /* if it selected 1to2 option, this sbc_re_init() is very important and necessary */
        sbc_re_init(app_sbc.sbc_ptr);
    #if (SBC_ERR_FRM_PROC == 1)
        memset(MidFilter_Flag,0,sizeof(MidFilter_Flag));
    #endif
    }

    if(app_sbc.sbc_ecout && ((decode_frms = aud_get_buffer_size() / 512) > 0))
    {
        #if (SBC_ERR_FRM_PROC == 0)
        uint32_t decoded_frames = 0;
        #endif

    #if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
        BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
    #endif

        decode_frms = decode_frms < app_sbc.sbc_ecout ? decode_frms : app_sbc.sbc_ecout;
         for(i = 0; i < decode_frms; i++)
        {
        #ifdef CONFIG_TWS
        {
            uint8*  p = &app_sbc.sbc_mem_pool.uselist.head->data_ptr[SBC_FRAME_LENGTH_MAX - 4];
            uint32  tp;
            int32_t r;

            tp = p[3];
            tp = (tp << 8) | p[2];
            tp = (tp << 8) | p[1];
            tp = (tp << 8) | p[0];

            r = audio_sync_process(tp);

            if(r == -1)
            {
                node = sbc_mem_pool_getNode(&app_sbc.sbc_mem_pool.uselist);
                sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
                decode_frms--;
                decoded_frames++;
                continue;
            }
            else if(r == 1)
            {
                memset(app_sbc.sbc_output, 0, 512);
                aud_fill_buffer(app_sbc.sbc_output, 512);
                wait_mailbox_ready(MCU2DSP_DAC_CTRL);
                decode_frms--;
                continue;
            }
        }
        #endif

            node = sbc_mem_pool_getNode(&app_sbc.sbc_mem_pool.uselist);

        #if (SBC_ERR_FRM_PROC == 1)
            if (NULL == node)
            {
                memset(app_sbc.sbc_output, 0, 512);
                encode_buffer_empty++;
            }
        #else
            if (NULL == node)
            {
            #ifdef BEKEN_DEBUG
                encode_buffer_empty++;
            #endif
                break;
            }
        #endif
            
        #if 0//ndef BT_ONE_TO_MULTIPLE
            /*
            if A is playing music,the app_sleep_func() can't be executed,and B may not enter sniff mode !
            */
            CLEAR_SLEEP_TICK;
        #endif

            CLEAR_PWDOWN_TICK;

        #if (SBC_ERR_FRM_PROC == 1)
            if(node != NULL)
            {
                frame_len = sbc_decode( app_sbc.sbc_ptr, node->data_ptr,
                                                        SBC_ENCODE_BUFFER_LEN,
                                                        app_sbc.sbc_output, 512, NULL,SBC_MODE);
                if(bt_flag2_is_set(APP_FLAG2_SW_MUTE))
                {
                    memset(app_sbc.sbc_output,0,512);
                }
            

                sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
            #ifdef BEKEN_DEBUG
                decode_pkts++;
            #endif
                app_sbc.sbc_ecout --;
                if( frame_len < 0 )
                {
                    os_printf("ERR: frame_len(=%d) < 0\r\n",frame_len);
                    continue;
                }
            }
        #else
            frame_len = sbc_decode(app_sbc.sbc_ptr, node->data_ptr, SBC_ENCODE_BUFFER_LEN, app_sbc.sbc_output, 512, NULL, SBC_MODE);

            if(bt_flag2_is_set(APP_FLAG2_SW_MUTE)) 
                memset(app_sbc.sbc_output,0,512);

        #ifdef BEKEN_DEBUG
            decode_pkts++;
        #endif
            decoded_frames++;

            sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
            if(frame_len < 0)
            {
                os_printf("ERR: frame_len < 0: %d\r\n", frame_len);
            #ifdef CONFIG_TWS
                memset(app_sbc.sbc_output, 0, pcm_bytes_per_sbc_frame());
            #else
                continue;
            #endif
            }
        #endif

            //if(!bt_flag1_is_set(APP_FLAG_WAVE_PLAYING) && !bt_flag2_is_set(APP_FLAG2_HFP_INCOMING))
            if(!bt_flag1_is_set(APP_FLAG_WAVE_PLAYING))
    	    {
            #if (SBC_ERR_FRM_PROC == 1)
                if(!mid_filter(app_sbc.sbc_output,MidFilter_Flag))
                    memset(app_sbc.sbc_output, 0, 512);
            #endif
				
            #if (CONFIG_AUD_FADE_IN_OUT == 1)
		  if(get_bt_dev_priv_work_flag())
                {
                    set_aud_fade_in_out_state(AUD_FADE_OUT);
                }
                else 
            #endif
                if(bt_flag1_is_set(APP_FLAG_HFP_OUTGOING) || bt_flag2_is_set(APP_FLAG2_HFP_INCOMING))
                {
                    memset(app_sbc.sbc_output,0,512);   
                }

            #ifdef BT_ONE_TO_MULTIPLE
                if(!app_wave_playing ())
            #endif
                {
                #ifdef CONFIG_TWS
                    int16 *smpl = (int16 *)app_sbc.sbc_output;
                    int16 k;
                    int32 tmp_smpl_L = 0;
                    app_tws_t app_tws_h = app_get_tws_handler();
                    if(bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE) && (app_tws_h->shareme<TWS_VENDOR_DEP_SHAREME_STEREO))
                    {
                        for(k=0;k<(512>>2);k++)
                        {
                            if (app_tws_h->shareme == TWS_VENDOR_DEP_SHAREME_LEFT)
                            {
                                if(get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)
                                    tmp_smpl_L = *(smpl + 2*k);
                                else
                                    tmp_smpl_L = *(smpl + 2*k + 1);
                            }
                            else
                            {
                                if(get_tws_prim_sec() == TWS_PRIM_SEC_PRIMARY)
                                    tmp_smpl_L = *(smpl + 2*k + 1);
                                else
                                    tmp_smpl_L = *(smpl + 2*k);
                            }

                            *(smpl + 2*k) = smpl_saturate(tmp_smpl_L);
                            *(smpl + 2*k + 1) = smpl_saturate(tmp_smpl_L);
                        }
                    }
                #endif
                    aud_fill_buffer(app_sbc.sbc_output, 512);
                    wait_mailbox_ready(MCU2DSP_DAC_CTRL);
                }
            }
        }

        #if (SBC_ERR_FRM_PROC == 0)
        app_sbc.sbc_ecout -= decoded_frames;
        #endif

    #if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
        BK3000_set_clock(CPU_CLK_SEL,CPU_OPTIMIZATION_DIV);
    #endif
    }
}

void sbc_discard_uselist_node(void)
{
    //uint32 i;
    sbc_mem_node_t *node = NULL;
    uint32 interrupts_info, mask;

    VICMR_disable_interrupts(&interrupts_info, &mask);

    //i = 0;
    do
    {
        node = sbc_mem_pool_getNode(&app_sbc.sbc_mem_pool.uselist);
        if(node)
        {
            sbc_mem_pool_freeNode(&app_sbc.sbc_mem_pool.freelist, node);
        #ifdef CONFIG_TWS
            app_sbc.sbc_ecout--;
            //i++;
            //os_printf("freeNode-%d, %d \n", i, app_sbc.sbc_ecout);
        #endif
        }
        else
        {
            //os_printf("freeNode_null-%d \n", app_sbc.sbc_ecout);
        }

    } while(node);

    VICMR_restore_interrupts(interrupts_info, mask);
	//os_printf("node:%d\r\n",i);
}

void sbc_first_play_init(uint8 enable,uint8 cnt)
{
    os_printf("sbc_first_play_init()\r\n");
    set_flag_sbc_buffer_play(0);
#ifdef SBC_FIRST_DISCARD_ENABLE
    sbc_first_discrad_flag = enable;
    sbc_first_discard_count = 0;
    sbc_first_discard_data = cnt;
#endif
}
#ifdef CONFIG_TWS
/* Get the encoded sbc frame node number left in the sbc_mem_pool.uselist */
inline uint16 get_sbc_mem_pool_node_left(void)
{    
    uint16 i = 0;

    sbc_mem_node_t *node = NULL;

    uint32 interrupts_info, mask;

    VICMR_disable_interrupts(&interrupts_info, &mask);    
    node = app_sbc.sbc_mem_pool.uselist.head;

    while (node)
    {
        i++;
        node = node->next;
    }
    VICMR_restore_interrupts(interrupts_info, mask);

    return i;
}

uint8 get_sbc_clk_mode(void)
{
    return app_sbc.sbc_clk_mode;
}

void sbc_stream_start_clear(void)
{
    app_sbc.src_pkt_num = 0;
}

void sbc_src_num_oper(uint8 oper, uint8 num)
{
//os_printf("A:%d\r\n", app_sbc.src_pkt_num);
    if (0 == oper) // plus
    {
        app_sbc.src_pkt_num += num;
    }
    else if (1 == oper)
    {
        app_sbc.src_pkt_num -= num;
        if(app_sbc.src_pkt_num >= num)
            app_sbc.src_pkt_num -= num;
        else
            app_sbc.src_pkt_num = 0;
    }

    return;
}

void sbc_src_num_set(uint8 num )
{
    app_sbc.src_pkt_num = num;
}

void sbc_notify_num( hci_link_t *link, uint8 num )
{
    //os_printf("%s\r\n", __func__);

    return;
}

int sbc_src_num( void )
{
    return (int8)app_sbc.src_pkt_num;
}

#endif

#ifdef TWS_CONFIG_LINEIN_BT_A2DP_SOURCE
void linein_sbc_encode_init(void)
{
    media_packet_sbc_t * pkt;
    sbc_t *sbc;
    sbc = (sbc_t *)jmalloc(sizeof(sbc_t), 0);
    
    if (!sbc)
        return ;
    
    os_printf("linein_sbc_encode_init: %p\n",sbc);
    sbc_init(sbc, 0, SBC_MODE);
    
    app_sbc.sbc_encode_ptr = sbc;
    
    app_sbc.sbc_target_initial = 0;
    app_sbc.src_pkt_num = 0;
    
    sbc_encode_buff_malloc();

    #if CONFIG_ADC_DMA
    #else
    rb_init( get_line_ringbuf(), get_linein_buffer_base(),0, get_linein_buffer_length());
    #endif

    app_sbc.sequence = 0;
    app_sbc.samples = 0;
    app_sbc.src_pkt_num = 0;

    app_sbc.in_frame_len = sbc_get_codesize(app_sbc.sbc_encode_ptr, SBC_MODE);
    app_sbc.out_frame_len = sbc_get_frame_length(app_sbc.sbc_encode_ptr, SBC_MODE);
    app_sbc.payload_len = L2CAP_MTU_DEFAULT - sizeof(rtp_header_t);//default mtu
    app_sbc.frames_per_packet = 8; //app_sbc.payload_len/ app_sbc.out_frame_len;
    app_sbc.sbc_output_buffer = (uint8 *)jmalloc(L2CAP_MTU_DEFAULT, 0);//		outputbuffer;

    if(app_sbc.sbc_output_buffer)
    {
        pkt = (media_packet_sbc_t*)app_sbc.sbc_output_buffer;
        memset((uint8 *)(&pkt->hdr), 0, sizeof(rtp_header_t) );

        pkt->hdr.ssrc = uw_htobe32(1);
        pkt->hdr.pt = 0x60;
        pkt->hdr.v = 2;
    }
//    app_sbc.sbc_output_buffer[0]=0x80;
}

void linein_sbc_alloc_free(void) // for sbc encode.
{
    os_printf("linein_sbc_alloc_free:%p-%p\r\n", app_sbc.sbc_encode_ptr, get_g_bt_sbc());
    app_handle_t app_h = app_get_sys_handler();

    jtask_stop(app_h->app_reset_task); 
    jtask_stop(app_h->app_auto_con_task); 
	
    if(!bt_flag1_is_set(APP_FLAG_POWERDOWN))
    	jtask_stop(app_h->app_tws_task); 

    bt_flag2_operate(APP_FLAG2_STEREO_LINEIN_TX, 0);

    sbc_encode_buff_free();   // for sbc encode

    sbc_target_deinit();    // for sbc decode
}

/*
Header + n* SBC frame
input:
		buffer: input buffer
		len:PCM DATA length
		mp: output buffer
		mp_data_len:packet length
		written:encoded data length
output:
		written: a2dp packet length
		return value: consumed PCM data length

added by beken
*/
static int	sbc_encode_mediapacket(
					uint32_t len, void *mp,
					uint32_t mp_data_len, uint32 *written)
{
    media_packet_sbc_t *mp_sbc = (media_packet_sbc_t *) mp;
    uint32_t consumed = 0;
    uint32_t encoded = 0;
    uint8_t frame_count = 0;
    int len_t;
    uint8_t *tmpbuffer = &(mp_sbc->frame_count);
    mp_data_len -= 1;//sizeof(mp_sbc->payload);
    tmpbuffer += 1;//skip
    while (len - consumed >= app_sbc.in_frame_len &&
                mp_data_len - encoded >= app_sbc.out_frame_len &&
                frame_count < app_sbc.frames_per_packet)
                //&& aud_get_buffer_size()>=app_sbc.in_frame_len
    {
        uint32_t read;
        uint32_t written_s;
        int16 aud_tmp[256];
        written_s =0;

    #if CONFIG_ADC_DMA
        len_t = dma_get_dst_data(DMA_CHN_4, (uint8 *)aud_tmp, app_sbc.in_frame_len);
    #else
        len_t = rb_get_buffer_with_length(get_line_ringbuf(), (uint8 *)aud_tmp, app_sbc.in_frame_len);
    #endif//        len_t = rb_get_buffer_with_length(get_line_ringbuf(), (uint8 *)aud_tmp, app_sbc.in_frame_len);
    
        if(len_t == 0)
            break;

        if (app_sbc.sbc_encode_ptr && (SBC_MODE_MONO != app_sbc.sbc_encode_ptr->mode))
        {
            int i;
            int16 *ptr_obj = (int16 *)app_sbc.adc_pcm_mid_buff;
            for (i = 0; i < 128; i++)
            {
                ptr_obj[i] = aud_tmp[2 * i + 0];

                if (0)//(bt_flag1_is_set(APP_FLAG_LINEIN) )
                {/* Chip bk3266 has a bug that the linein right channel is inverted, so here need to correct it. */
                    ptr_obj[128 + i] = aud_tmp[2 * i + 1] * (-1);
                }
                else
                {
                    ptr_obj[128 + i] = aud_tmp[2 * i + 1];
                }
            }
        }

        /* in while(): mp_data_len - encoded >= app_sbc.out_frame_len */
        read = sbc_encode(app_sbc.sbc_encode_ptr, (uint16_t *)&written_s,tmpbuffer+encoded,
								        app_sbc.in_frame_len,  // 512
                                        (void *)app_sbc.adc_pcm_mid_buff,SBC_MODE);
        
        if (read < 0)
        {
            break;
        }

        frame_count++;//encoded SBC frame count
        consumed += read;//consumed PCM data len
        encoded += written_s;//encoded SBC data len
    }

    *written = encoded + 1;//sizeof(mp_sbc->payload);
    mp_sbc->frame_count = frame_count;
    return consumed;

}

void a2dp_snk_stream_input(struct mbuf *m);
result_t RAM_CODE sbc_do_encode( void )
{
    uint8 *outbuf = app_sbc.sbc_output_buffer; //?????
    uint32 free_space = app_sbc.payload_len; //??????
    uint32 read,samples,consumed=0;
    result_t err = UWE_BUSY;
	//int i;
    //uint32 input_len=LINEIN_DATA_BUFFER_LEN- rb_get_buffer_size(get_line_ringbuf());
    //int32 input_len = rb_get_sample_number(get_line_ringbuf());

    //	if(!app_bt_is_flag2_set(APP_FLAG2_STEREO_STREAMING)) return err;
    if(!bt_flag2_is_set(APP_FLAG2_STEREO_LINEIN_TX)) 
        return err;
#if 0
    if (get_tws_flag(TWS_FLAG_MASTER_LINEIN_CLOSE_ENCODE))
    {		
        //for test
        //os_printf("C.%d\n",input_len);
        return -10;
    }
#endif
    //if((int8)sbc_src_num() >= 20)
    if((int8)sbc_src_num() >= 11)
    {
    //for test
    //os_printf("S.%d\n",sbc_src_num());
        return -12;
    }

#if CONFIG_ADC_DMA
    int32 input_len = dma_get_dst_data_size(DMA_CHN_4);
#else
    int32 input_len = rb_get_sample_number(get_line_ringbuf());
#endif

    //for test
    //os_printf("D.M%d\n",input_len);

    /*2-DH3: 2~369 bytes, 2-DH5: 2~681 bytes, best payload: 76 * 8 = 608 bytes.*/
    if(input_len < LINEIN_SBC_ENCODE_NODE_NUM * app_sbc.in_frame_len)
    //if(input_len < 6 * app_sbc.in_frame_len)
   //  if(input_len < 5 * app_sbc.in_frame_len)
    //if(input_len < 2 * app_sbc.in_frame_len)
        return err;
            
    media_packet_sbc_t *pkt = (media_packet_sbc_t*)outbuf;
    pkt->hdr.sequence_number = uw_htobe16(app_sbc.sequence);
    pkt->hdr.timestamp = uw_htobe32(app_sbc.samples);
    app_sbc.sequence++;
    //while(consumed < input_len)
    {
        uint32 written = 0;
        struct mbuf* m;

        read = sbc_encode_mediapacket(input_len-consumed, (void *)outbuf, free_space, &written);
        if(read <=0)
            return err;
        
        if(written > 0)
        {
            written += (12);//sizeof(rtp_header_t);
            m = m_get_flags(MT_DATA, M_PKTHDR, written);
            if(!m)
            {
                os_printf("do_encode, no mem!\n");
                return UWE_NOMEM;
            }
            //for(i=0;i<outbuf[12];i++)
            //{
            //if(outbuf[13+((written-13)/outbuf[12])*i]!=0x9c)
            //	os_printf("%d,%d,%x\n",written,outbuf[12],outbuf[13+((written-13)/outbuf[12])*i]);
            //}
            //os_printf("%d\n",written);
            m_copyback(m,0,written,outbuf);
            a2dp_snk_stream_input(m);
            if(app_sbc.sbc_encode_ptr->mode == 0x03/*SBC_MODE_JOINT_STEREO*/)
                samples = read >> 2;// /4
            else
                samples = read >> 1;// /2
            app_sbc.samples += samples;
            consumed += read;
        }
    }
    return err;
}

uint8 *get_linein_buffer_base(void)
{
	return app_sbc.adc_pcm_raw_buff;    
}

uint32 get_linein_buffer_length(void)
{
    return 512*4; //SBC_ENCODE_BUFFER_LEN;
}
#endif


#ifdef CONFIG_SBC_PROMPT
void app_wave_file_sbc_fill(void)
{
    static sbc_t* wave_sbc_ptr = NULL;
    static uint8_t* in_ptr = NULL;
    static uint8_t* out_ptr = NULL;
    uint8_t input_len = app_wave_get_blocksize();
    int i,decode_frms,j;
    int16_t ret;
    if(!app_wave_playing()
        ||app_wave_check_type(INTER_WAV)
        ||app_wave_check_type(EXT_WAV)
    )
        return;

    if (NULL == wave_sbc_ptr)
    {
        wave_sbc_ptr = (sbc_t *)jmalloc(sizeof(sbc_t), 0);
        if (!wave_sbc_ptr)
        {
            return ;
        }
        os_printf("sbc_prompt:%x\n",wave_sbc_ptr);
        wave_sbc_init(wave_sbc_ptr, 0,SBC_MODE);
        in_ptr = jmalloc(64,0); 
        out_ptr = jmalloc(256,0); 
    }
    decode_frms = (aud_get_buffer_size() + 2)/512;
    for(i=0;i<decode_frms;i++)
    {
        if(app_wave_check_status(PROMPT_EMPTY))
        {
            if (NULL == wave_sbc_ptr)
                return;
            
            if (NULL == wave_sbc_ptr->priv)
                return;
            
            wave_sbc_ptr->priv = NULL;
            
            if (NULL != wave_sbc_ptr)
            {
                jfree(wave_sbc_ptr);
                wave_sbc_ptr = NULL;
            }

            if (NULL != in_ptr)
            {
                jfree(in_ptr);
                in_ptr = NULL;
            }
            
            if (NULL != out_ptr)
            {
                jfree(out_ptr);
                out_ptr = NULL;
            }

            //app_wave_set_status(PROMPT_STOP);
            app_wave_set_status(PROMPT_CLOSED);
            app_wave_file_play_stop();
            break;
        }
        else if(app_wave_check_status(PROMPT_FILL_ZERO))
        {
            app_wave_fill_sbc_node(in_ptr);
            memset(out_ptr,0,256);
            aud_fill_buffer( out_ptr,256);
        }
        else
        {
            app_wave_fill_sbc_node(in_ptr);
            ret = sbc_decode(wave_sbc_ptr,in_ptr,input_len,out_ptr,256,NULL,SBC_MODE);
            if(ret < 0)
            {
                memset(out_ptr,0,256);    
            }
            aud_fill_buffer( out_ptr,256);
        }
    }

}
/*
void app_wave_file_sbc_stop(void)
{
#ifdef BT_ONE_TO_MULTIPLE
	if(!(a2dp_has_music()|(bt_flag1_is_set(APP_FLAG_SCO_CONNECTION))))
#else
	if(!bt_flag1_is_set(  APP_FLAG_MUSIC_PLAY
						 |APP_FLAG_SCO_CONNECTION
						 ))
#endif
	{
            sbc_t *sbc;
            sbc = app_sbc.sbc_ptr;            
            sbc_target_deinit();
            sbc_finish(sbc);
	}
	flag_sbc_buffer_play = 0;
	#ifdef SBC_FIRST_DISCARD_ENABLE
		sbc_first_discrad_flag = 1;
		sbc_first_discard_count = 0;
		sbc_first_discard_data = 1; //empirical value by measured when recover from prompt wav
	#endif

	app_wave_file_play_stop();
}
*/
#endif

// EOF
