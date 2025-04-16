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

//__wavctrl wavctrl;                      /* WAV���ƽṹ�� */
//__audiodev g_audiodev;          /* ���ֲ��ſ����� */

/*----------------------------------------------�ṹ������˵�--------------------------------------------------------*/

struct MenuItem MainMenu[5] = {                 //���˵�������Ŀ¼
    {5,(uint16_t *)"��ʼ��Ϸ",game_start,NULL,NULL},  //�Ӳ˵�ԭ�������еģ��������ڶ����ں��棬����д��ᱨ����дNULL��֮���ʼ��һ�¾Ϳ�����
    {5,(uint16_t *)"����",NULL,NULL,NULL},
    {5,(uint16_t *)"Ƥ��",NULL,NULL,NULL},
    {5,(uint16_t *)"���а�",ranking_show,NULL,NULL},
    {5,(uint16_t *)"��Ϸ���",intro,NULL,NULL}
};
struct MenuItem Setmenu2[2] = {                 //�����á��Ķ����˵�
    {2,(uint16_t *)"�Ѷ�",NULL,NULL,MainMenu},
    {2,(uint16_t *)"��Ч",NULL,NULL,MainMenu}
};
struct MenuItem Setmenu3[3] = {                 //��Ƥ�����Ķ����˵�
    {3,(uint16_t *)"��ɫ����",skin_black,NULL,MainMenu},
    {3,(uint16_t *)"��ɫ����",skin_green,NULL,MainMenu},
    {3,(uint16_t *)"��ɫ����",skin_blue,NULL,MainMenu}
};
struct MenuItem Setmenu21[3] = {                //�����á��С��Ѷȡ��������˵�
    {3,(uint16_t *)"��",easy_mode,NULL,Setmenu2},
    {3,(uint16_t *)"��ͨ",normal_mode,NULL,Setmenu2},
    {3,(uint16_t *)"����",hard_mode,NULL,Setmenu2}
};
struct MenuItem Setmenu22[2] = {                //�����á��С���Ч���������˵�
    {2,(uint16_t *)"��",di_open,NULL,Setmenu2},
    {2,(uint16_t *)"��",di_close,NULL,Setmenu2}
};

struct MenuItem* MenuPoint = MainMenu;  //��ǰ�˵��ĵ�ַ
short rowItem;                          //��ǰ�ڵڼ���
uint8_t subs_flag =0;


int main(void)
{
    uint8_t key;
    
    HAL_Init();                             /* ��ʼ��HAL�� */
    sys_stm32_clock_init(336, 8, 2, 7);     /* ����ʱ��,168Mhz */
    delay_init(168);                        /* ��ʱ��ʼ�� */
    usart_init(115200);                     /* ���ڳ�ʼ��Ϊ115200 */
    led_init();                             /* ��ʼ��LED */
    lcd_init();                             /* ��ʼ��LCD */
    key_init();                             /* ��ʼ��key*/
    extix_init();                           /*�жϳ�ʼ��*/
    beep_init();                            /*��ʼ��������*/
    sram_init();                            /* SRAM��ʼ�� */
    norflash_init();                        /* ��ʼ��NORFLASH */
    tpad_init(8);                           /*��ʼ�����ݴ���*/
    gtim_timx_int_init(10000 - 1, 168000 - 1); /* 168 000 000 / 168 000 = 1 000 1Khz�ļ���Ƶ�ʣ�����5K��Ϊ5s */
    
    
    my_mem_init(SRAMIN);                    /* ��ʼ���ڲ�SRAM�ڴ�� */
    my_mem_init(SRAMEX);                    /* ��ʼ���ⲿSRAM�ڴ�� */
    my_mem_init(SRAMCCM);                   /* ��ʼ��CCM�ڴ��*/
    
    exfuns_init();                          /* Ϊfatfs��ر��������ڴ� */

    
    f_mount(fs[0], "0:", 1);                /* ����SD�� */
    f_mount(fs[1], "1:", 1);                /* ����FLASH */
    while (fonts_init())                    // ����ֿ� 
    {
        lcd_clear(WHITE);                   // ���� 
        lcd_show_string(30, 30, 200, 16, 16, "STM32", RED);

        while (sd_init())                   // ���SD�� 
        {
            lcd_show_string(30, 50, 200, 16, 16, "SD Card Failed!", RED);
            delay_ms(200);
            lcd_fill(30, 50, 200 + 30, 50 + 16, WHITE);
            delay_ms(200);
        }
        key = fonts_update_font(20, 90, 16, (uint8_t *)"0:", RED);      // �����ֿ� 

        while (key)     // ����ʧ�� 
        {
            lcd_show_string(30, 90, 200, 16, 16, "Font Update Failed!", RED);
            delay_ms(200);
            lcd_fill(20, 90, 200 + 20, 90 + 16, WHITE);
            delay_ms(200);
        }

        lcd_show_string(30, 90, 200, 16, 16, "Font Update Success!   ", RED);
        delay_ms(1500);
        lcd_clear(WHITE); // ���� 
    }
    
    //��ʼ���˵�
    MainMenu[1].next = Setmenu2;
    MainMenu[2].next = Setmenu3;
    Setmenu2[0].next = Setmenu21;
    Setmenu2[1].next = Setmenu22;
    rowItem = 1;
    
    /*
        ��ʼ�����棬�����������
    */
    piclib_init();                                          /* ��ʼ����ͼ */
    piclib_ai_load_picfile("0:/PICTURE/snake.jpg", 0, 0, lcddev.width, lcddev.height, 1); /* ��ʾͼƬ */
    text_show_string(150,100,lcddev.width,lcddev.height,"̰���ߡ���Nokaƪ",24,1,BRRED);
    text_show_string_middle(0,700,"�밴���������...",24,lcddev.width,BLACK);
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
        �������˵�
    */
    while(1)
    {   
        // �˵�ѡ��
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
                    //��ǰΪ��һ�У������ϼ����������һ��
                    if(rowItem == 1) rowItem = MenuPoint->MeunCount;
                    //����������1�������ƶ�
                    else rowItem--;
                };break;
                
                case KEY1_PRES:
                {
                    //��ǰΪ���һ�У����¼���������һ��
                    if(rowItem == MenuPoint->MeunCount) rowItem = 1;
                    //����������1�������ƶ�
                    else rowItem++;
                };break;
                
                case KEY0_PRES:     //�����Ҽ���ȷ�ϴ���Ŀ
                {
                    if(MenuPoint[rowItem - 1].Subs != NULL)     //�Ӳ˵����ܺ����ǿ���ִ�й��ܺ���
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
                
                case KEY2_PRES:     //��������������ϸ�����
                {
                    if(MenuPoint[rowItem - 1].prev != NULL)     //���˵��ǿղŽ���
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


