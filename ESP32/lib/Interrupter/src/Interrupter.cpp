#include "Interrupter.h"

#include "BLEEngine.h"
#include "capabilities/CapabilityRegistry.h"

bool Interrupter::registerGpioInterrupt(uint8_t pin, int mode, InterruptCallback callback) {
    if (callback == nullptr || digitalPinToInterrupt(pin) == NOT_AN_INTERRUPT) {
        return false;
    }

    attachInterrupt(digitalPinToInterrupt(pin), callback, mode);
    return true;
}

bool Interrupter::registerTimer(uint32_t frequencyHz, InterruptCallback callback) {
    if (timer != nullptr || frequencyHz == 0 || callback == nullptr) {
        return false;
    }

    timer = timerBegin(frequencyHz);
    if (timer == nullptr) {
        return false;
    }

    timerAttachInterrupt(timer, callback);
    timerUsToTicksFactor = frequencyHz / 1000000;
    return true;
}

bool Interrupter::setTimerAlarm(uint64_t us, bool autoreload, uint64_t reloadCount) {
    if (timer == nullptr || us == 0) {
        return false;
    }

    resetTimer();
    timerAlarm(timer, us * timerUsToTicksFactor, autoreload, reloadCount);
    return true;
}

void Interrupter::resetTimer() {
    if (timer != nullptr) {
        timerWrite(timer, 0);
    }
}
