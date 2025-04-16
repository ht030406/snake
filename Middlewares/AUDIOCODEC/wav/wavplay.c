/**
 ****************************************************************************************************
 * @file        wavplay.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2021-11-16
 * @brief       wav解码 代码
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
 * 1,支持16位/24位WAV文件播放
 * 2,最高可以支持到192K/24bit的WAV格式.
 *
 ****************************************************************************************************
 */
 
#include "./AUDIOCODEC/wav/wavplay.h"
#include "./APP/audioplay.h"
#include "./SYSTEM/USART/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./MALLOC/malloc.h"
#include "./FATFS/source/ff.h"
#include "./BSP/I2S/i2s.h"
#include "./BSP/ES8388/es8388.h"
#include "./BSP/KEY/key.h"
#include "./BSP/LED/led.h"
#include "game.h"

__wavctrl wavctrl;                      /* WAV控制结构体 */

volatile uint8_t wavtransferend = 0;    /* i2s传输完成标志 */
volatile uint8_t wavwitchbuf = 0;       /* i2sbufx指示标志 */

/**
 * @brief       WAV解析初始化
 * @param       fname : 文件路径+文件名
 * @param       wavx  : 信息存放结构体指针
 * @retval      0,打开文件成功
 *              1,打开文件失败
 *              2,非WAV文件
 *              3,DATA区域未找到
 */
uint8_t wav_decode_init(char* fname, __wavctrl* wavx)
{
    FIL *ftemp;
    uint8_t *buf; 
    uint32_t br = 0;
    uint8_t res = 0;

    ChunkRIFF *riff;
    ChunkFMT *fmt;
    ChunkFACT *fact;
    ChunkDATA *data;
    
    ftemp = (FIL*)mymalloc(SRAMIN, sizeof(FIL));
    buf = mymalloc(SRAMIN, 512);
    
    if (ftemp && buf)    /* 内存申请成功 */
    {
        res = f_open(ftemp, (TCHAR*)fname, FA_READ);    /* 打开文件 */
        
        if (res == FR_OK)
        {
            f_read(ftemp, buf, 512, &br);               /* 读取512字节在数据 */
            riff = (ChunkRIFF *)buf;                    /* 获取RIFF块 */
            if (riff->Format == 0x45564157)             /* 是WAV文件 */
            {
                fmt = (ChunkFMT *)(buf + 12);           /* 获取FMT块 */
                fact = (ChunkFACT *)(buf + 12 + 8 + fmt->ChunkSize);                    /* 读取FACT块 */
                
                if (fact->ChunkID == 0x74636166 || fact->ChunkID == 0x5453494C)
                {
                    wavx->datastart = 12 + 8 + fmt->ChunkSize + 8 + fact->ChunkSize;    /* 具有fact/LIST块的时候(未测试) */
                }
                else
                {
                    wavx->datastart = 12 + 8 + fmt->ChunkSize;
                }
                
                data = (ChunkDATA *)(buf + wavx->datastart);    /* 读取DATA块 */
                
                if (data->ChunkID == 0x61746164)                /* 解析成功! */
                {
                    wavx->audioformat = fmt->AudioFormat;       /* 音频格式 */
                    wavx->nchannels = fmt->NumOfChannels;       /* 通道数 */
                    wavx->samplerate = fmt->SampleRate;         /* 采样率 */
                    wavx->bitrate = fmt->ByteRate * 8;          /* 得到位速 */
                    wavx->blockalign = fmt->BlockAlign;         /* 块对齐 */
                    wavx->bps = fmt->BitsPerSample;             /* 位数,16/24/32位 */
                    
                    wavx->datasize = data->ChunkSize;           /* 数据块大小 */
                    wavx->datastart = wavx->datastart + 8;      /* 数据流开始的地方. */
                     
                    printf("wavx->audioformat:%d\r\n", wavx->audioformat);
                    printf("wavx->nchannels:%d\r\n", wavx->nchannels);
                    printf("wavx->samplerate:%d\r\n", wavx->samplerate);
                    printf("wavx->bitrate:%d\r\n", wavx->bitrate);
                    printf("wavx->blockalign:%d\r\n", wavx->blockalign);
                    printf("wavx->bps:%d\r\n", wavx->bps);
                    printf("wavx->datasize:%d\r\n", wavx->datasize);
                    printf("wavx->datastart:%d\r\n", wavx->datastart);  
                }
                else
                {
                    res = 3;    /* data区域未找到. */
                }
            }
            else
            {
                res = 2;        /* 非wav文件 */
            }
        }
        else
        {
            res = 1;            /* 打开文件错误 */
        }
    }
    
    f_close(ftemp);
    
    myfree(SRAMIN, ftemp);      /* 释放内存 */
    myfree(SRAMIN, buf); 
    
    return 0;
}

/**
 * @brief       填充buf
 * @param       buf  : 填充区
 * @param       size : 填充数据量
 * @param       bits : 位数(16/24)
 * @retval      读取到的数据长度
 */
uint32_t wav_buffill(uint8_t *buf, uint16_t size, uint8_t bits)
{
    uint16_t readlen = 0;
    uint32_t bread;
    uint16_t i;
    uint8_t *p;
    
    if (bits == 24)                 /* 24bit音频,需要处理一下 */
    {
        readlen = (size / 4) * 3;   /* 此次要读取的字节数 */
        
        f_read(g_audiodev.file, g_audiodev.tbuf, readlen, (UINT*)&bread);   /* 读取数据 */
        p = g_audiodev.tbuf;
        
        for (i = 0; i < size;)
        {
            buf[i++] = p[1];
            buf[i] = p[2]; 
            i += 2;
            buf[i++] = p[0];
            p += 3;
        }
        
        bread = (bread * 4) / 3;    /* 填充后的大小. */
    }
    else
    {
        f_read(g_audiodev.file, buf, size, (UINT*)&bread);  /* 16bit音频,直接读取数据 */
        
        if (bread < size)           /* 不够数据了,补充0 */
        {
            for (i = bread; i < size - bread; i++)
            {
                buf[i] = 0;
            }
        }
    }
    
    return bread;
}

/**
 * @brief       WAV播放时,I2S DMA传输回调函数
 * @param       无
 * @retval      无
 */
void wav_i2s_dma_tx_callback(void) 
{
    uint16_t i;

    if (DMA1_Stream4->CR & (1 << 19))
    {
        wavwitchbuf = 0;
        
        if ((g_audiodev.status & 0x01) == 0)
        {
            for (i = 0; i < WAV_I2S_TX_DMA_BUFSIZE; i++)    /* 暂停 */
            {
                g_audiodev.i2sbuf1[i] = 0;                  /* 填充0 */
            }
        }
    }
    else 
    {
        wavwitchbuf = 1;

        if ((g_audiodev.status & 0x01) == 0)
        {
            for (i = 0; i < WAV_I2S_TX_DMA_BUFSIZE; i++)    /* 暂停 */
            {
                g_audiodev.i2sbuf2[i] = 0;                  /* 填充0 */
            }
        }
    }
    
    wavtransferend = 1;
}

/**
 * @brief       获取当前播放时间
 * @param       fname : 文件指针
 * @param       wavx  : wavx播放控制器
 * @retval      无
 */
void wav_get_curtime(FIL *fx, __wavctrl *wavx)
{
    long long fpos;

    wavx->totsec = wavx->datasize / (wavx->bitrate / 8);    /* 歌曲总长度(单位:秒) */
    fpos = fx->fptr-wavx->datastart;                        /* 得到当前文件播放到的地方 */
    wavx->cursec = fpos * wavx->totsec / wavx->datasize;    /* 当前播放到第多少秒了? */
}

/**
 * @brief       播放某个wav文件
 * @param       fname : 文件路径+文件名
 * @retval      KEY0_PRES,错误
 *              KEY1_PRES,打开文件失败
 *              其他,非WAV文件
 */
uint8_t wav_play_song(char* fname)
{
    uint8_t key;
    uint8_t t = 0; 
    uint8_t res;  
    uint32_t fillnum; 
    
    g_audiodev.file = (FIL*)mymalloc(SRAMIN, sizeof(FIL));
    g_audiodev.i2sbuf1 = mymalloc(SRAMIN, WAV_I2S_TX_DMA_BUFSIZE);
    g_audiodev.i2sbuf2 = mymalloc(SRAMIN, WAV_I2S_TX_DMA_BUFSIZE);
    g_audiodev.tbuf = mymalloc(SRAMIN, WAV_I2S_TX_DMA_BUFSIZE);
    
    if (g_audiodev.file && g_audiodev.i2sbuf1 && g_audiodev.i2sbuf2 && g_audiodev.tbuf)
    { 
        res = wav_decode_init(fname, &wavctrl); /* 得到文件的信息 */
        
        if (res == 0)   /* 解析文件成功 */
        {
            if (wavctrl.bps == 16)
            {
                es8388_i2s_cfg(0, 3);   /* 飞利浦标准,16位数据长度 */
                i2s_init(I2S_STANDARD_PHILIPS, I2S_MODE_MASTER_TX, I2S_CPOL_LOW, I2S_DATAFORMAT_16B_EXTENDED); /* 飞利浦标准,主机发送,时钟低电平有效,16位扩展帧长度 */
            }
            else if (wavctrl.bps == 24)
            {
                es8388_i2s_cfg(0, 0);   /* 飞利浦标准,24位数据长度 */
                i2s_init(I2S_STANDARD_PHILIPS, I2S_MODE_MASTER_TX, I2S_CPOL_LOW, I2S_DATAFORMAT_24B);          /* 飞利浦标准,主机发送,时钟低电平有效,24位长度 */
            }
            
            i2s_samplerate_set(wavctrl.samplerate);     /* 设置采样率 */
            i2s_tx_dma_init(g_audiodev.i2sbuf1, g_audiodev.i2sbuf2, WAV_I2S_TX_DMA_BUFSIZE / 2);   /* 配置TX DMA */
            i2s_tx_callback = wav_i2s_dma_tx_callback;  /* 回调函数指wav_i2s_dma_callback */
            audio_stop();
            
            res = f_open(g_audiodev.file, (TCHAR*)fname, FA_READ);    /* 打开文件 */
            
            if (res == 0)
            {
                f_lseek(g_audiodev.file, wavctrl.datastart);          /* 跳过文件头 */
                
                fillnum = wav_buffill(g_audiodev.i2sbuf1, WAV_I2S_TX_DMA_BUFSIZE, wavctrl.bps);
                fillnum = wav_buffill(g_audiodev.i2sbuf2, WAV_I2S_TX_DMA_BUFSIZE, wavctrl.bps);
                
                audio_start();  
                
                while (res == 0)
                { 
                    while(wavtransferend == 0);             // 等待wav传输完成; 
                    
                    wavtransferend = 0;
                    
                    if (fillnum != WAV_I2S_TX_DMA_BUFSIZE)  // 播放结束? 
                    {
                        res = KEY0_PRES;
                        break;
                    }
                    
                    if (wavwitchbuf)
                    {
                        fillnum = wav_buffill(g_audiodev.i2sbuf2, WAV_I2S_TX_DMA_BUFSIZE, wavctrl.bps);   // 填充buf2 
                    }
                    else
                    {
                        fillnum = wav_buffill(g_audiodev.i2sbuf1, WAV_I2S_TX_DMA_BUFSIZE, wavctrl.bps);   // 填充buf1 
                    }
                    
                    while (1)
                    {
                        key = key_scan(0);
                        if (key == WKUP_PRES)   // 暂停 
                        {
                            if (g_audiodev.status & 0x01)
                            {
                                g_audiodev.status &= ~(1 << 0);
                            }
                            else 
                            {
                                g_audiodev.status |= 0x01;
                            }
                        }
                        
                        if (key == KEY2_PRES || key == KEY0_PRES)   // 下一曲/上一曲 
                        {
                            res = key;
                            break;
                        }
                        
                        wav_get_curtime(g_audiodev.file, &wavctrl); // 得到总时间和当前播放的时间 
                        audio_msg_show(wavctrl.totsec, wavctrl.cursec, wavctrl.bitrate);
                        
                        t++;
                        if (t == 20)
                        {
                            t = 0 ;
                            LED0_TOGGLE();
                        }
                        
                        if ((g_audiodev.status & 0x01) == 0)
                        {
                            delay_ms(10);
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                audio_stop();
            }
            else
            {
                res = 0xFF;
            }
        }
        else
        {
            res = 0xFF;
        }
    }
    else
    {
        res = 0xFF;
    }
    
    myfree(SRAMIN, g_audiodev.tbuf);      /* 释放内存 */
    myfree(SRAMIN, g_audiodev.i2sbuf1);   /* 释放内存 */
    myfree(SRAMIN, g_audiodev.i2sbuf2);   /* 释放内存 */
    myfree(SRAMIN, g_audiodev.file);      /* 释放内存 */
    
    return res;
}



