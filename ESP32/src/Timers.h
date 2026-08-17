#pragma once
#include <Arduino.h>
#include <PotReader.h>
#include <soc/gpio_struct.h>

static hw_timer_t *timer = nullptr;
static volatile u8_t stage = 0;
static volatile u8_t zcCount = 0;

static constexpr float vMax = 3300.0;
static constexpr u32_t delayMax = 5000;
static constexpr u8_t zcSkipsMax = 100;

static void IRAM_ATTR timerISR() {
    switch (stage) {
        case 0: {
            u32_t delayUs = channelDelay.voltage * delayMax / vMax;

            stage = 1;
            timerWrite(timer, 0);
            timerAlarm(timer, delayUs, false, 0);

            break;
        }
        case 1: {
            u32_t pwUs = channelPw.voltage * delayMax / vMax;

            stage = 2;
            timerWrite(timer, 0);
            timerAlarm(timer, pwUs, false, 0);

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

    u8_t zcSkips = channelPulseSkips.voltage * zcSkipsMax / vMax;
    if (zcCount < zcSkips) return;

    zcCount = 0;
    stage = 0;
    timerWrite(timer, 1);
    timerAlarm(timer, 1, false, 0);
}


static void setupTimers() {
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);

    pinMode(16, INPUT_PULLDOWN); //zero cross
    attachInterrupt(digitalPinToInterrupt(16), gpioISR, RISING);

    timer = timerBegin(1000000); // 1mhz?
    timerAttachInterrupt(timer, timerISR);
    timerAlarm(timer, 1, false, 0);
}