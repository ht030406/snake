#ifndef __GAME_H
#define __GAME_H


#include "./SYSTEM/sys/sys.h"



struct MenuItem
{
    short MeunCount;    //当前菜单项目总数
    uint16_t *DisplayString;  //当前要显示的字符
    void (*Subs)();      //选择某一个菜单后要执行的功能函数
    struct MenuItem *next;  //当前项目的子菜单
    struct MenuItem *prev;  //当前项目的父菜单
};

struct snake
{                           //蛇的结构体
    int snake_grid[1800][2];
    int length;
    int direction;          //“0”表示右移，“1”下移，“2”左移，“3”上移
};


void GUI_clear(void);                                                //清屏
void head(int i,int j);                                              //画蛇头
void body(int i,int j,uint16_t color);                               //画蛇身
void food(int i,int j);                                              //画食物
void GUI_Refresh(void);                                              //更新屏幕
void snake_init(void);                                               //蛇的初始化
void Draw_snake(void);                                               //画蛇
void Get_key(void);                                                  //获取按键操作蛇
void Move(void);                                                     //蛇移动
int Check(int i,int j);                                              //检查空点放食物
void set_food(void);                                                 //放食物
void set_badfood(void);                                              //放“魅惑果实”
void set_goodfood(void);                                             //放“奖励果实”
void Eatfood(void);                                                  //判断吃到食物
void game_start(void);                                               //游戏开始
int game_over(void);                                                 //游戏结束
void display_menu(struct MenuItem * MenuPoint,short rowItem);        //菜单绘制
void map_init(void);                                                 //地图初始化
void show_score(void);                                               //得分显示


/***************************菜单功能函数**************************/
void skin_black(void);  //蛇的皮肤
void skin_green(void);
void skin_blue(void);

void easy_mode(void);   //游戏难度
void normal_mode(void);
void hard_mode(void);
    
void di_open(void);     //音效开
void di_close(void);    //音效关

void maopao(uint8_t *num);  //冒泡排序
void sort(uint8_t score);   //分数排序
void ranking_show(void); //排行榜显示函数

void intro(void);       //游戏简介

void choose_menu(void);
#endif

