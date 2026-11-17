#include "BLEEngine.h"
#include "Interrupter.h"
#include "capabilities/CapabilityRegistry.h"
#include "capabilities/NumberCapability.h"
#include <Arduino.h>
#include <soc/gpio_struct.h>

static constexpr uint8_t PIN_ZERO_CROSS = 7;
static constexpr uint8_t PIN_OUTPUT = 18;

static NumberCapability delayUS("Delay after zero cross", 0, 0, 10000);
static NumberCapability pulseWidthUS("Pulse width", 0, 0, 10000);
static NumberCapability zeroCrossSkips("Zero cross skips", 100, 0, 100);

static uint32_t outputPinMask = 0;
static volatile uint8_t stage = 0;
static volatile uint8_t zeroCrossCount = 0;

static bool enforceCapabilitySafety(Capability& incoming, uint32_t& candidate) {
    uint32_t projectedDelay = delayUS.value;
    uint32_t projectedPulseWidth = pulseWidthUS.value;
    uint32_t projectedSkips = zeroCrossSkips.value;

    if (&incoming == &delayUS) {
        projectedDelay = candidate;
    } else if (&incoming == &pulseWidthUS) {
        projectedPulseWidth = candidate;
    } else if (&incoming == &zeroCrossSkips) {
        projectedSkips = candidate;
    }

    if (projectedDelay + projectedPulseWidth > 10000 &&
        projectedSkips <= 2) {
        projectedSkips = 3;
    }

    if (projectedPulseWidth > 300 && projectedSkips < 20) {
        projectedSkips = 20;
        projectedPulseWidth = 300;
    }

    if (!delayUS.validate(projectedDelay) ||
        !pulseWidthUS.validate(projectedPulseWidth) ||
        !zeroCrossSkips.validate(projectedSkips)) {
        Serial.println("Safety check rejected update: adjusted value invalid");
        return false;
    }

    if (&zeroCrossSkips != &incoming &&
        zeroCrossSkips.value != projectedSkips &&
        !CapabilityRegistry::setValue(zeroCrossSkips, projectedSkips)) {
        Serial.println("Safety check failed to adjust zero-cross skips");
        return false;
    }
    if (&pulseWidthUS != &incoming &&
        pulseWidthUS.value != projectedPulseWidth &&
        !CapabilityRegistry::setValue(pulseWidthUS, projectedPulseWidth)) {
        Serial.println("Safety check failed to adjust pulse width");
        return false;
    }
    if (&delayUS != &incoming &&
        delayUS.value != projectedDelay &&
        !CapabilityRegistry::setValue(delayUS, projectedDelay)) {
        Serial.println("Safety check failed to adjust delay");
        return false;
    }

    if (&incoming == &delayUS) {
        candidate = projectedDelay;
    } else if (&incoming == &pulseWidthUS) {
        candidate = projectedPulseWidth;
    } else if (&incoming == &zeroCrossSkips) {
        candidate = projectedSkips;
    }

    return true;
}

static void IRAM_ATTR onTimer() {
    switch (stage) {
        case 0:
            stage = 1;
            Interrupter::setTimerAlarm(delayUS.value + 350);
            break;
        case 1:
            if (pulseWidthUS.value == 0) {
                stage = 99;
                break;
            }
            stage = 2;
            Interrupter::setTimerAlarm(pulseWidthUS.value);
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
    Interrupter::resetTimer();
    Interrupter::setTimerAlarm(1);
}

void setup() {
    Serial.begin(115200);

    if (!CapabilityRegistry::registerCapability(delayUS) ||
        !CapabilityRegistry::registerCapability(pulseWidthUS) ||
        !CapabilityRegistry::registerCapability(zeroCrossSkips)) {
        Serial.println("Failed to register interrupter capabilities");
        return;
    }
    CapabilityRegistry::setBeforeChangeCallback(enforceCapabilitySafety);

    outputPinMask = 1UL << PIN_OUTPUT;
    pinMode(PIN_OUTPUT, OUTPUT);
    digitalWrite(PIN_OUTPUT, LOW);
    pinMode(PIN_ZERO_CROSS, INPUT_PULLDOWN);

    if (!Interrupter::registerTimer(1000000, onTimer)) {
        Serial.println("Failed to register hardware timer");
        return;
    }
    if (!Interrupter::registerGpioInterrupt(PIN_ZERO_CROSS, RISING, onZeroCross)) {
        Serial.println("Failed to register zero-cross GPIO interrupt");
        return;
    }

    BLEEngine::initBLE("Mains Ramped Interrupter");
}

void loop() {
    delay(1000);
    BLEEngine::sendBLEHeartbeat();
}
