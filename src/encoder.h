//
// Created by mastwet on 24-8-19.
//

#ifndef CHOCOLY_ENCODER_H
#define CHOCOLY_ENCODER_H

//实际上这个部分的按键是接在按键拓展上面的，所以实际上Versatile_RotaryEncoder自带的读取按键功能不能使用
//使用一个未使用的GPIO来代替

#define clk1   7  // (A3)
#define dt1    8   // (A2)
#define sw1    39   // (A4)

#define clk2   9  // (A3)
#define dt2    10   // (A2)
#define sw2    39   // (A4)

#define clk3   11  // (A3)
#define dt3    12   // (A2)
#define sw3    39   // (A4)

#define clk4   13  // (A3)
#define dt4    14   // (A2)
#define sw4    39   // (A4)

#endif //CHOCOLY_ENCODER_H
