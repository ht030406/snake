#ifndef __GAME_H
#define __GAME_H


#include "./SYSTEM/sys/sys.h"



struct MenuItem
{
    short MeunCount;    //��ǰ�˵���Ŀ����
    uint16_t *DisplayString;  //��ǰҪ��ʾ���ַ�
    void (*Subs)();      //ѡ��ĳһ���˵���Ҫִ�еĹ��ܺ���
    struct MenuItem *next;  //��ǰ��Ŀ���Ӳ˵�
    struct MenuItem *prev;  //��ǰ��Ŀ�ĸ��˵�
};

struct snake
{                           //�ߵĽṹ��
    int snake_grid[1800][2];
    int length;
    int direction;          //��0����ʾ���ƣ���1�����ƣ���2�����ƣ���3������
};


void GUI_clear(void);                                                //����
void head(int i,int j);                                              //����ͷ
void body(int i,int j,uint16_t color);                               //������
void food(int i,int j);                                              //��ʳ��
void GUI_Refresh(void);                                              //������Ļ
void snake_init(void);                                               //�ߵĳ�ʼ��
void Draw_snake(void);                                               //����
void Get_key(void);                                                  //��ȡ����������
void Move(void);                                                     //���ƶ�
int Check(int i,int j);                                              //���յ��ʳ��
void set_food(void);                                                 //��ʳ��
void set_badfood(void);                                              //�š��Ȼ��ʵ��
void set_goodfood(void);                                             //�š�������ʵ��
void Eatfood(void);                                                  //�жϳԵ�ʳ��
void game_start(void);                                               //��Ϸ��ʼ
int game_over(void);                                                 //��Ϸ����
void display_menu(struct MenuItem * MenuPoint,short rowItem);        //�˵�����
void map_init(void);                                                 //��ͼ��ʼ��
void show_score(void);                                               //�÷���ʾ


/***************************�˵����ܺ���**************************/
void skin_black(void);  //�ߵ�Ƥ��
void skin_green(void);
void skin_blue(void);

void easy_mode(void);   //��Ϸ�Ѷ�
void normal_mode(void);
void hard_mode(void);
    
void di_open(void);     //��Ч��
void di_close(void);    //��Ч��

void maopao(uint8_t *num);  //ð������
void sort(uint8_t score);   //��������
void ranking_show(void); //���а���ʾ����

void intro(void);       //��Ϸ���

void choose_menu(void);
#endif

