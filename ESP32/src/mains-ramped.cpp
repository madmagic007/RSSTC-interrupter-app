#include "Interrupter.h"
#include <soc/gpio_struct.h>

static constexpr uint8_t PIN_ZERO_CROSS = 7;
static constexpr uint8_t PIN_OUTPUT = 18;

static Interrupter interrupter;
static Capability delayUS("Delay after zero cross", 0, 0, 10000);
static Capability pulseWidthUS("Pulse width", 0, 0, 10000);
static Capability zeroCrossSkips("Zero cross skips", 100, 0, 100);

static uint32_t outputPinMask = 0;
static volatile uint8_t stage = 0;
static volatile uint8_t zeroCrossCount = 0;

static void IRAM_ATTR onTimer() {
    switch (stage) {
        case 0:
            stage = 1;
            interrupter.setTimerAlarm(delayUS.value + 350);
            break;
        case 1:
            if (pulseWidthUS.value == 0) {
                stage = 99;
                break;
            }
            stage = 2;
            interrupter.setTimerAlarm(pulseWidthUS.value);
            GPIO.out_w1ts.val = outputPinMask;
            break;
        case 2:
            GPIO.out_w1tc.val = outputPinMask;
            stage = 99;
            break;
    }
}

static void IRAM_ATTR onZeroCross() {
    zeroCrossCount = static_cast<uint8_t>(zeroCrossCount + 1);
    if (zeroCrossCount <= zeroCrossSkips.value || pulseWidthUS.value == 0) {
        return;
    }

    zeroCrossCount = 0;
    stage = 0;
    interrupter.resetTimer();
    interrupter.setTimerAlarm(1);
}

void setup() {
    Serial.begin(115200);

    if (!interrupter.registerCapability(delayUS) ||
        !interrupter.registerCapability(pulseWidthUS) ||
        !interrupter.registerCapability(zeroCrossSkips)) {
        Serial.println("Failed to register interrupter capabilities");
        return;
    }

    outputPinMask = 1UL << PIN_OUTPUT;
    pinMode(PIN_OUTPUT, OUTPUT);
    digitalWrite(PIN_OUTPUT, LOW);
    pinMode(PIN_ZERO_CROSS, INPUT_PULLDOWN);

    if (!interrupter.registerTimer(1000000, onTimer)) {
        Serial.println("Failed to register hardware timer");
        return;
    }
    if (!interrupter.registerGpioInterrupt(PIN_ZERO_CROSS, RISING, onZeroCross)) {
        Serial.println("Failed to register zero-cross GPIO interrupt");
        return;
    }

    interrupter.begin("Mains Ramped Interrupter");
}

void loop() {
    delay(1000);
    interrupter.sendHeartbeat();
}
