#include <Arduino.h>
#include <FreeRTOS.h>
#include <U8g2lib.h>
#include <Versatile_RotaryEncoder.h>
#include "keypad.h"
#include "key.h"
#include "encoder.h"
#include "main.h"
#include "../lib/PCF8574_library/PCF8574.h"

#ifdef U8X8_HAVE_HW_SPI

#include <SPI.h>

#endif
#ifdef U8X8_HAVE_HW_I2C

#include <Wire.h>

#endif

SemaphoreHandle_t xMutex;

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA, /* reset=*/ U8X8_PIN_NONE);
Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


TwoWire I2Cone = TwoWire(0);
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


    QueueHandle = xQueueCreate(100, sizeof(Event_t));
    //OLEDQueueHandle = xQueueCreate(100, sizeof(Event_t));
    xMutex = xSemaphoreCreateMutex();
    // Check if the queue was successfully created
    if(QueueHandle == NULL){
        Serial.println("Queue could not be created. Halt.");
        while(1) delay(1000); // Halt at this point as is not possible to continue
    }

    xTaskCreate(
            TaskEncoder, "Task Encoder" // A name just for humans
            ,
            2048        // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
            , nullptr // Task parameter which can modify the task behavior. This must be passed as pointer to void.
            , 1  // Priority
            , nullptr // Task handle is not used here - simply pass NULL
    );
//    xTaskCreate(
//            TaskOLED, "Task OLED" // A name just for humans
//            ,
//            2048        // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
//            , nullptr // Task parameter which can modify the task behavior. This must be passed as pointer to void.
//            , 2  // Priority
//            , nullptr // Task handle is not used here - simply pass NULL
//    );
    xTaskCreate(
            TaskButton, "Task Button" // A name just for humans
            ,
            2048        // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
            , nullptr // Task parameter which can modify the task behavior. This must be passed as pointer to void.
            , 1  // Priority
            , nullptr // Task handle is not used here - simply pass NULL
    );
    xTaskCreate(
            TaskKeypad, "Task Keypad" // A name just for humans
            ,
            1024        // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
            , nullptr // Task parameter which can modify the task behavior. This must be passed as pointer to void.
            , 1  // Priority
            , nullptr // Task handle is not used here - simply pass NULL
    );
    xTaskCreate(
            TaskReadEvent, "Task ReadEvent" // A name just for humans
            ,
            2048        // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
            , nullptr // Task parameter which can modify the task behavior. This must be passed as pointer to void.
            , 1  // Priority
            , nullptr // Task handle is not used here - simply pass NULL
    );

    u8g2.begin();
}

void drawURL(void)
{
    u8g2.setFont(u8g2_font_inb16_mf);
    u8g2.drawStr(1,30,strshow);
}

void loop() {
    u8g2.firstPage();
    do {
        xSemaphoreTake(xMutex, 200);
        drawURL();
        xSemaphoreGive(xMutex);
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

void handlePress() {

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

void TaskOLED(void *pvParameters) {
    u8g2.firstPage();
    char tempStr[100];
    for (;;) {

        delay(50);
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


        if(event_temp.type != -1){
            // 使用 sprintf 将 Event_t 的内容格式化并存储到字符串中
            char tempStr[100];
            // 使用 sprintf 将 Event_t 的内容格式化并存储到临时字符串中
            sprintf(tempStr, "t%d|n%d|e%d", event_temp.type, event_temp.num,event_temp.event);
            xSemaphoreTake(xMutex, portMAX_DELAY);
            strshow = tempStr;
            xSemaphoreGive(xMutex);

            Serial.print("Event type: ");
            Serial.print(event_temp.type);
            Serial.print(" Event num: ");
            Serial.print(event_temp.num);
            Serial.print(" Event event: ");
            Serial.println(event_temp.event);
        }

        delay(50);

    }
}

Event_t QRecv() {
    Event_t event_temp;
    if (QueueHandle != NULL) { // Sanity check just to make sure the queue actually exists
        int ret = xQueueReceive(QueueHandle, &event_temp, portMAX_DELAY);
        if (ret == pdPASS) {
            return event_temp;
        }
            // The item is queued by copy, not by reference, so lets free the buffer after use.
        else if (ret == pdFALSE) {
            Serial.println("The `TaskWriteToSerial` was unable to receive data from the Queue");

        }
//    else{
//        delay(100);
//    }
    }
        return event_temp = {
                -1,-1,-1
        };
}