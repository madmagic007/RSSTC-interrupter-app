#include "Timers.h"
#include "Capability.h"

#include <Arduino.h>
#include <soc/gpio_struct.h>

static hw_timer_t* timer = nullptr;
static volatile u8_t stage = 0;
static volatile u8_t zcCount = 0;

static void IRAM_ATTR timerISR() {
    switch (stage) {
        case 0: {
            stage = 1;
            timerWrite(timer, 0);
            timerAlarm(timer, delayUS.value, false, 0);
            break;
        }
        case 1: {
            stage = 2;
            timerWrite(timer, 0);
            timerAlarm(timer, pwUS.value, false, 0);

            GPIO.out_w1ts.val = (1 << 1);
            break;
        }
        case 2: {
            GPIO.out_w1tc.val = (1 << 1);
            stage = 99;
            break;
        }
    }
}

static void IRAM_ATTR gpioISR() {
    zcCount = zcCount + 1;
    if (zcCount < zcSkipCount.value) return;

    zcCount = 0;
    stage = 0;
    timerWrite(timer, 1);
    timerAlarm(timer, 1, false, 0);
}

void setupTimers() {
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);

    pinMode(16, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(16), gpioISR, RISING);

    timer = timerBegin(1000000); // 1MHz
    timerAttachInterrupt(timer, timerISR);
    timerAlarm(timer, 1, false, 0);
}
