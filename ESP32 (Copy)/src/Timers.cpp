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
            timerAlarm(timer, delayUS.value + 350, false, 0); // my hardware triggered 350us to early to the actual zero cross
            break;
        }
        case 1: {
            stage = 2;
            timerWrite(timer, 0);
            timerAlarm(timer, pwUS.value, false, 0);

            GPIO.out_w1ts.val = (1 << 17);
            break;
        }
        case 2: {
            GPIO.out_w1tc.val = (1 << 17);
            stage = 99;
            break;
        }
    }
}

static void IRAM_ATTR gpioISR() {
    zcCount = zcCount + 1;
    if (zcCount <= zcSkipCount.value || pwUS.value == 0) return;

    zcCount = 0;
    stage = 0;
    timerWrite(timer, 1);
    timerAlarm(timer, 1, false, 0);
}

void setupTimers(u8_t pin_zc, u8_t pin_output) {
    pinMode(pin_output, OUTPUT);
    digitalWrite(pin_output, LOW);

    pinMode(pin_zc, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(pin_zc), gpioISR, RISING);

    timer = timerBegin(1000000); // 1MHz
    timerAttachInterrupt(timer, timerISR);
    timerAlarm(timer, 1, false, 0);
}
