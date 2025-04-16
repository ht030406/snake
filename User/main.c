#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/SRAM/sram.h"
#include "./BSP/SDIO/sdio_sdcard.h"
#include "./BSP/NORFLASH/norflash.h"
#include "./FATFS/exfuns/exfuns.h"
#include "./MALLOC/malloc.h"
#include "./USMART/usmart.h"
#include "./TEXT/text.h"
#include "./PICTURE/piclib.h"
#include "string.h"
#include "math.h"
#include "./BSP/BEEP/beep.h"
#include "./BSP/EXTI/exti.h"
#include "game.h"
#include "./BSP/TIMER/gtim.h"
#include "./BSP/ES8388/es8388.h"
#include "./BSP/TPAD/tpad.h"

void choose_menu(void);

//__wavctrl wavctrl;                      /* WAV控制结构体 */
//__audiodev g_audiodev;          /* 音乐播放控制器 */

/*----------------------------------------------结构体数组菜单--------------------------------------------------------*/

struct MenuItem MainMenu[5] = {                 //主菜单，即根目录
    {5,(uint16_t *)"开始游戏",game_start,NULL,NULL},  //子菜单原理上是有的，但是由于定义在后面，这里写入会报错，先写NULL，之后初始化一下就可以了
    {5,(uint16_t *)"设置",NULL,NULL,NULL},
    {5,(uint16_t *)"皮肤",NULL,NULL,NULL},
    {5,(uint16_t *)"排行榜",ranking_show,NULL,NULL},
    {5,(uint16_t *)"游戏简介",intro,NULL,NULL}
};
struct MenuItem Setmenu2[2] = {                 //“设置”的二级菜单
    {2,(uint16_t *)"难度",NULL,NULL,MainMenu},
    {2,(uint16_t *)"音效",NULL,NULL,MainMenu}
};
struct MenuItem Setmenu3[3] = {                 //“皮肤”的二级菜单
    {3,(uint16_t *)"黑色蛇身",skin_black,NULL,MainMenu},
    {3,(uint16_t *)"绿色蛇身",skin_green,NULL,MainMenu},
    {3,(uint16_t *)"蓝色蛇身",skin_blue,NULL,MainMenu}
};
struct MenuItem Setmenu21[3] = {                //“设置”中“难度”的三级菜单
    {3,(uint16_t *)"简单",easy_mode,NULL,Setmenu2},
    {3,(uint16_t *)"普通",normal_mode,NULL,Setmenu2},
    {3,(uint16_t *)"困难",hard_mode,NULL,Setmenu2}
};
struct MenuItem Setmenu22[2] = {                //“设置”中“音效”的三级菜单
    {2,(uint16_t *)"开",di_open,NULL,Setmenu2},
    {2,(uint16_t *)"关",di_close,NULL,Setmenu2}
};

struct MenuItem* MenuPoint = MainMenu;  //当前菜单的地址
short rowItem;                          //当前在第几行
uint8_t subs_flag =0;


int main(void)
{
    uint8_t key;
    
    HAL_Init();                             /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7);     /* 设置时钟,168Mhz */
    delay_init(168);                        /* 延时初始化 */
    usart_init(115200);                     /* 串口初始化为115200 */
    led_init();                             /* 初始化LED */
    lcd_init();                             /* 初始化LCD */
    key_init();                             /* 初始化key*/
    extix_init();                           /*中断初始化*/
    beep_init();                            /*初始化蜂鸣器*/
    sram_init();                            /* SRAM初始化 */
    norflash_init();                        /* 初始化NORFLASH */
    tpad_init(8);                           /*初始化电容触摸*/
    gtim_timx_int_init(10000 - 1, 168000 - 1); /* 168 000 000 / 168 000 = 1 000 1Khz的计数频率，计数5K次为5s */
    
    
    my_mem_init(SRAMIN);                    /* 初始化内部SRAM内存池 */
    my_mem_init(SRAMEX);                    /* 初始化外部SRAM内存池 */
    my_mem_init(SRAMCCM);                   /* 初始化CCM内存池*/
    
    exfuns_init();                          /* 为fatfs相关变量申请内存 */

    
    f_mount(fs[0], "0:", 1);                /* 挂载SD卡 */
    f_mount(fs[1], "1:", 1);                /* 挂载FLASH */
    while (fonts_init())                    // 检查字库 
    {
        lcd_clear(WHITE);                   // 清屏 
        lcd_show_string(30, 30, 200, 16, 16, "STM32", RED);

        while (sd_init())                   // 检测SD卡 
        {
            lcd_show_string(30, 50, 200, 16, 16, "SD Card Failed!", RED);
            delay_ms(200);
            lcd_fill(30, 50, 200 + 30, 50 + 16, WHITE);
            delay_ms(200);
        }
        key = fonts_update_font(20, 90, 16, (uint8_t *)"0:", RED);      // 更新字库 

        while (key)     // 更新失败 
        {
            lcd_show_string(30, 90, 200, 16, 16, "Font Update Failed!", RED);
            delay_ms(200);
            lcd_fill(20, 90, 200 + 20, 90 + 16, WHITE);
            delay_ms(200);
        }

        lcd_show_string(30, 90, 200, 16, 16, "Font Update Success!   ", RED);
        delay_ms(1500);
        lcd_clear(WHITE); // 清屏 
    }
    
    //初始化菜单
    MainMenu[1].next = Setmenu2;
    MainMenu[2].next = Setmenu3;
    Setmenu2[0].next = Setmenu21;
    Setmenu2[1].next = Setmenu22;
    rowItem = 1;
    
    /*
        初始化界面，按任意键继续
    */
    piclib_init();                                          /* 初始化画图 */
    piclib_ai_load_picfile("0:/PICTURE/snake.jpg", 0, 0, lcddev.width, lcddev.height, 1); /* 显示图片 */
    text_show_string(150,100,lcddev.width,lcddev.height,"贪吃蛇——Noka篇",24,1,BRRED);
    text_show_string_middle(0,700,"请按任意键继续...",24,lcddev.width,BLACK);
    while(1)
    {
        uint8_t key = key_scan(0);
        if(key != 0)
            break;
        if(tpad_scan(0))
            break;
    }
    lcd_clear(LGRAY);
    display_menu(MenuPoint,rowItem);
    
    /*
        进入主菜单
    */
    while(1)
    {   
        // 菜单选择
        choose_menu();
    }

}





void choose_menu(void)
{
    uint8_t key = key_scan(0);   
        if(key!=0)
        {
            switch(key)
            {
                case WKUP_PRES:
                {
                    //当前为第一行，按下上键，调到最后一行
                    if(rowItem == 1) rowItem = MenuPoint->MeunCount;
                    //否则，行数减1，向上移动
                    else rowItem--;
                };break;
                
                case KEY1_PRES:
                {
                    //当前为最后一行，按下键，调到第一行
                    if(rowItem == MenuPoint->MeunCount) rowItem = 1;
                    //否则，行数加1，向下移动
                    else rowItem++;
                };break;
                
                case KEY0_PRES:     //按下右键，确认此项目
                {
                    if(MenuPoint[rowItem - 1].Subs != NULL)     //子菜单功能函数非空则执行功能函数
                    {
                        MenuPoint[rowItem - 1].Subs();
                        subs_flag = 1;
                    }
                    else if(MenuPoint[rowItem - 1].next != NULL)
                    {
                        MenuPoint = MenuPoint[rowItem - 1].next;
                        rowItem = 1;
                    }
                };break;
                
                case KEY2_PRES:     //按下左键，返回上个界面
                {
                    if(MenuPoint[rowItem - 1].prev != NULL)     //父菜单非空才进入
                    {
                        MenuPoint = MenuPoint[rowItem - 1].prev;
                        rowItem = 1;
                    }
                };break;
            }
            if(subs_flag == 0)
            {
                lcd_clear(LGRAY);
                display_menu(MenuPoint,rowItem);
            }
            subs_flag = 0;
        }
}


