/**
 ****************************************************************************************************
 * @file        audioplay.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2021-11-16
 * @brief       音乐播放器 应用代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20211116
 * 第一次发布
 *
 ****************************************************************************************************
 */

#ifndef __AUDIOPLAY_H
#define __AUDIOPLAY_H
#include "./SYSTEM/SYS/sys.h"
#include "./FATFS/source/ff.h"
#include "./AUDIOCODEC/wav/wavplay.h"


#define I2S_TX_DMA_BUF_SIZE    10240        /* 定义TX DMA 数组大小(播放192Kbps@24bit的时候,需要设置8192大才不会卡) */


/* 音乐播放控制器 */
typedef __packed struct
{  
    uint8_t *i2sbuf1;           /* I2S解码的BUF */
    uint8_t *i2sbuf2;
    uint8_t *tbuf;              /* 临时数组,仅在24bit解码的时候需要用到 */
    FIL *file;                  /* 音频文件指针 */

    uint8_t status;             /* bit0:0,暂停播放;1,继续播放 */
                                /* bit1:0,结束播放;1,开启播放 */
}__audiodev;

extern __audiodev g_audiodev;   /* 音乐播放控制器 */

void wav_i2s_dma_callback(void);
void audio_start(void);         /* 开始音频播放 */
void audio_stop(void);          /* 停止音频播放 */
uint16_t audio_get_tnum(char *path);                                        /* 得到path路径下,目标文件的总个数 */
void audio_index_show(uint16_t index, uint16_t total);                      /* 显示曲目索引 */
void audio_msg_show(uint32_t totsec, uint32_t cursec, uint32_t bitrate);    /* 显示播放时间,比特率 信息 */
void audio_play(void);          /* 播放音乐 */
uint8_t audio_play_song(char* fname);                                       /* 播放某个音频文件 */

#endif

