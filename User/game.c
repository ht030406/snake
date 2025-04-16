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
#include "time.h"
#include "./BSP/ES8388/es8388.h"
#include "./BSP/TPAD/tpad.h"


int map[30][60]={0};        //��ͼ���꣬��0����ʾ����ʾ����1����ʾ��ʾ��ͷ����2����ʾ������-1����ʾʳ��
int game_start_flag = 0;    //��Ϸ��ʼ��־
struct {                    //�ߵĽṹ��
    int snake_grid[1800][2];
    int length;
    int direction;          //��0����ʾ���ƣ���1�����ƣ���2�����ƣ���3������
}snake;
int score = 0;              //����

uint8_t score_easy_num[10]={0};     //��¼����(Сbug���������ᳬ��255)
uint8_t score_normal_num[10]={0};
uint8_t score_hard_num[10]={0};

uint8_t keyvalue;    //��ȡ����
int eated = 0;              //�Ե�ʳ����źţ�1��ʾ�Ե�ʳ�-1��ʾ�Ե����Ȼ��ʵ��
uint16_t color;              //������ɫ
int speed = 200;                  //���ƶ��ٶȣ�speedԽС�ƶ�Խ��,��ʼֵ200
int mode_flag = 0;              //�Ѷ�ѡ��0����򵥣�1������ͨ��2��������
int di_flag = 1;                //��Чѡ���־λ
int bad_score = 0;              //��ʾ���¡��Ȼ��ʵ��������
int eatfood_num = 0;            //��ʾ����ʳ�������

uint32_t flashsize = 16 * 1024 * 1024;

/* ��ʱ�����þ�� ���� */
TIM_HandleTypeDef g_timx_handle; /* ��ʱ��x��� */

//����
void GUI_clear(void)
{
    for(int i=0;i<30;i++)
    {
        for(int j=0;j<60;j++)
            map[i][j] = 0;
    }
}


//�˵���ʾ

void display_menu(struct MenuItem * MenuPoint,short rowItem)
{
    uint16_t x=0;
    uint16_t y=300;
    int i;
    lcd_fill(0,y+rowItem * 30-30,lcddev.width,y+rowItem * 30,DARKBLUE);
    for(i=0;i<MenuPoint->MeunCount;i++)
    {
        text_show_string_middle(x,y,(char* )MenuPoint[i].DisplayString,16,lcddev.width,WHITE);
        y+=30;
    }
}


//���Ƶ�ͼ
/*
    ��300����600��ÿ����ռ10��10����ζ���ߵ���󳤶�Ϊ30��60=1800
*/
void map_init(void)
{
    lcd_clear(LGRAY);
    lcd_draw_rectangle(74,49,375,650,RED);
    lcd_draw_rectangle(73,48,376,651,RED);
    lcd_draw_rectangle(72,47,377,652,RED);
    lcd_draw_rectangle(71,46,378,653,RED);
}

/****************************************��***********************************/
//������ͷ
void head(int i,int j)
{
    position(i,j,RED);
}
//����������ɫ�ɱ䣩
void body(int i,int j,uint16_t color)    
{
    position_body(i,j,color);
}
//����ʳ��
void food(int i,int j)
{
    position(i,j,RED);
}

//ˢ�½��溯����ִ��map�����������
void GUI_Refresh(void)
{
    int i,j,temp;
    for(i=0;i<30;i++)
    {
        for(j=0;j<60;j++)
        {
            temp = map[i][j];
            switch(temp)
            {
                case 0:         
                    position_clear(i,j);break;  //������ص�
                case 1:
                    head(i,j);break;            //��ͷ
                case 2:
                    body(i,j,color);break;      //������
                case -1:
                    food(i,j);break;            //��ʳ��
                case -2:
                    position(i,j,MAGENTA);break;    //�����Ȼ��ʵ��        
                case -3:
                    position(i,j,GREEN);break;      //����������ʵ��
                default:break;
            }
        }
    }
    
}

void snake_init(void)
{
    snake.length = 4;           //��ʼ������Ϊ3
    snake.direction = 0;        //��ʼ���ƶ�
    score = 0;                  //��ʼ����0
    eatfood_num = 0;            //��ʼ�Ե�ʳ������Ϊ0
    if(mode_flag != 2)          //��ʼ���ٶ�
        speed = 100;
    else
        speed = 75;
    bad_score = 0;              //��ʼ�Ե����Ȼ��ʵ������0
    snake.snake_grid[0][0] = 15;   //��ʼλ�����ڵ�ͼ�м�
    snake.snake_grid[0][1] = 30;
    for(int i = 1;i<snake.length;i++)
    {
        snake.snake_grid[i][0] = snake.snake_grid[0][0]-i;
        snake.snake_grid[i][1] = snake.snake_grid[0][1];
    }
}

//����
void Draw_snake(void)
{
    //��ͷ
    int i,x,y;
    x = snake.snake_grid[0][0];
    y = snake.snake_grid[0][1];
    map[x][y] = 1;
    //����
    for(i=1;i<snake.length;i++)
    {
        x = snake.snake_grid[i][0];
        y = snake.snake_grid[i][1];
        map[x][y] = 2;
    }

}



//�����ƶ�����
void Move(void)
{
    int i;
    map[snake.snake_grid[snake.length-1][0]][snake.snake_grid[snake.length-1][1]] = 0;  //��β���
    if(eated == 1)           //�Ե���ʳ��
    {
        snake.length++;
        eated = 0;
    }
    if(eated == -1)         //�Ե��ˡ��Ȼ��ʵ��
    {
        map[snake.snake_grid[snake.length-2][0]][snake.snake_grid[snake.length-2][1]] = 0;  //��β���
        snake.length--;
        eated = 0;
    }
    if(mode_flag == 0)              //��ģʽ��ײǽ�����������������һ��
    {
        if(snake.snake_grid[0][0]<0)    snake.snake_grid[0][0] = 29;
        if(snake.snake_grid[0][0]>29)   snake.snake_grid[0][0] = 0;
        if(snake.snake_grid[0][1]<0)    snake.snake_grid[0][1] = 59;
        if(snake.snake_grid[0][1]>59)    snake.snake_grid[0][1] = 0;
    }
    for(i=snake.length-1;i>0;i--)   //��һ��λ�õ��������ǰһ��
    {
        snake.snake_grid[i][0] = snake.snake_grid[i-1][0];
        snake.snake_grid[i][1] = snake.snake_grid[i-1][1];
    }
    switch(snake.direction)     //�ƶ�    
    {
        case 0:
            snake.snake_grid[0][0]++;break;
        case 1:
            snake.snake_grid[0][1]++;break;
        case 2:
            snake.snake_grid[0][0]--;break;
        case 3:
            snake.snake_grid[0][1]--;break;
        default:
            break;
    }
}

/*****************************************ʳ��*****************************************/
int Check(int i,int j)    //����λ��ʳ��
{
    if(map[i][j] == 0)
        return 1;
    else
        return 0;
}



void set_food(void)
{
    int i,j;
    do
    {
        srand(uwTick);
        i = rand()%30;
        j = rand()%60;
    }
    while(Check(i,j) == 0);  //����Ƿ�Ϊ��λ    
    map[i][j] = -1;          //����ʳ��  
}

void set_goodfood(void)
{
    int i,j;
    do
    {
        srand(uwTick);
        i = rand()%30;
        j = rand()%60;
    }
    while(Check(i,j) == 0);  //����Ƿ�Ϊ��λ   
    map[i][j] = -3;          //���ɡ�������ʵ��
}

void set_badfood(void)
{
    int i,j;
    do
    {
        srand(uwTick);
        i = rand()%30;
        j = rand()%60;
    }
    while(Check(i,j) == 0);  //����Ƿ�Ϊ��λ   
    map[i][j] = -2;          //���ɡ��Ȼ��ʵ��

    
}

void Eatfood(void)
{
    if(map[snake.snake_grid[0][0]][snake.snake_grid[0][1]] == -1)
    {
        eated = 1;          //��ǳԵ���ʳ��
        if(di_flag)         //��Ч��־λ�����Ż���
            di();
        score++;            //������һ
        eatfood_num++;      //��ǳԵ�ʳ���������һ
        if(eatfood_num == 4 && mode_flag!=2)    
        {           
            eatfood_num = 0;            
            set_goodfood();
            __HAL_TIM_SetCounter(&g_timx_handle , 0);   //���������㣨������ģʽ��
            
        }
        else
        {
            set_food();
            __HAL_TIM_SetCounter(&g_timx_handle , 0);   //���������㣨����ģʽ��
        }
            
        map[snake.snake_grid[0][0]][snake.snake_grid[0][1]] = 0;    //ʳ��λ������
    }
    
    if(map[snake.snake_grid[0][0]][snake.snake_grid[0][1]] == -3)   //�Ե��ˡ�������ʵ��
    {
        eated = 1;          //��ǳԵ���ʳ��
        if(di_flag)         //��Ч��־λ�����Ż���
            di();
        score+=2;            //������2
        set_food();         //���·�ʳ��
        map[snake.snake_grid[0][0]][snake.snake_grid[0][1]] = 0;    //ʳ��λ������
    }
    
    if(mode_flag == 2 && map[snake.snake_grid[0][0]][snake.snake_grid[0][1]] == -2)
    {
        eated = -1;          //��ǳԵ��ˡ��Ȼ��ʵ��
        if(di_flag)         //��Ч��־λ�����Ż���
            di();
        score--;            //������һ
        bad_score++;        //���Ȼ��ʵ��������һ
        set_badfood();         //���·š��Ȼ��ʵ��
        map[snake.snake_grid[0][0]][snake.snake_grid[0][1]] = 0;    //ʳ��λ������
    }
}

/*****************************************��Ϸ��ʼ�ͽ���****************************************************/
void game_start(void)
{
    game_start_flag = 1;
    GUI_clear();
    map_init();
    snake_init();
    set_food();
    if(mode_flag == 2)          //����ģʽ��20�����Ȼ��ʵ��
    {
        for(int i=0;i<20;i++)
        {
            set_badfood();
        }
    }

    while(1)
    {        
        int isgameover = game_over();
        int return_flag = 0;    // ���ز˵��ı�־
        GUI_Refresh();
        if(isgameover == 0)     // ��Ϸδ����
        {
            show_score();   // ��ʾ����        
            Move();         // ���ƶ�����    
            Eatfood();      // �Ե�ʳ��
            Draw_snake();   // ����
            delay_ms(speed);// �ƶ��ٶ�
            if(mode_flag != 0 && speed>20)  //һ��ģʽ�£�������ʳ�speed-5
            {
                if(score%3 == 0 && score != 0 && eated == 1)
                    speed-=5;
            }
            if(tpad_scan(0))    // ���ݴ���������Ϸ��ͣ
            {
                lcd_clear(LGRAY);
                while(1)
                {                    
                    text_show_string_middle(0,300,"��Ϸ��ͣ",24,lcddev.width,RED);
                    text_show_string_middle(0,350,"����������������˵�",24,lcddev.width,BLACK);
                    text_show_string_middle(0,400,"�Ҽ�����",24,lcddev.width,BLACK);
                    text_show_string_middle(0,450,"�Ȱ�һ��������ٰ�һ���Ҽ�����Ϸ���¿�ʼ",24,lcddev.width,BLACK);
                    
                    uint8_t key = key_scan(0);
                    if(key == KEY0_PRES)    // �Ҽ������� ��Ϸ����
                    {
                       map_init();
                       break;
                    }
                       
                    if(key == KEY2_PRES)    // ��������£��������˵�
                    {
                        return_flag = 1;
                        break;
                    }
                      
                }

            }
            if(return_flag == 1)
                break;  
        }
        else    // ��Ϸ����
        {
            lcd_clear(LGRAY);
            text_show_string_middle(0,300,"GAME OVER!",32,lcddev.width,RED);
            text_show_string_middle(0,350,"�밴����������˵�",24,lcddev.width,BLACK);
            text_show_string_middle(0,400,"����������¿�ʼ���밴�Ҽ�",24,lcddev.width,BLACK);
            show_score();
            
            //��������������
            sort(score);
            break;
        }
            
    }
    
}

int game_over(void)
{
    int isGameOver = 0;
    //int sx = snake.snake_grid[0][0],sy = snake.snake_grid[0][1];
    
    if(mode_flag != 0)                          //��ģʽײǽ����
    {
        if(snake.snake_grid[0][0]<0 || snake.snake_grid[0][0]>29 || snake.snake_grid[0][1]<0 || snake.snake_grid[0][1]>59)      //�ж�ײǽ
            isGameOver = 1;
    }
    
    if(mode_flag == 2)                          //����ģʽ������С�ڳ�ʼ���Ⱦ���
    {
        if(snake.length < 4)
            isGameOver = 1;
    }
    
    for(int i = 1;i < snake.length;i++)
    {
        if(snake.snake_grid[0][0] == snake.snake_grid[i][0] && snake.snake_grid[0][1] == snake.snake_grid[i][1])    //�ж�ҧ���Լ�
        {
            isGameOver = 1;
            break;
        }
    }
    return isGameOver;
}
//��ʾ����
void show_score(void)
{
    if(score<0)
        score = 0;
    text_show_string(160,680,lcddev.width,lcddev.height,"score:",24,1,BLACK);
    lcd_show_num(230,680,score,4,24,RED);
    if(mode_flag == 2 && game_over() == 0)
    {
        switch(bad_score)
        {
            case 0:text_show_string_middle(0,730,"Noka״̬����",24,lcddev.width,RED);break;
            case 1:{lcd_fill(0,730,350,754,LGRAY);text_show_string_middle(0,730,"Noka���ж� 33%",24,lcddev.width,RED);}break;
            case 2:{lcd_fill(0,730,350,754,LGRAY);text_show_string_middle(0,730,"Noka���ж� 66%",24,lcddev.width,RED);}break;
            case 3:{lcd_fill(0,730,350,754,LGRAY);text_show_string_middle(0,730,"Noka���ж�",24,lcddev.width,RED);}break;           
        }
        
    }
}


/************************************************�˵����õĹ��ܺ���**********************************/

//�ߵ�Ƥ������
void skin_black(void)
{
    color = BLACK;
    text_show_string_middle(0,500,"���óɹ�",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}
void skin_green(void)
{
    color = GREEN;
    text_show_string_middle(0,500,"���óɹ�",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}
void skin_blue(void)
{
    color = BLUE;
    text_show_string_middle(0,500,"���óɹ�",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}



/*---------------------------------------------��Ϸģʽ-----------------------------------------*/
/*
    ��ģʽ�£��ٶȲ���������������ģʽֻ��ҧ���Լ��Ż�����ײǽ��������
*/
void easy_mode(void)
{
    speed = 100;
    mode_flag = 0;
    text_show_string_middle(0,500,"���óɹ�",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}
/*
    һ��ģʽ�£���ʼ�ٶ�Ϊ150������ٶ�Ϊspeed=50��ÿ�Ե�3��ʳ��ٶ�speed-5��
    ֱ��speed=50��������Ϊ50ʱ�ͻᵽ������ٶȣ�����ģʽΪײǽ��ҧ���Լ�;
*/
void normal_mode(void)  
{
    speed = 100;
    mode_flag = 1;
    text_show_string_middle(0,500,"���óɹ�",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}
/*
    ����ģʽ�£���ʼ�ٶ�Ϊ100������ٶ�50��ÿ�Ե�3����ʵ��speed-5����score = 30���ٶ����
    ��������ɶ�����Ȼ��ʵ������Ʒ��ɫ����ɫ���֣�
    ���߳ٵ����Ȼ��ʵ��ʱ�������1��������1���ۼƳԵ�3����ʵ�߻᲻�ܿ��ƣ������෴�������ϼ������ߣ�����������ߣ�
    ͬʱ����������С�ڳ�ʼֵʱ���߻�������
*/
void hard_mode(void)
{
    speed = 75;
    mode_flag = 2;
    text_show_string_middle(0,500,"���óɹ�",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}

/**
    ��Ч���غ���
**/
void di_open(void)
{
    di_flag = 1;
    text_show_string_middle(0,500,"���óɹ�",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}
void di_close(void)
{
    di_flag = 0;
    text_show_string_middle(0,500,"���óɹ�",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}

/**
    ���а��ܺ����������»�õķ�����֮ǰ�ķ����Ƚϣ����С����ͷ֣��򲻼��룻
    ���������ͷ֣��������鲢�������򣬲���ð������ķ�����
**/

void maopao(uint8_t *num)
{
    int i,j,temp;
    for(i=0;i<10;i++)
    {
        for(j=i+1;j<10;j++)
        {
            if(num[i]<num[j])
            {
                temp = num[i];
                num[i] = num[j];
                num[j] = temp;
            }
        }
    }
}


void sort(uint8_t score)    //��������
{
    if(mode_flag == 0)
    {
        if(score > score_easy_num[9])   //����ͷ�Ҫ�߲ż���ɼ�
        {
            score_easy_num[9] = score;
            maopao(score_easy_num);
            norflash_write(score_easy_num,flashsize - 200,sizeof(score_easy_num));  //д������
        }
    }
    else if(mode_flag == 1)
    {
        if(score > score_normal_num[9])   //����ͷ�Ҫ�߲ż���ɼ�
        {
            score_normal_num[9] = score;
            maopao(score_normal_num);
            norflash_write(score_normal_num,flashsize - 150,sizeof(score_normal_num));  //д������
        }
    }
    else if(mode_flag == 2)
    {
        if(score > score_hard_num[9])   //����ͷ�Ҫ�߲ż���ɼ�
        {
            score_hard_num[9] = score;
            maopao(score_hard_num);
            norflash_write(score_hard_num,flashsize - 100,sizeof(score_hard_num));  //д������
        }
    }
    
}


void ranking_show(void)                 //���а���ʾ����
{
    norflash_read(score_easy_num, flashsize - 200, sizeof(score_easy_num));   /* �ӵ�����200����ַ����ʼ,����SIZE���ֽ� */
    norflash_read(score_normal_num, flashsize - 150, sizeof(score_normal_num));
    norflash_read(score_hard_num, flashsize - 100, sizeof(score_hard_num));
    lcd_clear(LGRAY);
    text_show_string(50,100,lcddev.width,lcddev.height,"��ģʽ",24,1,BLACK);
    for(int i=0;i<10;i++)
    {
        lcd_show_xnum(50,148+i*48,score_easy_num[i],4,24,1,RED);
    }
    text_show_string(185,100,lcddev.width,lcddev.height,"��ͨģʽ",24,1,BLACK);
    for(int i=0;i<10;i++)
    {
        lcd_show_xnum(185,148+i*48,score_normal_num[i],4,24,1,RED);
    }
    text_show_string(320,100,lcddev.width,lcddev.height,"����ģʽ",24,1,BLACK);
    for(int i=0;i<10;i++)
    {
        lcd_show_xnum(320,148+i*48,score_hard_num[i],4,24,1,RED);
    }
}



/**
    ��Ϸ���
**/
void intro(void)
{
    lcd_clear(LGRAY);
    //ÿ�����ϱ��������30���֡�
    text_show_string_middle(0,0,"��ңԶ������ķ�������һ���뵺�ϣ�������һȺ��Զ�Բ�����������ڳ��ڵĽ�ʳ�����¸ð뵺��Դ�ѷ����޷���Ӧ���и������档���ǣ�Ϊ���������壬�����ɳ���һȺ�ж�Ѹ�ٵĸ������ȥѰ���µļ�԰������Щ������һ��ͳ�ơ���Noka��",24,lcddev.width,BLACK);
    text_show_string_middle(0,160,"��ģʽ�£�Noka������һ��������Դ�ǳ��ḻ�ĵ������������ųԲ����ʳ�ÿ�γ���ڶ��춼�᳤��������������û�в��ǻ�������û�����ǵ���С�����ÿ�����춼�᳤���ǳ���ζ�Ĺ�ʵ������Ҫע�⼰ʱ���꣬��Ȼ�ͻ���ʧ��",24,lcddev.width,BLACK);
    text_show_string_middle(0,330,"��ͨģʽ�£�Noka������һ������Ҳ�ܷḻ�ĵ�����ֻ���������ʳ����Ʒһ�㣬�Ե�Խ��Խ��ԣ��Ե��ٶ�ҲԽ��Խ�죬Noka�����һ��ȥ��ʳ������Χǽ���滹�����ǵ���У�ֻҪ����Χǽ���ͻ��Ϊ���˵�ʳ�",24,lcddev.width,BLACK);    text_show_string_middle(0,480,"����ģʽ�£�Noka�����˽����������ʳ��ÿ�춼����ָ��ʱ����֣���ֻ����̵ֺܶ�ʱ�䡣ͬʱ�������Σ�ղ�ֹ��ǽ�����У��������������ġ��Ȼ��ʵ�������Noka�̲�ס�������������������ϵĹ�ʵ���ͻᷢ�����������......",24,lcddev.width,BLACK);
    text_show_string_middle(0,700,"���Ϲ��´����鹹��������ͬ�������ɺ� :)",16,lcddev.width,RED);
}



/**
 * @brief       �жϷ����������Ҫ��������
 *              ��HAL�������е��ⲿ�жϷ�����������ô˺���
 * @param       GPIO_Pin:�ж����ź�
 * @retval      ��
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    delay_ms(20);      /* ���� */
    switch(GPIO_Pin)
    {
        case KEY0_INT_GPIO_PIN:
            if (KEY0 == 0)
            {
                if(mode_flag == 2 && bad_score>=3)  //����ģʽ�³�������ʵ����ת����
                {
                    if(snake.direction != 0)
                        snake.direction = 2;
                }
                else
                {
                    if(snake.direction != 2)
                        snake.direction = 0;
                }
                
            } 
            break;

        case KEY1_INT_GPIO_PIN:
            if (KEY1 == 0)
            {
                if(mode_flag == 2 && bad_score>=3)
                {
                    if(snake.direction != 1)
                        snake.direction = 3;
                }
                else
                {
                    if(snake.direction != 3)
                        snake.direction = 1;
                }
                
            }
            break;

        case KEY2_INT_GPIO_PIN:
            if (KEY2 == 0)
            {
                if(mode_flag == 2 && bad_score>=3)
                {
                    if(snake.direction != 2)
                        snake.direction = 0;
                }
                else
                {
                    if(snake.direction != 0)
                        snake.direction = 2;
                }
            }
            break;

        case WKUP_INT_GPIO_PIN:
            if (WK_UP == 1)
            {
                if(mode_flag == 2 && bad_score>=3)
                {
                    if(snake.direction != 3)
                        snake.direction = 1;
                }
                else
                {
                    if(snake.direction != 1)
                        snake.direction = 3;
                }
            }
            break;

        default : break;
    }
}





/**
 * @brief       ͨ�ö�ʱ��TIMX��ʱ�жϳ�ʼ������
 * @note
 *              ͨ�ö�ʱ����ʱ������APB1,��PPRE1 �� 2��Ƶ��ʱ��
 *              ͨ�ö�ʱ����ʱ��ΪAPB1ʱ�ӵ�2��, ��APB1Ϊ42M, ���Զ�ʱ��ʱ�� = 84Mhz
 *              ��ʱ�����ʱ����㷽��: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=��ʱ������Ƶ��,��λ:Mhz
 *
 * @param       arr: �Զ���װֵ
 * @param       psc: Ԥ��Ƶϵ��
 * @retval      ��
 */
void gtim_timx_int_init(uint32_t arr, uint32_t psc)
{
    GTIM_TIMX_INT_CLK_ENABLE();                             /* ʹ��TIMxʱ�� */

    g_timx_handle.Instance = GTIM_TIMX_INT;                 /* ͨ�ö�ʱ��x */
    g_timx_handle.Init.Prescaler = psc;                     /* Ԥ��Ƶϵ�� */
    g_timx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;    /* ��������ģʽ */
    g_timx_handle.Init.Period = arr;                        /* �Զ�װ��ֵ */
    HAL_TIM_Base_Init(&g_timx_handle);
    
    HAL_NVIC_SetPriority(GTIM_TIMX_INT_IRQn, 4, 3);         /* �����ж����ȼ�����ռ���ȼ�1�������ȼ�3 */
    HAL_NVIC_EnableIRQ(GTIM_TIMX_INT_IRQn);                 /* ����ITMx�ж� */

    HAL_TIM_Base_Start_IT(&g_timx_handle);                  /* ʹ�ܶ�ʱ��x�Ͷ�ʱ��x�����ж� */
}

/**
 * @brief       ��ʱ���жϷ�����
 * @param       ��
 * @retval      ��
 */
void GTIM_TIMX_INT_IRQHandler(void)
{
    /* ���´���û��ʹ�ö�ʱ��HAL�⹲�ô���������������ֱ��ͨ���ж��жϱ�־λ�ķ�ʽ */
    if(__HAL_TIM_GET_FLAG(&g_timx_handle, TIM_FLAG_UPDATE) != RESET)
    {
        if(game_start_flag == 1 && mode_flag!=2)    // ������ģʽˢ��ʳ��
        {
            for(int i =0;i<30;i++)
            {
                for(int j=0;j<60;j++)
                {
                    if(map[i][j] == -3)
                    {
                        map[i][j] = 0;
                        set_food();
                    }
                }
            }
        }

        if(mode_flag == 2 && game_start_flag == 1)  // ����ģʽˢ��ʳ��
        {
            for(int i =0;i<30;i++)
            {
                for(int j=0;j<60;j++)
                {
                    if(map[i][j] == -1)
                    {
                        map[i][j] = 0;
                        set_food();
                    }
                }
            }
        }
         
        //HAL_NVIC_DisableIRQ(GTIM_TIMX_INT_IRQn);            //�ض�ʱ��
        __HAL_TIM_CLEAR_IT(&g_timx_handle, TIM_IT_UPDATE);  /* �����ʱ������жϱ�־λ */
        
        
    }
}


