//
// Created by mastwet on 24-8-19.
//

#ifndef CHOCOLY_MAIN_H
#define CHOCOLY_MAIN_H

// 不在其他头文件的引脚定义在这里
//屏幕
#define OLED_SCL 21
#define OLED_SDA 40

void init_button();
void show_oled();
void encoder_init();
void read_button();
void read_encoder();
void read_keypad();

#endif //CHOCOLY_MAIN_H
