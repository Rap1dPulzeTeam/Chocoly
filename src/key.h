//
// Created by mastwet on 24-8-19.
//



// This file contains predefined setup for various Adafruit Matrix Keypads.
#ifndef __KEYPAD_CONFIG_H__
#define __KEYPAD_CONFIG_H__

#define R1    16
#define R2    17
#define R3    18
#define R4    5
#define C1    46
#define C2    45
#define C3    42
#define C4    15

const byte ROWS = 4; // rows
const byte COLS = 4; // columns
// define the symbols on the buttons of the keypads
char keys[ROWS][COLS] = {{0x01, 0x02, 0x03, 0x04},
                         {0x05, 0x06, 0x07, 0x08},
                         {0x09, 0x0a, 0x0b, 0x0c},
                         {0x0d, 0x0e, 0x0f, 0x10}};
byte rowPins[ROWS] = {R1, R2, R3,
                      R4}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {C1, C2, C3,
                      C4}; // connect to the column pinouts of the keypad

#endif //CHOCOLY_KEY_H
