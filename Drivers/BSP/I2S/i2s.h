/**
 ****************************************************************************************************
 * @file        i2s.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2021-11-16
 * @brief       I2S 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 探索者 F407开发板
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
 
#ifndef __I2S_H
#define __I2S_H
#include "./SYSTEM/SYS/sys.h"


/* 引脚复用为I2S */
#define GPIO_AF_I2S_SPI                 GPIO_AF5_SPI2
#define GPIO_AF_I2S_EXT_SPI             GPIO_AF6_I2S2ext


#define I2S_LRCK_GPIO_PORT              GPIOB
#define I2S_LRCK_GPIO_PIN               GPIO_PIN_12
#define I2S_LRCK_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)     /* PB口时钟使能 */

#define I2S_SCLK_GPIO_PORT              GPIOB
#define I2S_SCLK_GPIO_PIN               GPIO_PIN_13
#define I2S_SCLK_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)     /* PB口时钟使能 */

#define I2S_SDOUT_GPIO_PORT             GPIOC
#define I2S_SDOUT_GPIO_PIN              GPIO_PIN_2
#define I2S_SDOUT_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)     /* PC口时钟使能 */

#define I2S_SDIN_GPIO_PORT              GPIOC
#define I2S_SDIN_GPIO_PIN               GPIO_PIN_3
#define I2S_SDIN_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)     /* PC口时钟使能 */

#define I2S_MCLK_GPIO_PORT              GPIOC
#define I2S_MCLK_GPIO_PIN               GPIO_PIN_6
#define I2S_MCLK_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)     /* PC口时钟使能 */

#define I2S_SPI                         SPI2
#define I2S_SPI_CLK_ENABLE()            do{ __HAL_RCC_SPI2_CLK_ENABLE(); }while(0)      /* I2S2时钟使能*/

#define I2S_TX_DMASx                    DMA1_Stream4
#define I2S_TX_DMASx_Channel            DMA_CHANNEL_0
#define I2S_TX_DMASx_Handle             DMA1_Stream4_IRQHandler
#define I2S_TX_DMASx_IRQ                DMA1_Stream4_IRQn
#define I2S_TX_DMASx_FLAG               DMA_FLAG_TCIF0_4
#define I2S_TX_DMA_CLK_ENABLE()         do{ __HAL_RCC_DMA1_CLK_ENABLE(); }while(0)   /* I2S2 TX DMA时钟使能 */


/*****************************************************************************************************/

extern void (*i2s_tx_callback)(void);   /* I2S TX回调函数指针  */

void i2s_init(uint32_t i2s_standard, uint32_t i2s_mode, uint32_t i2s_clock_polarity, uint32_t i2s_dataformat);  /* I2S初始化 */
void i2s_dma_enable(void);      /* 开启I2S的DMA功能 */
uint8_t i2s_samplerate_set(uint32_t samplerate);    /* 设置I2S的采样率 */
void i2s_tx_dma_init(uint8_t* buf0, uint8_t *buf1, uint16_t num);   /* I2S TX DMA配置 */
void i2s_play_start(void);      /* I2S开始播放 */
void i2s_play_stop(void);       /* I2S停止播放 */

#endif





















