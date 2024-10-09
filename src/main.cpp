#include <Arduino.h>
#include <FreeRTOS.h>
#include <U8g2lib.h>
#include <Versatile_RotaryEncoder.h>
#include "keypad.h"
#include "key.h"
#include "encoder.h"
#include "main.h"
#include "midi.h"
#include "../lib/PCF8574_library/PCF8574.h"

#ifdef U8X8_HAVE_HW_SPI

#include <SPI.h>

#endif
#ifdef U8X8_HAVE_HW_I2C

#include <Wire.h>

#endif

SemaphoreHandle_t xMutex;

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA);
Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


TwoWire I2Cone = TwoWire(1);
PCF8574 pcf8574(&I2Cone, 0x20);

Versatile_RotaryEncoder *encoder_1;
Versatile_RotaryEncoder *encoder_2;
Versatile_RotaryEncoder *encoder_3;
Versatile_RotaryEncoder *encoder_4;

//TaskHandle_t keypad_read_task_handle;
//TaskHandle_t OLED_Read_Task_Handle;

//管理encoder的值
int encoder_value[8] = {0};
char* strshow = "helloworld!";
display_event display_string = {1, "hello_world!", "", 0,0};

//初始化，正常情况下会读取SPIFFS读取先前保存的数据，如果没有数据，则默认为0
//全局变量,KEYPAD,KNOB等变量, 一共8页
keypad keypad_bank[16][8] = {0};
knob knob_bank[4][8] = {0};
button button_bank[8][8] = {0};

int current_keypad_page = 0;
int current_knob_page = 0;
int current_button_page = 0;


//用于设定普通模式下具体按下的按键，如果0，则没有按键按下，正常发送逻辑。
//编码器1按下的时候并旋转，选择PADPADGE  1
//编码器2按下的时候并旋转，选择KNOBPAGE  2
//编码器3按下的时候旋转，走带控制  3
//编码器4按下的时候旋转，选择KEYPAGE    4
int normal_mod_set_type = 0;



//全局变量，是否进入菜单
bool menu_flag = false;
//用于菜单，选择当前激活的PAD
int menu_key_selected = 0;
//用于菜单，选择当前激活的KNOB
int menu_knob_selected = 0;
//用于菜单，选择当前激活的BUTTON
int menu_button_selected = 0;
//用于菜单，选择当前激活的设置
int menu_setting_mode = 0;



/*-------------------------------------*/
/*   事件队列的逻辑：
 * 1、识别按键\编码器\按钮事件
 * 2、读取队列中进行处理，如果说是正常模式，则直接队列输出，如果非正常模式（例如进入了菜单）
 * 则判断为事件，输出全局事件组
 * 3、后续的UI，例如说未来可能会加入的Astra_arduino框架会使用事件组进行操作，
 */
/*-------------------------------------*/

void setup() {
    Serial.begin(115200);
    init_button();
    encoder_init();
    u8g2.begin();

//    QueueHandle = xQueueCreate(100, sizeof(Event_t));
//    //OLEDQueueHandle = xQueueCreate(100, sizeof(Event_t));
//    xMutex = xSemaphoreCreateMutex();
//    // Check if the queue was successfully created
//    if(QueueHandle == NULL){
//        Serial.println("Queue could not be created. Halt.");
//        while(1) delay(1000); // Halt at this point as is not possible to continue
//    }

//    xTaskCreate(
//            TaskEncoder, "Task Encoder" // A name just for humans
//            ,
//            2048        // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
//            , nullptr // Task parameter which can modify the task behavior. This must be passed as pointer to void.
//            , 1  // Priority
//            , nullptr // Task handle is not used here - simply pass NULL
//    );
////    xTaskCreate(
////            TaskOLED, "Task OLED" // A name just for humans
////            ,
////            2048        // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
////            , nullptr // Task parameter which can modify the task behavior. This must be passed as pointer to void.
////            , 2  // Priority
////            , nullptr // Task handle is not used here - simply pass NULL
////    );
//    xTaskCreate(
//            TaskButton, "Task Button" // A name just for humans
//            ,
//            2048        // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
//            , nullptr // Task parameter which can modify the task behavior. This must be passed as pointer to void.
//            , 1  // Priority
//            , nullptr // Task handle is not used here - simply pass NULL
//    );
//    xTaskCreate(
//            TaskKeypad, "Task Keypad" // A name just for humans
//            ,
//            1024        // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
//            , nullptr // Task parameter which can modify the task behavior. This must be passed as pointer to void.
//            , 1  // Priority
//            , nullptr // Task handle is not used here - simply pass NULL
//    );
//    xTaskCreate(
//            TaskReadEvent, "Task ReadEvent" // A name just for humans
//            ,
//            4096        // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
//            , nullptr // Task parameter which can modify the task behavior. This must be passed as pointer to void.
//            , 1  // Priority
//            , nullptr // Task handle is not used here - simply pass NULL
//    );


}

//几种屏幕显示事件：
//0：无动作
//1：文字
//2：进度条

//当为2的时候判断为编码器事件

char items[4] = {'K', 'N', 'B', 'S'}; // 左侧菜单选项
int selectedIndex = 3; // 当前选中的索引



void menu_main_ui() {
    // 绘制 "PAGE:1" 字符串在顶部
    u8g2.setFont(u8g2_font_6x10_tf); // 使用更小的字体
    u8g2.setCursor(20, 10); // 设置顶部文字坐标
    u8g2.drawVLine(20, 0, 64); // 绘制垂直分隔线，从顶部到屏幕底部
    for (int i = 0; i < 5; i++) {
        if (i == selectedIndex) {
            u8g2.setDrawColor(1); // 设置绘制颜色为白色
            u8g2.drawBox(0, i * 16, 16, 16); // 画选中背景色块
            u8g2.setDrawColor(0); // 设置绘制颜色为黑色
            u8g2.setCursor(4, (i + 1) * 16 - 4); // 设置文字坐标
            u8g2.print(items[i]); // 打印字母，反色显示
        } else {
            u8g2.setDrawColor(1); // 设置绘制颜色为白色
            u8g2.setCursor(4, (i + 1) * 16 - 4); // 设置文字坐标
            u8g2.print(items[i]); // 打印字母
        }
    }
    u8g2.setCursor(0,0); // 设置文字坐标

    // 绘制右侧内容，根据不同的选择显示不同的内容
    if (selectedIndex == 0) {
        // 如果选择了 'K'
        u8g2.setFont(u8g2_font_5x8_tr); // 使用较小的字体
        u8g2.setCursor(30, 14);
        u8g2.print("K Page:");

        // 绘制 4x4 的格子
        int gridSize = 10;
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 4; col++) {
                u8g2.drawFrame(30 + col * gridSize, 20 + row * gridSize, gridSize, gridSize);
            }
        }
        u8g2.setCursor(75, 28);
        u8g2.print("ParamA: 5");
        u8g2.setCursor(75, 36);
        u8g2.print("ParamB: 15");
        u8g2.setCursor(75, 44);
        u8g2.print("ParamC: 25");
        u8g2.setCursor(75, 52);
        u8g2.print("ParamD: 25");

    } else if (selectedIndex == 1) {
        // 如果选择了 'N'
        u8g2.setFont(u8g2_font_5x8_tr); // 使用较小的字体
        u8g2.setCursor(30, 16);
        u8g2.print("N Page:");

        // 绘制四个圆圈，及其参数
        int circleRadius = 4;
        for (int i = 0; i < 4; i++) {
            u8g2.drawCircle(32 + i * 16, 30, circleRadius);
        }
        u8g2.setCursor(30, 48);
        u8g2.print("Param1: 10");
        u8g2.setCursor(30, 56);
        u8g2.print("Param2: 20");
        u8g2.setCursor(30, 64);
        u8g2.print("Param3: 30");
    } else if (selectedIndex == 2) {
        // 如果选择了 'B'
        u8g2.setFont(u8g2_font_5x8_tr); // 使用较小的字体
        u8g2.setCursor(30, 16);
        u8g2.print("PAGE:");

        // 绘制三个方形，及其参数，防止溢出屏幕
        int boxSize = 10;
        for (int i = 0; i < 3; i++) {
            int x = 30 + i * 20;
            int y = 25;
            u8g2.drawFrame(x, y, boxSize, boxSize);
        }
        u8g2.setCursor(30, 48);
        u8g2.print("ParamA: 5");
        u8g2.setCursor(30, 56);
        u8g2.print("ParamB: 15");
        u8g2.setCursor(30, 64);
        u8g2.print("ParamC: 25");
    }
    else if(selectedIndex == 3){
        u8g2.setFont(u8g2_font_6x12_t_cyrillic); // 使用较小的字体
        // 绘制菜单标题 "MENU"，并填充区域
        u8g2.setDrawColor(1); // 设置绘制颜色为白色
        u8g2.drawBox(30, 0, 98, 10); // 填充从 X 轴 30 到 128 的区域，高度为 16
        u8g2.setDrawColor(0); // 设置文字颜色为黑色，确保文字可见
        u8g2.setCursor(60, 8);
        u8g2.print("MENU");
        // 绘制菜单选项 A, B, C
        u8g2.setDrawColor(1); // 恢复绘制颜色为白色
        u8g2.setCursor(30, 22);
        u8g2.print("Option A");
        u8g2.setCursor(30, 32);
        u8g2.print("Option B");
        u8g2.setCursor(30, 42);
        u8g2.print("Option C");
// 假设有一个变量 selectedOption 表示当前选中的选项 (A=0, B=1, C=2)
        int selectedOption = 1; // 示例值，假设当前选中了 Option B

// 绘制选中选项的方框标注
        switch (selectedOption) {
            case 0: // 选中 Option A
                u8g2.drawFrame(58, 24, 70, 12); // 绘制选中框
                break;
            case 1: // 选中 Option B
                u8g2.drawFrame(28, 34, 70, 10); // 绘制选中框
                break;
            case 2: // 选中 Option C
                u8g2.drawFrame(58, 40, 70, 12); // 绘制选中框
                break;
        }

    }
}


void draw_knob(void)
{

    // 绘制 "PAGE:1" 字符串在顶部
    u8g2.setFont(u8g2_font_6x10_tf); // 使用更小的字体
    u8g2.drawStr(0, 8, "PAGE:1");

    // 进度条数值
    int knob1_value = knob_bank[current_keypad_page][0].value;  // 进度条值
    int knob2_value = knob_bank[current_keypad_page][1].value;
    int knob3_value = knob_bank[current_keypad_page][2].value;
    int knob4_value = knob_bank[current_keypad_page][3].value;

    // KNOB1
    u8g2.drawStr(0, 22, "KNOB1:");
    u8g2.drawFrame(40, 14, 80, 8);      // 绘制进度条框架
    u8g2.drawBox(40, 14, knob1_value, 8); // 绘制进度条

    // 绘制数值（反色处理）
    if (knob1_value >= 20) {
        u8g2.setDrawColor(0); // 设置反色
        u8g2.drawStr(40 + knob1_value - 15, 22, String(knob1_value).c_str());
        u8g2.setDrawColor(1); // 恢复正常颜色
    } else {
        u8g2.drawStr(40 + knob1_value + 5, 22, String(knob1_value).c_str());
    }

    // KNOB2
    u8g2.drawStr(0, 34, "KNOB2:");
    u8g2.drawFrame(40, 26, 80, 8);      // 绘制进度条框架
    u8g2.drawBox(40, 26, knob2_value, 8); // 绘制进度条

    // 绘制数值（反色处理）
    if (knob2_value >= 20) {
        u8g2.setDrawColor(0); // 设置反色
        u8g2.drawStr(40 + knob2_value - 15, 34, String(knob2_value).c_str());
        u8g2.setDrawColor(1); // 恢复正常颜色
    } else {
        u8g2.drawStr(40 + knob2_value + 5, 34, String(knob2_value).c_str());
    }

    // KNOB3
    u8g2.drawStr(0, 46, "KNOB3:");
    u8g2.drawFrame(40, 38, 80, 8);      // 绘制进度条框架
    u8g2.drawBox(40, 38, knob3_value, 8); // 绘制进度条

    // 绘制数值（反色处理）
    if (knob3_value >= 20) {
        u8g2.setDrawColor(0); // 设置反色
        u8g2.drawStr(40 + knob3_value - 15, 46, String(knob3_value).c_str());
        u8g2.setDrawColor(1); // 恢复正常颜色
    } else {
        u8g2.drawStr(40 + knob3_value + 5, 46, String(knob3_value).c_str());
    }

    // KNOB4
    u8g2.drawStr(0, 58, "KNOB4:");
    u8g2.drawFrame(40, 50, 80, 8);      // 绘制进度条框架
    u8g2.drawBox(40, 50, knob4_value, 8); // 绘制进度条

    // 绘制数值（反色处理）
    if (knob4_value >= 20) {
        u8g2.setDrawColor(0); // 设置反色
        u8g2.drawStr(40 + knob4_value - 15, 58, String(knob4_value).c_str());
        u8g2.setDrawColor(1); // 恢复正常颜色
    } else {
        u8g2.drawStr(40 + knob4_value + 5, 58, String(knob4_value).c_str());
    }

}

void drawinfo(void)
{
    char buffer[50];
    u8g2.setFont(u8g2_font_6x10_tf);
    //u8g2.drawStr(1,30,strshow);
    if(menu_flag){
        u8g2.drawStr(1, 50, "menu in");
    }
    //普通模式显示，绝大多数的逻辑在TaskReadEvent，这里只负责显示
    switch (display_string.type) {
//        case 0:
//            u8g2.drawStr(1, 30, display_string.line.c_str());
//            break;
        case 0:
            display_string.name = "key";
            snprintf(buffer, sizeof(buffer), "%s: %d", display_string.name.c_str(), display_string.number);
            u8g2.drawStr(1, 30, buffer);
            if(display_string.value == 1){

                u8g2.drawStr(1, 40, "pressed");
            }
            else {
                u8g2.drawStr(1, 40, "released");
            }

            break;

        case 1:
            display_string.name = "button";
            snprintf(buffer, sizeof(buffer), "%s: %d", display_string.name.c_str(), display_string.number);
            u8g2.drawStr(1, 30, buffer);
            if(display_string.value == 0){
                u8g2.drawStr(1, 40, "pressed");
            }
            else {
                // 打印释放时间
                u8g2.drawStr(1, 40, "released");
            }

            break;

        case 2:  //测试 编码器使用
            display_string.name = "knob";
            if(normal_mod_set_type == NORMALMOD){
                draw_knob();
            }
            else {
                switch(normal_mod_set_type){
                    case NORMALMOD_SET_KNOBPAGE:
                        u8g2.drawStr(1, 20, "Change knob page:");
                        u8g2.drawStr(1, 40, "left");
                        break;
                    case NORMALMOD_SET_PADPAGE:
                        u8g2.drawStr(1, 20, "Change pad page:");
                        u8g2.drawStr(1, 40, "left");
                        break;
                    case NORMALMOD_SET_TRACK_CONTROL:
                        u8g2.drawStr(1, 20, "track:");
                        //左转
                        if(display_string.value == 1){
                            u8g2.drawStr(1, 40, "left");
                        }
                            //右转
                        else if(display_string.value == -1){
                            u8g2.drawStr(1, 40, "right");
                        }
                        break;
                    case NORMALMOD_SET_KEYPAGE:
                        u8g2.drawStr(1, 20, "Change key page:");
                        u8g2.drawStr(1, 40, "left");
                        break;

                }
            }

            //snprintf(buffer, sizeof(buffer), "%s: %d", display_string.name.c_str(), display_string.number);
            //u8g2.drawStr(1, 30, buffer);
            //char buffer[20]; // 用于存储格式化后的字符串
            //sprintf(buffer, "value:%d", display_string.value);
            //u8g2.drawStr(1, 40, buffer);
            /*
            if(display_string.value == 1){
                u8g2.drawStr(1, 40, "left");
            }
            else if(display_string.value == -1){
                u8g2.drawStr(1, 40, "right");
            }
            */
            break;
        default:
            u8g2.drawStr(1, 30, display_string.line.c_str());

            break;
    }
    //菜单模式，绝大多数的按键都作为事件进入，处理事件的逻辑在这里
}

void loop() {
    u8g2.firstPage();
    do {
        //xSemaphoreTake(xMutex, 10);
        //drawinfo();
        //draw_knob();
        menu_main_ui();
        //xSemaphoreGive(xMutex);
    } while ( u8g2.nextPage() );
    delay(10);
    //show_oled();
    //read_button();
    //read_encoder();
    //read_keypad();
}

void init_button() {
    I2Cone.begin(4, 3); // SDA pin 16, SCL pin 17, 400kHz frequency
    delay(1000);

    // Set pinMode to OUTPUT
    for (int i = 0; i < 8; i++) {
        pcf8574.pinMode(i, INPUT);
    }

    Serial.print("Init pcf8574...");
    if (pcf8574.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("KO");
    }
    Serial.println("START");
}

int encoder_direct = 0;

void output_value(int num, int direct) {
    if (direct == 1) {
        encoder_value[num]++;
    } else if (direct == -1) {
        encoder_value[num]--;
    }
    Serial.printf("Encoder %d value:%d", num, encoder_value[num]);
}


void handleRotate(int8_t rotation) {
    if (rotation > 0) {
        encoder_direct = 1;
    } else {
        encoder_direct = -1;
    }
}

void encoder_init() {
    encoder_1 = new Versatile_RotaryEncoder(clk1, dt1, sw1);
    encoder_2 = new Versatile_RotaryEncoder(clk2, dt2, sw2);
    encoder_3 = new Versatile_RotaryEncoder(clk3, dt3, sw3);
    encoder_4 = new Versatile_RotaryEncoder(clk4, dt4, sw4);

    encoder_1->setHandleRotate(handleRotate);
    encoder_2->setHandleRotate(handleRotate);
    encoder_3->setHandleRotate(handleRotate);
    encoder_4->setHandleRotate(handleRotate);
//    encoder_1->setHandlePress(handlePress);
//    encoder_2->setHandlePress(handlePress);
//    encoder_3->setHandlePress(handlePress);
//    encoder_4->setHandlePress(handlePress);
    Serial.println("Encoder OK!");
}

void read_encoder() {

    if (encoder_1->ReadEncoder()) {
        //output_value(0, encoder_direct);
        encoder_direct = 0;
    }

    if (encoder_2->ReadEncoder()) {
        //output_value(1, encoder_direct);
        encoder_direct = 0;
    }

    if (encoder_3->ReadEncoder()) {
        //output_value(2, encoder_direct);
        encoder_direct = 0;
    }

    if (encoder_4->ReadEncoder()) {
        //output_value(3, encoder_direct);
        encoder_direct = 0;
    }
}

// 宏定义用于读取当前状态
#define READ_CURRENT_STA(N) (button_event[N].current_sta = di.p##N)
#define UPDATE_LAST_STA(N) (button_event[N].last_sta = button_event[N].current_sta)
#define BUTTON_NOT_ISPRESSED(N) button_event[N].current_sta != button_event[N].last_sta
key_event button_event[8] = {
        {1, 1},
        {1, 1},
        {1, 1},
        {1, 1},
        {1, 1},
        {1, 1},
        {1, 1},
        {1, 1},
};

void read_button() {
    //0
    PCF8574::DigitalInput di = pcf8574.digitalReadAll();

    button_event[0].current_sta = di.p0;
    if (button_event[0].current_sta != button_event[0].last_sta) {
        if(button_event[0].current_sta == 0){
            QSend(BUTTON_TYPE, 0, BUTTON_PRESSED_EVENT);
        }
        else{
            QSend(BUTTON_TYPE, 0, BUTTON_RELEASED_EVENT);
        }
    }
    button_event[0].last_sta = button_event[0].current_sta;

    button_event[1].current_sta = di.p1;
    if (button_event[1].current_sta != button_event[1].last_sta) {
        if(button_event[1].current_sta == 1){
            QSend(BUTTON_TYPE, 1, BUTTON_PRESSED_EVENT);
        }
        else{
            QSend(BUTTON_TYPE, 1, BUTTON_RELEASED_EVENT);
        }
    }
    button_event[1].last_sta = button_event[1].current_sta;

    button_event[2].current_sta = di.p2;
    if (button_event[2].current_sta != button_event[2].last_sta) {
        if(button_event[2].current_sta == 0){
            QSend(BUTTON_TYPE, 2, BUTTON_PRESSED_EVENT);
        }
        else{
            QSend(BUTTON_TYPE, 2, BUTTON_RELEASED_EVENT);
        }
    }
    button_event[2].last_sta = button_event[2].current_sta;

    button_event[3].current_sta = di.p3;
    if (button_event[3].current_sta != button_event[3].last_sta) {
        if(button_event[3].current_sta == 0){
            QSend(BUTTON_TYPE, 3, BUTTON_PRESSED_EVENT);
        }
        else{
            QSend(BUTTON_TYPE, 3, BUTTON_RELEASED_EVENT);
        }
    }
    button_event[3].last_sta = button_event[3].current_sta;

    button_event[4].current_sta = di.p4;
    if (button_event[4].current_sta != button_event[4].last_sta) {
        if(button_event[4].current_sta == 0){
            QSend(BUTTON_TYPE, 4, BUTTON_PRESSED_EVENT);
        }
        else{
            QSend(BUTTON_TYPE, 4, BUTTON_RELEASED_EVENT);
        }
    }
    button_event[4].last_sta = button_event[4].current_sta;

    button_event[5].current_sta = di.p5;
    if (button_event[5].current_sta != button_event[5].last_sta) {
        if(button_event[5].current_sta == 0){
            QSend(BUTTON_TYPE, 5, BUTTON_PRESSED_EVENT);
        }
        else{
            QSend(BUTTON_TYPE, 5, BUTTON_RELEASED_EVENT);
        }
    }
    button_event[5].last_sta = button_event[5].current_sta;

    button_event[6].current_sta = di.p6;
    if (button_event[6].current_sta != button_event[6].last_sta) {
        if(button_event[6].current_sta == 0){
            QSend(BUTTON_TYPE, 6, BUTTON_PRESSED_EVENT);
        }
        else{
            QSend(BUTTON_TYPE, 6, BUTTON_RELEASED_EVENT);
        }
    }
    button_event[6].last_sta = button_event[6].current_sta;

    button_event[7].current_sta = di.p7;
    if (button_event[7].current_sta != button_event[7].last_sta) {
        if(button_event[7].current_sta == 0){
            QSend(BUTTON_TYPE, 7, BUTTON_PRESSED_EVENT);
        }
        else{
            QSend(BUTTON_TYPE, 7, BUTTON_RELEASED_EVENT);
        }
    }
    button_event[7].last_sta = button_event[7].current_sta;
//    READ_CURRENT_STA(7);
//    if (BUTTON_NOT_ISPRESSED(7)) {
//        // button pressed
//        QSend(BUTTON_TYPE, 7, BUTTON_PRESSED_EVENT);
//    } else {
//        QSend(BUTTON_TYPE, 7, BUTTON_RELEASED_EVENT);
//    }
//    UPDATE_LAST_STA(7);



//    Serial.print("READ VALUE FROM PCF P1: ");
//    Serial.print(di.p0);
//    Serial.print(" - ");
//    Serial.print(di.p1);
//    Serial.print(" - ");
//    Serial.print(di.p2);
//    Serial.print(" - ");
//    Serial.println(di.p3);
//    Serial.print(di.p4);
//    Serial.print(" - ");
//    Serial.print(di.p5);
//    Serial.print(" - ");
//    Serial.print(di.p6);
//    Serial.print(" - ");
//    Serial.println(di.p7);

}


//void show_oled() {
//    u8g2.drawHLine(0, 0, 10);
//    u8g2.drawHLine(0, 31, 10);
//    u8g2.drawStr(0, 24, strshow);
//    u8g2.nextPage();
//}

//void TaskKeypad( void *pvParameters ){
//    for(;;){
//        read_keypad();
//        delay(10);
//    }
//}
//
//
//void TaskOLED( void *pvParameters ){
//
//    for(;;){
//        u8g2.drawHLine(0,0,10);
//        u8g2.drawHLine(0,31,10);
//
//        u8g2.setFont(u8g2_font_ncenB10_tr);
//        u8g2.drawStr(0,24,"Hello World!");
//        u8g2.nextPage();
//        delay(10);
//    }
//}

void TaskEncoder(void *pvParameters) {
    for (;;) {
        if (encoder_1->ReadEncoder()) {
            //output_value(0, encoder_direct);
            QSend(ENCODER_TYPE, 0, encoder_direct);
            encoder_direct = 0;
        }

        if (encoder_2->ReadEncoder()) {
            QSend(ENCODER_TYPE, 1, encoder_direct);
            encoder_direct = 0;
        }

        if (encoder_3->ReadEncoder()) {
            QSend(ENCODER_TYPE, 2, encoder_direct);
            encoder_direct = 0;
        }

        if (encoder_4->ReadEncoder()) {
            QSend(ENCODER_TYPE, 3, encoder_direct);
            encoder_direct = 0;
        }
        delay(1);
    }
}

void TaskButton(void *pvParameters) {
    for (;;) {
        read_button();
        delay(10);
    }
}



void TaskKeypad(void *pvParameters) {
    String msg;
    msg = "";
    int kstate = 0;
    for (;;) {
        // Fills kpd.key[ ] array with up-to 10 active keys.
        // Returns true if there are ANY active keys.
        if (kpd.getKeys()) {
            for (int i = 0; i < LIST_MAX; i++)   // Scan the whole key list.
            {
                if (kpd.key[i].stateChanged)   // Only find keys that have changed state.
                {
                    switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                        case PRESSED:
                            //msg = " PRESSED.";
                            kstate = KEY_PRESSED_EVENT;
                            break;
                        case HOLD:
                            //msg = " HOLD.";
                            kstate = KEY_HOLD_EVENT;
                            break;
                        case RELEASED:
                            //msg = " RELEASED.";
                            kstate = KEY_RELEASED_EVENT;
                            break;
                        case IDLE:
                            //msg = " IDLE.";
                            kstate = KEY_IDLE_EVENT;
                    }
//                    Serial.print("Key ");
//                    Serial.print(kpd.key[i].kchar);
//                    Serial.println(msg);
                    QSend(KEYPAD_TYPE, kpd.key[i].kchar, kstate);
                }
            }
        }
        delay(100);
    }
}


void QSend(int type, int num, int event) {
    if (QueueHandle != NULL && uxQueueSpacesAvailable(QueueHandle) > 0) {
        Event_t event_d;
        event_d.event = event;
        event_d.num = num;
        event_d.type = type;

        int ret = xQueueSend(QueueHandle, (void *) &event_d, 0);
        if (ret == pdTRUE) {
        } else if (ret == errQUEUE_FULL) {
            Serial.println("unable to send data");
        }
    }
//    else{
//        delay(100);
//    }
}

////全局变量,KEYPAD,KNOB等变量, 一共8页
//keypad keypad_bank[16][8];
//knob knob_bank[4][8];
//button button_bank[8][8];

void midi_send_note(int channel, int note, int velocity,int onoff) {
    // 输出伪造的MIDI发送信息，用于调试或展示
    printf("MIDI Send: Channel: %d, Note: %d, Velocity: %d\n", channel, note, velocity);
}

void midi_cc_send(int channel, int control_number, int value) {
    // 输出伪造的MIDI CC发送信息，用于调试或展示
    printf("MIDI CC Send: Channel: %d, Control Number: %d, Value: %d\n", channel, control_number, value);
}

//typedef struct {
//    int type;
//    int num;
//    int event;
//} Event_t;

/*船新版本*/



//kepad_event
Event_t handle_keypad_event(int num,int event) {
    switch (event) {
        case KEY_PRESSED_EVENT:
            midi_send_note(keypad_bank[current_keypad_page][num].channel,
                      keypad_bank[current_keypad_page][num].note,
                      keypad_bank[current_keypad_page][num].velocity,
                      1
                      );
            return (Event_t) {
                .type = KEYPAD_TYPE,
                .num = num,
                .event = KEY_PRESSED_EVENT
            };
            break;
        case KEY_RELEASED_EVENT:
            midi_send_note(keypad_bank[current_keypad_page][num].channel,
                           keypad_bank[current_keypad_page][num].note,
                           keypad_bank[current_keypad_page][num].velocity,
                           0
            );
            return (Event_t) {
                    .type = KEYPAD_TYPE,
                    .num = num,
                    .event = KEY_RELEASED_EVENT
            };
            break;
        default:
            return (Event_t) {
                    .type = -1,
                    .num = 0,
                    .event = 0
            };
    }
}
//正常模式，发送MIDI信号并显示在屏幕上
Event_t handle_encoder_normal_event(int num,int event) {
    int value_temp = 0;
    switch (event) {
        case ENCODER_LEFT_EVENT:
            if(knob_bank[current_keypad_page][num].value > 0){
                knob_bank[current_keypad_page][num].value -= 1;
            }
            value_temp = knob_bank[current_keypad_page][num].value;
            midi_cc_send(knob_bank[current_keypad_page][num].channel,
                         knob_bank[current_keypad_page][num].value,
                         knob_bank[current_keypad_page][num].num
            );
            break;
        case ENCODER_RIGHT_EVENT:
            if(knob_bank[current_keypad_page][num].value < 127){
                knob_bank[current_keypad_page][num].value += 1;
            }

            value_temp = knob_bank[current_keypad_page][num].value;
            midi_cc_send(knob_bank[current_keypad_page][num].channel,
                         knob_bank[current_keypad_page][num].value,
                         knob_bank[current_keypad_page][num].num
            );
            break;
    }
    return (Event_t) {
            .type = ENCODER_EVENT_NORMAL,
            .num = num,
            .event = value_temp
    };
}

void handle_button_cc_event(int num,int event) {
    //根据按钮编号执行不同操作，单步触发模式，实际上是只会发送一次播放或者是停止
    switch (event) {
        case BUTTON_PRESSED_EVENT:
            midi_send_note(button_bank[current_keypad_page][num].channel,
                           button_bank[current_keypad_page][num].note,
                           button_bank[current_keypad_page][num].velocity,
                           1
            );
//            return (Event_t) {
//                    .type = BUTTON_EVENT_NORMAL,
//                    .num = num,
//                    .event = BUTTON_PRESSED_EVENT
//            };
            break;
        case BUTTON_RELEASED_EVENT:
            midi_send_note(button_bank[current_keypad_page][num].channel,
                           button_bank[current_keypad_page][num].note,
                           button_bank[current_keypad_page][num].velocity,
                           0
            );
//            return (Event_t) {
//                    .type = BUTTON_EVENT_NORMAL,
//                    .num = num,
//                    .event = BUTTON_RELEASED_EVENT
//            };
            break;
    }
}

void handle_button_pressed_switch_mod(int num,int event) {
     int event_temp = 0;
    switch (num) {
        case 1:
            normal_mod_set_type = NORMALMOD_SET_PADPAGE;
            break;
        case 5:
            normal_mod_set_type = NORMALMOD_SET_KNOBPAGE;
            break;
        case 4:
            normal_mod_set_type = NORMALMOD_SET_TRACK_CONTROL;
            break;
        case 0:
            normal_mod_set_type = NORMALMOD_SET_KEYPAGE;
            break;
        default:
            normal_mod_set_type = NORMALMOD;
            break;
    }
    Serial.printf("current mod:%d \n",normal_mod_set_type);
}


void handle_button_release_switch_mod(int num,int event) {
    normal_mod_set_type = NORMALMOD;
}

//根据按钮编号执行不同操作
//按钮1：选择设置PAD
//按钮2：选择设置编码器
//按钮3：选择设置按钮

void handle_button_menu_event(int num, int event) {
    // 检查按钮按下事件
    if (event == BUTTON_PRESSED_EVENT) {
        switch (num) {
            case 1:  // 按钮1：选择设置PAD
                menu_setting_mode = 1;  // 设置菜单模式为1表示设置PAD
                printf("Menu mode: Set PAD\n");
                break;
            case 2:  // 按钮2：选择设置编码器
                menu_setting_mode = 2;  // 设置菜单模式为2表示设置编码器
                printf("Menu mode: Set Encoder\n");
                break;
            case 3:  // 按钮3：选择设置按钮
                menu_setting_mode = 3;  // 设置菜单模式为3表示设置按钮
                printf("Menu mode: Set Button\n");
                break;
            default:
                printf("Invalid button number.\n");
                break;
        }
    } else if (event == BUTTON_RELEASED_EVENT) {
        // 如果需要在按钮释放时执行某些操作，可以在这里处理
        printf("Button %d released.\n", num);
    }
}
//用于菜单，选择当前激活的PAD
//int menu_key_selected = 0;
//用于菜单，选择当前激活的KNOB
//int menu_knob_selected = 0;
//用于菜单，选择当前激活的BUTTON
//int menu_button_selected = 0;
//按下某个按键选择，然后旋转旋钮A，实现选择NOTE值，旋转旋钮B，选择调整力度，旋转旋钮C，选择整体channel，旋转旋钮D，调整当前设置的页面
void handle_encoder_menu_event(int num, int event){
    switch (num) {
        case 1:  // 操作PAD的选择
//        if(menu_key_selected){
//            if (event == ENCODER_LEFT_EVENT) {
//
//            } else if (event == ENCODER_RIGHT_EVENT) {
//
//            }
//            printf("Current PAD selected: %d\n", menu_key_selected);
//            break;
//        }
//

        case 2:  // 操作KNOB的选择
            if (event == ENCODER_LEFT_EVENT) {

            }
            else if (event == ENCODER_RIGHT_EVENT) {

            }
            printf("Current KNOB selected: %d\n", menu_knob_selected);
            break;

        case 3:  // 操作BUTTON的选择
            if (event == ENCODER_LEFT_EVENT) {

            } else if (event == ENCODER_RIGHT_EVENT) {


            }
            printf("Current BUTTON selected: %d\n", menu_button_selected);
            break;

        default:
            printf("Invalid encoder number: %d\n", num);
            break;
    }
}

//int current_keypad_page = 0;
//int current_knob_page = 0;
//int current_button_page = 0;

// 编码器按下和旋转事件处理函数
void handle_encoder_setpage_event(int num, int event) {
    switch (num) {
        case 1:
            // 编码器1按下并旋转，选择PADPAGE
            if (event == ENCODER_LEFT_EVENT) {  // 假设1表示按下
                if (current_keypad_page < 8) {
                    current_keypad_page++;
                }
            } else if (event == ENCODER_RIGHT_EVENT) {
                if (current_keypad_page > 0) {
                    current_keypad_page--;
                }
            }
            break;

        case 2:
            // 编码器2按下并旋转，选择KNOBPAGE
            if (event == ENCODER_LEFT_EVENT) {  // 假设1表示按下
                if (current_keypad_page < 8) {
                    current_knob_page++;
                }
            } else if (event == ENCODER_RIGHT_EVENT) {
                if (current_keypad_page > 0) {
                    current_knob_page--;
                }
            }
            break;

        case 3:
            // 编码器3按下并旋转，控制走带
            if (event == ENCODER_LEFT_EVENT) {  // 假设1表示按下
                //发送走带控制往左
            }
            else if (event == ENCODER_RIGHT_EVENT) {
                //发送走带控制往右

            }
            break;

        case 4:
            if (event == ENCODER_LEFT_EVENT) {  // 假设1表示按下
                if (current_keypad_page < 8) {
                    current_button_page++;
                }
            } else if (event == ENCODER_RIGHT_EVENT) {
                if (current_keypad_page > 0) {
                    current_button_page--;
                }
            }
            break;

        default:
            break;
    }
}

//菜单键（按键4）连续按下5次，进入设置模式
//按钮1：选择设置PAD  1
//按钮2：选择设置编码器  1
//按钮3：选择设置按钮  1
//设置模式1：设置PAD的CC值
//按下某个按键选择，然后旋转旋钮A，实现选择NOTE值，旋转旋钮B，选择调整力度，旋转旋钮C，选择整体channel，旋转旋钮D，调整当前设置的页面
//设置模式2，设置编码器的CC值
//设置模式3：设置编码器的CC值

//按键统计次数
int menu_button_pressed;
unsigned long pressed_time;
void TaskReadEvent( void *pvParameters ){
    for(;;){
        Event_t event_temp;
        if(QueueHandle != NULL){ // Sanity check just to make sure the queue actually exists
            int ret = xQueueReceive(QueueHandle, &event_temp, portMAX_DELAY);
            if(ret == pdPASS){
                // The item is queued by copy, not by reference, so lets free the buffer after use.
            }else if(ret == pdFALSE){
                Serial.println("The `TaskWriteToSerial` was unable to receive data from the Queue");
            }
        }

        //打印事件信息
        Serial.print("Event type: ");
        Serial.print(event_temp.type);
        Serial.print(" Event num: ");
        Serial.print(event_temp.num);
        Serial.print(" Event event: ");
        Serial.println(event_temp.event);

        //处理事件在这里，调用midi输出也是在这里。
        if(event_temp.type != -1){
            // 使用 sprintf 将 Event_t 的内容格式化并存储到字符串中
            char tempStr[100];

            //处理按钮事件
            if(event_temp.type == BUTTON_TYPE && event_temp.num == 3 && event_temp.event == BUTTON_PRESSED_EVENT){
                pressed_time = millis();
            }
            if(event_temp.type == BUTTON_TYPE && event_temp.event == BUTTON_PRESSED_EVENT){
                if(menu_flag){
                    handle_button_menu_event(event_temp.num, event_temp.event);
                }
                else{
                    //正常模式
                    handle_button_pressed_switch_mod(event_temp.num, event_temp.event);
                    handle_button_cc_event(event_temp.num, event_temp.event);
                }
            }
            //按钮释放事件，这里只为了还原翻页模式设定
            if(event_temp.type == BUTTON_TYPE && event_temp.event == BUTTON_RELEASED_EVENT){
                handle_button_release_switch_mod(event_temp.num, event_temp.event);
            }

            //按钮释放事件处理，这里只处理菜单进入逻辑
            if(event_temp.type == BUTTON_TYPE && event_temp.num == 3 && event_temp.event == BUTTON_RELEASED_EVENT){
                if(millis() - pressed_time < 500){
                    menu_button_pressed++;
                }
                Serial.print(menu_button_pressed);
                if(menu_button_pressed >= 3){
                    menu_flag = !menu_flag;
                    menu_button_pressed = 0;
                }
            }

            //处理键盘矩阵事件
            else if(event_temp.type == KEYPAD_TYPE){
                handle_keypad_event(event_temp.num, event_temp.event);
            }
            //处理编码器事件
            else if(event_temp.type == ENCODER_TYPE){
                if(menu_flag){
                    handle_encoder_menu_event(event_temp.num, event_temp.event);
                }
                else{
                    if(normal_mod_set_type == NORMALMOD){
                        event_temp = handle_encoder_normal_event(event_temp.num, event_temp.event);
                    }
                }
            }

            //传递显示数据
            //获取互斥锁访问共享资源
            xSemaphoreTake(xMutex, portMAX_DELAY);
            display_string.type = event_temp.type;
            display_string.number = event_temp.num;
            display_string.value = event_temp.event;
            //释放
            xSemaphoreGive(xMutex);


        }

        //delay(10);

    }
}
