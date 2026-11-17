#pragma once

#include <esp32-hal-timer.h>

using InterruptCallback = void (*)();

class Interrupter {
public:
    static bool registerGpioInterrupt(uint8_t pin, int mode, InterruptCallback callback);
    static bool registerTimer(uint32_t frequencyHz, InterruptCallback callback);
    static bool IRAM_ATTR setTimerAlarm(uint64_t us, bool autoreload = false, uint64_t reloadCount = 0);
    static void IRAM_ATTR resetTimer();

private:
    static inline hw_timer_t* timer = nullptr;
    static inline int timerUsToTicksFactor = 0;
};
