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
 
#include "./BSP/I2S/i2s.h"  
#include "./SYSTEM/delay/delay.h"


I2S_HandleTypeDef g_i2s_handle;         /* I2S句柄 */
DMA_HandleTypeDef g_i2s_txdma_handle;   /* I2S发送DMA句柄 */

/**
 * @brief       I2S初始化
 * @param       i2s_standard        : I2S标准
 *   @note      可以设置            : I2S_STANDARD_PHILIPS/I2S_STANDARD_MSB/
 *                                    I2S_STANDARD_LSB/I2S_STANDARD_PCM_SHORT/I2S_STANDARD_PCM_LONG
 * @param       i2s_mode            : I2S工作模式
 *   @note      可以设置            : I2S_MODE_SLAVE_TX/I2S_MODE_SLAVE_RX/I2S_MODE_MASTER_TX/I2S_MODE_MASTER_RX
 * @param       i2s_clock_polarity  : 空闲状态时钟电平
 * @param       i2s_dataformat      : 数据长度
 *   @note      可以设置            : I2S_DATAFORMAT_16B/I2S_DATAFORMAT_16B_EXTENDED/I2S_DATAFORMAT_24B/I2S_DATAFORMAT_32B
 * @retval      无
 */
void i2s_init(uint32_t i2s_standard, uint32_t i2s_mode, uint32_t i2s_clock_polarity, uint32_t i2s_dataformat)
{
    g_i2s_handle.Instance = I2S_SPI;
    g_i2s_handle.Init.Mode = i2s_mode;                     /* IIS模式 */
    g_i2s_handle.Init.Standard = i2s_standard;             /* IIS标准 */
    g_i2s_handle.Init.DataFormat = i2s_dataformat;         /* IIS数据长度 */
    g_i2s_handle.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;  /* 主时钟输出使能 */
    g_i2s_handle.Init.AudioFreq = I2S_AUDIOFREQ_DEFAULT;   /* IIS频率设置 */
    g_i2s_handle.Init.CPOL = i2s_clock_polarity;           /* 空闲状态时钟电平 */
    g_i2s_handle.Init.ClockSource = I2S_CLOCK_PLL;         /* IIS时钟源为PLL */
    HAL_I2S_Init(&g_i2s_handle); 

    I2S_SPI->CR2 |= 1<<1;               /* SPI2/I2S2 TX DMA请求使能 */
    __HAL_I2S_ENABLE(&g_i2s_handle);    /* 使能I2S2 */
}

/**
 * @brief       I2S底层驱动，时钟使能，引脚配置，DMA配置
 * @note        此函数会被HAL_I2S_Init()调用
 * @param       hi2s:I2S句柄
 * @retval      无
 */
void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s)
{
    GPIO_InitTypeDef gpio_init_struct;

    I2S_SPI_CLK_ENABLE();           /* 使能SPI2/I2S2时钟 */
    I2S_LRCK_GPIO_CLK_ENABLE();     /* 使能I2S_LRCK时钟 */
    I2S_SCLK_GPIO_CLK_ENABLE();     /* 使能I2S_SCLK时钟 */
    I2S_SDOUT_GPIO_CLK_ENABLE();    /* 使能I2S_SDOUT时钟 */
    I2S_SDIN_GPIO_CLK_ENABLE();     /* 使能I2S_SDIN时钟 */
    I2S_MCLK_GPIO_CLK_ENABLE();     /* 使能I2S_MCLK时钟 */

    gpio_init_struct.Pin = I2S_LRCK_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* 推挽复用 */
    gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_HIGH;               /* 高速 */
    gpio_init_struct.Alternate = GPIO_AF_I2S_SPI;           /* 复用为SPI/I2S */
    HAL_GPIO_Init(I2S_LRCK_GPIO_PORT, &gpio_init_struct);   /* 初始化I2S_LRCK引脚 */

    gpio_init_struct.Pin = I2S_SCLK_GPIO_PIN; 
    HAL_GPIO_Init(I2S_SCLK_GPIO_PORT,&gpio_init_struct);    /* 初始化I2S_SCLK引脚 */

    gpio_init_struct.Pin = I2S_SDIN_GPIO_PIN; 
    HAL_GPIO_Init(I2S_SDIN_GPIO_PORT,&gpio_init_struct);    /* 初始化I2S_SDIN引脚 */
    
    gpio_init_struct.Pin = I2S_MCLK_GPIO_PIN; 
    HAL_GPIO_Init(I2S_MCLK_GPIO_PORT,&gpio_init_struct);    /* 初始化I2S_MCLK引脚 */

    gpio_init_struct.Pin = I2S_SDOUT_GPIO_PIN; 
    gpio_init_struct.Alternate = GPIO_AF_I2S_EXT_SPI;       /* 复用为I2Sext */
    HAL_GPIO_Init(I2S_SDOUT_GPIO_PORT,&gpio_init_struct);   /* 初始化I2S_SDOUT引脚 */
}

/**
 * 采样率计算公式:Fs=I2SxCLK/[256*(2*I2SDIV+ODD)]
 * I2SxCLK=(HSE/pllm)*PLLI2SN/PLLI2SR
 * 一般HSE=8Mhz 
 * pllm:在Sys_Clock_Set设置的时候确定，一般是8
 * PLLI2SN:一般是192~432
 * PLLI2SR:2~7
 * I2SDIV:2~255
 * ODD:0/1
 * I2S分频系数表@pllm=8,HSE=8Mhz,即vco输入频率为1Mhz
 * 表格式:采样率/10,PLLI2SN,PLLI2SR,I2SDIV,ODD
 */
const uint16_t I2S_PSC_TBL[][5]=
{
    {   800, 256, 5, 12, 1 },   /* 8Khz采样率 */
    {  1102, 429, 4, 19, 0 },   /* 11.025Khz采样率 */
    {  1600, 213, 2, 13, 0 },   /* 16Khz采样率 */
    {  2205, 429, 4,  9, 1 },   /* 22.05Khz采样率 */
    {  3200, 213, 2,  6, 1 },   /* 32Khz采样率 */
    {  4410, 271, 2,  6, 0 },   /* 44.1Khz采样率 */
    {  4800, 258, 3,  3, 1 },   /* 48Khz采样率 */
    {  8820, 316, 2,  3, 1 },   /* 88.2Khz采样率 */
    {  9600, 344, 2,  3, 1 },   /* 96Khz采样率 */
    { 17640, 361, 2,  2, 0 },   /* 176.4Khz采样率 */
    { 19200, 393, 2,  2, 0 },   /* 192Khz采样率 */
};

/**
 * @brief       开启I2S的DMA功能
 * @param       无
 * @retval      无
 */
void i2s_dma_enable(void)
{
    uint32_t tempreg = 0;
    tempreg = I2S_SPI->CR2;     /* 先读出以前的设置 */
    tempreg |= 1 << 1;          /* 使能DMA */
    I2S_SPI->CR2 = tempreg;     /* 写入CR2寄存器中 */
}

/**
 * @brief       设置I2S的采样率
 * @param       samplerate:采样率, 单位:Hz
 * @retval      0,设置成功
 *              1,无法设置
 */
uint8_t i2s_samplerate_set(uint32_t samplerate)
{   
    uint8_t i = 0; 
    uint32_t tempreg = 0;

    RCC_PeriphCLKInitTypeDef rcc_i2s_clk_init;

    for (i = 0; i < (sizeof(I2S_PSC_TBL) / 10); i++)    /* 看看该采样率是否可以支持 */
    {
        if ((samplerate / 10) == I2S_PSC_TBL[i][0])
        {
            break;
        }
    }
    if (i == (sizeof(I2S_PSC_TBL) / 10))
    {
        return 1;   /* 找不到 */
    }

    rcc_i2s_clk_init.PeriphClockSelection = RCC_PERIPHCLK_I2S;        /* 外设时钟源选择 */
    rcc_i2s_clk_init.PLLI2S.PLLI2SN = (uint32_t)I2S_PSC_TBL[i][1];    /* 设置PLLI2SN */
    rcc_i2s_clk_init.PLLI2S.PLLI2SR = (uint32_t)I2S_PSC_TBL[i][2];    /* 设置PLLI2SR */
    HAL_RCCEx_PeriphCLKConfig(&rcc_i2s_clk_init);                     /* 设置时钟 */

    RCC->CR |= 1 << 26;                 /* 开启I2S时钟 */
    while((RCC->CR & 1 << 27) == 0);    /* 等待I2S时钟开启成功. */
    tempreg = I2S_PSC_TBL[i][3] << 0;   /* 设置I2SDIV */
    tempreg |= I2S_PSC_TBL[i][4] << 8;  /* 设置ODD位 */
    tempreg |= 1 << 9;                  /* 使能MCKOE位,输出MCK */
    I2S_SPI->I2SPR = tempreg;           /* 设置I2SPR寄存器 */

    return 0;
}

/**
 * @brief       I2S TX DMA配置
 *  @note       设置为双缓冲模式,并开启DMA传输完成中断
 * @param       buf0 : M0AR地址.
 * @param       buf1 : M1AR地址.
 * @param       num  : 每次传输数据量
 * @retval      无
 */
void i2s_tx_dma_init(uint8_t* buf0, uint8_t *buf1, uint16_t num)
{  
    I2S_TX_DMA_CLK_ENABLE();        /* 使能I2S TX DMA时钟 */
    __HAL_LINKDMA(&g_i2s_handle, hdmatx, g_i2s_txdma_handle);              /* 将DMA与I2S联系起来 */

    g_i2s_txdma_handle.Instance = I2S_TX_DMASx;                            /* 设置I2S TX DMA数据流 */
    g_i2s_txdma_handle.Init.Channel = I2S_TX_DMASx_Channel;                /* 设置I2S TX DMA通道 */
    g_i2s_txdma_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;              /* 存储器到外设模式 */
    g_i2s_txdma_handle.Init.PeriphInc = DMA_PINC_DISABLE;                  /* 外设非增量模式 */
    g_i2s_txdma_handle.Init.MemInc = DMA_MINC_ENABLE;                      /* 存储器增量模式 */
    g_i2s_txdma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD; /* 外设数据长度:16位 */
    g_i2s_txdma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;    /* 存储器数据长度:16位 */
    g_i2s_txdma_handle.Init.Mode = DMA_CIRCULAR;                           /* 使用循环模式 */
    g_i2s_txdma_handle.Init.Priority = DMA_PRIORITY_HIGH;                  /* 高优先级 */
    g_i2s_txdma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;               /* 不使用FIFO */
    g_i2s_txdma_handle.Init.MemBurst = DMA_MBURST_SINGLE;                  /* 存储器单次突发传输 */
    g_i2s_txdma_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;               /* 外设突发单次传输 */
    HAL_DMA_DeInit(&g_i2s_txdma_handle);                                   /* 先清除以前的设置 */
    HAL_DMA_Init(&g_i2s_txdma_handle);                                     /* 初始化DMA */

    HAL_DMAEx_MultiBufferStart(&g_i2s_txdma_handle, (uint32_t)buf0, (uint32_t)&I2S_SPI->DR, (uint32_t)buf1, num);  /* 开启双缓冲 */

    delay_us(10);                                                          /* 10us延时，防止-O2优化出问题 */
    __HAL_DMA_ENABLE_IT(&g_i2s_txdma_handle, DMA_IT_TC);                   /* 开启传输完成中断 */
    __HAL_DMA_CLEAR_FLAG(&g_i2s_txdma_handle, I2S_TX_DMASx_FLAG);          /* 清除DMA传输完成中断标志位 */
    HAL_NVIC_SetPriority(I2S_TX_DMASx_IRQ, 0, 0);                          /* DMA中断优先级 */
    HAL_NVIC_EnableIRQ(I2S_TX_DMASx_IRQ);

    __HAL_DMA_DISABLE(&g_i2s_txdma_handle);                                /* 先关闭DMA */
} 

void (*i2s_tx_callback)(void); /* I2S DMA回调函数指针 */
 
/**
 * @brief       I2S TX DMA 中断服务函数
 * @param       无
 * @retval      无
 */
void I2S_TX_DMASx_Handle(void)
{
    if (__HAL_DMA_GET_FLAG(&g_i2s_txdma_handle, I2S_TX_DMASx_FLAG) != RESET)   /* DMA传输完成 */
    {
        __HAL_DMA_CLEAR_FLAG(&g_i2s_txdma_handle, I2S_TX_DMASx_FLAG);          /* 清除DMA传输完成中断标志位 */

        if (i2s_tx_callback != NULL)
        {
            i2s_tx_callback();  /* 执行回调函数,读取数据等操作在这里面处理 */
        } 
    }
} 

/**
 * @brief       I2S开始播放
 * @param       无
 * @retval      无
 */
void i2s_play_start(void)
{
    __HAL_DMA_ENABLE(&g_i2s_txdma_handle);  /* 开启DMA TX传输 */
}

/**
 * @brief       I2S停止播放
 * @param       无
 * @retval      无
 */
void i2s_play_stop(void)
{
    __HAL_DMA_DISABLE(&g_i2s_txdma_handle);  /* 关闭DMA TX传输 */
}







