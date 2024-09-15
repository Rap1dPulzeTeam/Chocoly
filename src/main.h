//
// Created by mastwet on 24-8-19.
//

#ifndef CHOCOLY_MAIN_H
#define CHOCOLY_MAIN_H

// 不在其他头文件的引脚定义在这里

#include "Keypad.h"

//屏幕
#define OLED_SCL 21
#define OLED_SDA 40

typedef struct {
    int type;
    int num;
    int event;
} Event_t;

typedef struct {
    int last_sta;
    int current_sta;
} key_event;

//几种屏幕显示事件：
//0：无动作
//1：文字
//2：进度条
//value可以对应事件或者是显示的进度
typedef struct {
    int type;
    String line;
    String name;
    int number;
    int value;
} display_event;

void init_button();
void show_oled();
void encoder_init();
void read_button();
void read_encoder();
void read_keypad();


void TaskKeypad( void *pvParameters );
void TaskOLED( void *pvParameters );
void TaskEncoder( void *pvParameters );
void TaskButton( void *pvParameters );
void TaskReadEvent( void *pvParameters );

Event_t QRecv();
void QSend(int type,int num,int event);


typedef enum{
    LEFT,
    RIGHT
}EncoderDirect;

#define MAX_LINE_LENGTH (64)
#define ButtonEvent_t KeyEvent_t

// Define Queue handle
QueueHandle_t QueueHandle;
QueueHandle_t OLEDQueueHandle;

const int QueueElementSize = 10;
typedef struct{
    char line[MAX_LINE_LENGTH];
    uint8_t line_length;
} message_t;

//typedef struct {
//    uint8_t key_num;
//    KeyState pressed;
//} KeyEvent_t;
//
//typedef struct {
//    uint8_t encoder_num;
//    EncoderDirect direct;
//} EncoderEvent_t;

//实际上已经在Keypad.h定义，若要自己实现按键库，请加上
//typedef enum{ IDLE, PRESSED, HOLD, RELEASED } Key_State;

//例如说：keypad event
//type:0 按键 1 按钮 2 编码器
//0和1：
//按下，谈起，无动作
//例如说：encoder event
//2：
//左转，右转

#define KEYPAD_TYPE 0
#define BUTTON_TYPE 1
#define ENCODER_TYPE 2

#define KEY_IDLE_EVENT 0
#define KEY_PRESSED_EVENT 1
#define KEY_HOLD_EVENT 2
#define KEY_RELEASED_EVENT 3

#define ENCODER_LEFT_EVENT 0
#define ENCODER_RIGHT_EVENT 1

#define BUTTON_PRESSED_EVENT 0
#define BUTTON_RELEASED_EVENT 1

#define UIMOD_NORMAL 0
#define UIMOD_CHANGE_KEYPAGE 1
#define UIMOD_CHANGE_KNOBPAGE 2
#define UIMOD_CHANGE_SETTINGS 3
//如果启用了菜单则关闭
#define UIMOD_DISABLE 4


#endif //CHOCOLY_MAIN_H
