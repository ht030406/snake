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


int map[30][60]={0};        //地图坐标，“0”表示不显示，“1”表示显示蛇头，“2”表示蛇身，“-1”表示食物
int game_start_flag = 0;    //游戏开始标志
struct {                    //蛇的结构体
    int snake_grid[1800][2];
    int length;
    int direction;          //“0”表示右移，“1”下移，“2”左移，“3”上移
}snake;
int score = 0;              //分数

uint8_t score_easy_num[10]={0};     //记录分数(小bug，分数不会超过255)
uint8_t score_normal_num[10]={0};
uint8_t score_hard_num[10]={0};

uint8_t keyvalue;    //获取按键
int eated = 0;              //吃到食物的信号，1表示吃到食物，-1表示吃到“魅惑果实”
uint16_t color;              //蛇身颜色
int speed = 200;                  //蛇移动速度，speed越小移动越快,初始值200
int mode_flag = 0;              //难度选择，0代表简单，1代表普通，2代表困难
int di_flag = 1;                //音效选择标志位
int bad_score = 0;              //表示吃下“魅惑果实”的数量
int eatfood_num = 0;            //表示吃下食物的数量

uint32_t flashsize = 16 * 1024 * 1024;

/* 定时器配置句柄 定义 */
TIM_HandleTypeDef g_timx_handle; /* 定时器x句柄 */

//清屏
void GUI_clear(void)
{
    for(int i=0;i<30;i++)
    {
        for(int j=0;j<60;j++)
            map[i][j] = 0;
    }
}


//菜单显示

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


//绘制地图
/*
    宽300，长600，每个点占10×10，意味着蛇的最大长度为30×60=1800
*/
void map_init(void)
{
    lcd_clear(LGRAY);
    lcd_draw_rectangle(74,49,375,650,RED);
    lcd_draw_rectangle(73,48,376,651,RED);
    lcd_draw_rectangle(72,47,377,652,RED);
    lcd_draw_rectangle(71,46,378,653,RED);
}

/****************************************蛇***********************************/
//绘制蛇头
void head(int i,int j)
{
    position(i,j,RED);
}
//绘制蛇身（颜色可变）
void body(int i,int j,uint16_t color)    
{
    position_body(i,j,color);
}
//绘制食物
void food(int i,int j)
{
    position(i,j,RED);
}

//刷新界面函数，执行map数组里的数据
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
                    position_clear(i,j);break;  //清除像素点
                case 1:
                    head(i,j);break;            //画头
                case 2:
                    body(i,j,color);break;      //画身体
                case -1:
                    food(i,j);break;            //画食物
                case -2:
                    position(i,j,MAGENTA);break;    //画“魅惑果实”        
                case -3:
                    position(i,j,GREEN);break;      //画“奖励果实”
                default:break;
            }
        }
    }
    
}

void snake_init(void)
{
    snake.length = 4;           //初始蛇身长度为3
    snake.direction = 0;        //初始右移动
    score = 0;                  //初始分数0
    eatfood_num = 0;            //初始吃到食物数量为0
    if(mode_flag != 2)          //初始化速度
        speed = 100;
    else
        speed = 75;
    bad_score = 0;              //初始吃到“魅惑果实”数量0
    snake.snake_grid[0][0] = 15;   //初始位置设在地图中间
    snake.snake_grid[0][1] = 30;
    for(int i = 1;i<snake.length;i++)
    {
        snake.snake_grid[i][0] = snake.snake_grid[0][0]-i;
        snake.snake_grid[i][1] = snake.snake_grid[0][1];
    }
}

//画蛇
void Draw_snake(void)
{
    //蛇头
    int i,x,y;
    x = snake.snake_grid[0][0];
    y = snake.snake_grid[0][1];
    map[x][y] = 1;
    //蛇身
    for(i=1;i<snake.length;i++)
    {
        x = snake.snake_grid[i][0];
        y = snake.snake_grid[i][1];
        map[x][y] = 2;
    }

}



//蛇身移动函数
void Move(void)
{
    int i;
    map[snake.snake_grid[snake.length-1][0]][snake.snake_grid[snake.length-1][1]] = 0;  //蛇尾清除
    if(eated == 1)           //吃到了食物
    {
        snake.length++;
        eated = 0;
    }
    if(eated == -1)         //吃到了“魅惑果实”
    {
        map[snake.snake_grid[snake.length-2][0]][snake.snake_grid[snake.length-2][1]] = 0;  //蛇尾清除
        snake.length--;
        eated = 0;
    }
    if(mode_flag == 0)              //简单模式下撞墙不会死，会出现在另一端
    {
        if(snake.snake_grid[0][0]<0)    snake.snake_grid[0][0] = 29;
        if(snake.snake_grid[0][0]>29)   snake.snake_grid[0][0] = 0;
        if(snake.snake_grid[0][1]<0)    snake.snake_grid[0][1] = 59;
        if(snake.snake_grid[0][1]>59)    snake.snake_grid[0][1] = 0;
    }
    for(i=snake.length-1;i>0;i--)   //后一个位置的坐标等于前一个
    {
        snake.snake_grid[i][0] = snake.snake_grid[i-1][0];
        snake.snake_grid[i][1] = snake.snake_grid[i-1][1];
    }
    switch(snake.direction)     //移动    
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

/*****************************************食物*****************************************/
int Check(int i,int j)    //检查空位放食物
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
    while(Check(i,j) == 0);  //检查是否为空位    
    map[i][j] = -1;          //生成食物  
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
    while(Check(i,j) == 0);  //检查是否为空位   
    map[i][j] = -3;          //生成“奖励果实”
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
    while(Check(i,j) == 0);  //检查是否为空位   
    map[i][j] = -2;          //生成“魅惑果实”

    
}

void Eatfood(void)
{
    if(map[snake.snake_grid[0][0]][snake.snake_grid[0][1]] == -1)
    {
        eated = 1;          //标记吃到了食物
        if(di_flag)         //音效标志位开启才会响
            di();
        score++;            //分数加一
        eatfood_num++;      //标记吃到食物的数量加一
        if(eatfood_num == 4 && mode_flag!=2)    
        {           
            eatfood_num = 0;            
            set_goodfood();
            __HAL_TIM_SetCounter(&g_timx_handle , 0);   //计数器清零（非困难模式）
            
        }
        else
        {
            set_food();
            __HAL_TIM_SetCounter(&g_timx_handle , 0);   //计数器清零（困难模式）
        }
            
        map[snake.snake_grid[0][0]][snake.snake_grid[0][1]] = 0;    //食物位置清零
    }
    
    if(map[snake.snake_grid[0][0]][snake.snake_grid[0][1]] == -3)   //吃到了“奖励果实”
    {
        eated = 1;          //标记吃到了食物
        if(di_flag)         //音效标志位开启才会响
            di();
        score+=2;            //分数加2
        set_food();         //重新放食物
        map[snake.snake_grid[0][0]][snake.snake_grid[0][1]] = 0;    //食物位置清零
    }
    
    if(mode_flag == 2 && map[snake.snake_grid[0][0]][snake.snake_grid[0][1]] == -2)
    {
        eated = -1;          //标记吃到了“魅惑果实”
        if(di_flag)         //音效标志位开启才会响
            di();
        score--;            //分数减一
        bad_score++;        //“魅惑果实”分数加一
        set_badfood();         //重新放“魅惑果实”
        map[snake.snake_grid[0][0]][snake.snake_grid[0][1]] = 0;    //食物位置清零
    }
}

/*****************************************游戏开始和结束****************************************************/
void game_start(void)
{
    game_start_flag = 1;
    GUI_clear();
    map_init();
    snake_init();
    set_food();
    if(mode_flag == 2)          //困难模式放20个“魅惑果实”
    {
        for(int i=0;i<20;i++)
        {
            set_badfood();
        }
    }

    while(1)
    {        
        int isgameover = game_over();
        int return_flag = 0;    // 返回菜单的标志
        GUI_Refresh();
        if(isgameover == 0)     // 游戏未结束
        {
            show_score();   // 显示分数        
            Move();         // 蛇移动函数    
            Eatfood();      // 吃到食物
            Draw_snake();   // 画蛇
            delay_ms(speed);// 移动速度
            if(mode_flag != 0 && speed>20)  //一般模式下，吃三个食物，speed-5
            {
                if(score%3 == 0 && score != 0 && eated == 1)
                    speed-=5;
            }
            if(tpad_scan(0))    // 电容触摸按下游戏暂停
            {
                lcd_clear(LGRAY);
                while(1)
                {                    
                    text_show_string_middle(0,300,"游戏暂停",24,lcddev.width,RED);
                    text_show_string_middle(0,350,"按两次左键返回主菜单",24,lcddev.width,BLACK);
                    text_show_string_middle(0,400,"右键继续",24,lcddev.width,BLACK);
                    text_show_string_middle(0,450,"先按一次左键，再按一次右键，游戏重新开始",24,lcddev.width,BLACK);
                    
                    uint8_t key = key_scan(0);
                    if(key == KEY0_PRES)    // 右键被按下 游戏继续
                    {
                       map_init();
                       break;
                    }
                       
                    if(key == KEY2_PRES)    // 左键被按下，返回主菜单
                    {
                        return_flag = 1;
                        break;
                    }
                      
                }

            }
            if(return_flag == 1)
                break;  
        }
        else    // 游戏结束
        {
            lcd_clear(LGRAY);
            text_show_string_middle(0,300,"GAME OVER!",32,lcddev.width,RED);
            text_show_string_middle(0,350,"请按左键返回主菜单",24,lcddev.width,BLACK);
            text_show_string_middle(0,400,"如果您想重新开始，请按右键",24,lcddev.width,BLACK);
            show_score();
            
            //将分数存入数组
            sort(score);
            break;
        }
            
    }
    
}

int game_over(void)
{
    int isGameOver = 0;
    //int sx = snake.snake_grid[0][0],sy = snake.snake_grid[0][1];
    
    if(mode_flag != 0)                          //简单模式撞墙不死
    {
        if(snake.snake_grid[0][0]<0 || snake.snake_grid[0][0]>29 || snake.snake_grid[0][1]<0 || snake.snake_grid[0][1]>59)      //判断撞墙
            isGameOver = 1;
    }
    
    if(mode_flag == 2)                          //困难模式，蛇身小于初始长度就死
    {
        if(snake.length < 4)
            isGameOver = 1;
    }
    
    for(int i = 1;i < snake.length;i++)
    {
        if(snake.snake_grid[0][0] == snake.snake_grid[i][0] && snake.snake_grid[0][1] == snake.snake_grid[i][1])    //判断咬到自己
        {
            isGameOver = 1;
            break;
        }
    }
    return isGameOver;
}
//显示分数
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
            case 0:text_show_string_middle(0,730,"Noka状态良好",24,lcddev.width,RED);break;
            case 1:{lcd_fill(0,730,350,754,LGRAY);text_show_string_middle(0,730,"Noka已中毒 33%",24,lcddev.width,RED);}break;
            case 2:{lcd_fill(0,730,350,754,LGRAY);text_show_string_middle(0,730,"Noka已中毒 66%",24,lcddev.width,RED);}break;
            case 3:{lcd_fill(0,730,350,754,LGRAY);text_show_string_middle(0,730,"Noka已中毒",24,lcddev.width,RED);}break;           
        }
        
    }
}


/************************************************菜单设置的功能函数**********************************/

//蛇的皮肤函数
void skin_black(void)
{
    color = BLACK;
    text_show_string_middle(0,500,"设置成功",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}
void skin_green(void)
{
    color = GREEN;
    text_show_string_middle(0,500,"设置成功",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}
void skin_blue(void)
{
    color = BLUE;
    text_show_string_middle(0,500,"设置成功",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}



/*---------------------------------------------游戏模式-----------------------------------------*/
/*
    简单模式下，速度不变且最慢，死亡模式只有咬到自己才会死，撞墙不会死。
*/
void easy_mode(void)
{
    speed = 100;
    mode_flag = 0;
    text_show_string_middle(0,500,"设置成功",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}
/*
    一般模式下，初始速度为150，最高速度为speed=50，每吃到3个食物，速度speed-5，
    直到speed=50，即分数为50时就会到达最高速度，死亡模式为撞墙或咬到自己;
*/
void normal_mode(void)  
{
    speed = 100;
    mode_flag = 1;
    text_show_string_middle(0,500,"设置成功",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}
/*
    困难模式下，初始速度为100，最大速度50，每吃到3个果实，speed-5，即score = 30后，速度最大，
    会随机生成多个“魅惑果实”，以品红色的颜色呈现，
    当蛇迟到“魅惑果实”时，蛇身减1，分数减1，累计吃到3个果实蛇会不受控制，按键相反，即按上键往下走，按左键往右走，
    同时，当蛇身长度小于初始值时，蛇会死亡。
*/
void hard_mode(void)
{
    speed = 75;
    mode_flag = 2;
    text_show_string_middle(0,500,"设置成功",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}

/**
    音效开关函数
**/
void di_open(void)
{
    di_flag = 1;
    text_show_string_middle(0,500,"设置成功",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}
void di_close(void)
{
    di_flag = 0;
    text_show_string_middle(0,500,"设置成功",24,lcddev.width,RED);
    delay_ms(1000);
    lcd_fill(0,500,350,550,LGRAY);
}

/**
    排行榜功能函数，将最新获得的分数与之前的分数比较，如果小于最低分，则不计入；
    如果大于最低分，加入数组并进行排序，采用冒泡排序的方法。
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


void sort(uint8_t score)    //分数排序
{
    if(mode_flag == 0)
    {
        if(score > score_easy_num[9])   //比最低分要高才计入成绩
        {
            score_easy_num[9] = score;
            maopao(score_easy_num);
            norflash_write(score_easy_num,flashsize - 200,sizeof(score_easy_num));  //写入数据
        }
    }
    else if(mode_flag == 1)
    {
        if(score > score_normal_num[9])   //比最低分要高才计入成绩
        {
            score_normal_num[9] = score;
            maopao(score_normal_num);
            norflash_write(score_normal_num,flashsize - 150,sizeof(score_normal_num));  //写入数据
        }
    }
    else if(mode_flag == 2)
    {
        if(score > score_hard_num[9])   //比最低分要高才计入成绩
        {
            score_hard_num[9] = score;
            maopao(score_hard_num);
            norflash_write(score_hard_num,flashsize - 100,sizeof(score_hard_num));  //写入数据
        }
    }
    
}


void ranking_show(void)                 //排行榜显示函数
{
    norflash_read(score_easy_num, flashsize - 200, sizeof(score_easy_num));   /* 从倒数第200个地址处开始,读出SIZE个字节 */
    norflash_read(score_normal_num, flashsize - 150, sizeof(score_normal_num));
    norflash_read(score_hard_num, flashsize - 100, sizeof(score_hard_num));
    lcd_clear(LGRAY);
    text_show_string(50,100,lcddev.width,lcddev.height,"简单模式",24,1,BLACK);
    for(int i=0;i<10;i++)
    {
        lcd_show_xnum(50,148+i*48,score_easy_num[i],4,24,1,RED);
    }
    text_show_string(185,100,lcddev.width,lcddev.height,"普通模式",24,1,BLACK);
    for(int i=0;i<10;i++)
    {
        lcd_show_xnum(185,148+i*48,score_normal_num[i],4,24,1,RED);
    }
    text_show_string(320,100,lcddev.width,lcddev.height,"困难模式",24,1,BLACK);
    for(int i=0;i<10;i++)
    {
        lcd_show_xnum(320,148+i*48,score_hard_num[i],4,24,1,RED);
    }
}



/**
    游戏简介
**/
void intro(void)
{
    lcd_clear(LGRAY);
    //每行算上标点符号最多30个字。
    text_show_string_middle(0,0,"在遥远的帕达姆特星球的一个半岛上，生活着一群永远吃不饱的生物，由于长期的进食，导致该半岛资源匮乏，无法供应所有个体生存。于是，为了延续种族，他们派出了一群行动迅速的个体出发去寻找新的家园，而这些个体有一个统称——Noka。",24,lcddev.width,BLACK);
    text_show_string_middle(0,160,"简单模式下，Noka来到了一个物质资源非常丰富的地区，这里有着吃不完的食物，每次吃完第二天都会长出来，而且这里没有豺狼虎豹，更没有它们的天敌。而且每隔几天都会长出非常美味的果实，不过要注意及时吃完，不然就会消失。",24,lcddev.width,BLACK);
    text_show_string_middle(0,330,"普通模式下，Noka来到了一个物资也很丰富的地区，只不过这里的食物像毒品一般，吃的越多越想吃，吃的速度也越来越快，Noka像疯了一样去进食，而且围墙外面还有它们的天敌，只要出了围墙，就会成为别人的食物！",24,lcddev.width,BLACK);    text_show_string_middle(0,480,"困难模式下，Noka来到了禁区，这里的食物每天都会在指定时间出现，但只会出现很短的时间。同时，这里的危险不止城墙外的天敌，这里孕育大量的“魅惑果实”，如果Noka忍不住饥饿，吃了三个及以上的果实，就会发生离奇的事情......",24,lcddev.width,BLACK);
    text_show_string_middle(0,700,"以上故事纯属虚构，如有雷同，纯属巧合 :)",16,lcddev.width,RED);
}



/**
 * @brief       中断服务程序中需要做的事情
 *              在HAL库中所有的外部中断服务函数都会调用此函数
 * @param       GPIO_Pin:中断引脚号
 * @retval      无
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    delay_ms(20);      /* 消抖 */
    switch(GPIO_Pin)
    {
        case KEY0_INT_GPIO_PIN:
            if (KEY0 == 0)
            {
                if(mode_flag == 2 && bad_score>=3)  //困难模式下吃三个果实会逆转方向
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
 * @brief       通用定时器TIMX定时中断初始化函数
 * @note
 *              通用定时器的时钟来自APB1,当PPRE1 ≥ 2分频的时候
 *              通用定时器的时钟为APB1时钟的2倍, 而APB1为42M, 所以定时器时钟 = 84Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 * @param       arr: 自动重装值
 * @param       psc: 预分频系数
 * @retval      无
 */
void gtim_timx_int_init(uint32_t arr, uint32_t psc)
{
    GTIM_TIMX_INT_CLK_ENABLE();                             /* 使能TIMx时钟 */

    g_timx_handle.Instance = GTIM_TIMX_INT;                 /* 通用定时器x */
    g_timx_handle.Init.Prescaler = psc;                     /* 预分频系数 */
    g_timx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;    /* 递增计数模式 */
    g_timx_handle.Init.Period = arr;                        /* 自动装载值 */
    HAL_TIM_Base_Init(&g_timx_handle);
    
    HAL_NVIC_SetPriority(GTIM_TIMX_INT_IRQn, 4, 3);         /* 设置中断优先级，抢占优先级1，子优先级3 */
    HAL_NVIC_EnableIRQ(GTIM_TIMX_INT_IRQn);                 /* 开启ITMx中断 */

    HAL_TIM_Base_Start_IT(&g_timx_handle);                  /* 使能定时器x和定时器x更新中断 */
}

/**
 * @brief       定时器中断服务函数
 * @param       无
 * @retval      无
 */
void GTIM_TIMX_INT_IRQHandler(void)
{
    /* 以下代码没有使用定时器HAL库共用处理函数来处理，而是直接通过判断中断标志位的方式 */
    if(__HAL_TIM_GET_FLAG(&g_timx_handle, TIM_FLAG_UPDATE) != RESET)
    {
        if(game_start_flag == 1 && mode_flag!=2)    // 非困难模式刷新食物
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

        if(mode_flag == 2 && game_start_flag == 1)  // 困难模式刷新食物
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
         
        //HAL_NVIC_DisableIRQ(GTIM_TIMX_INT_IRQn);            //关定时器
        __HAL_TIM_CLEAR_IT(&g_timx_handle, TIM_IT_UPDATE);  /* 清除定时器溢出中断标志位 */
        
        
    }
}


