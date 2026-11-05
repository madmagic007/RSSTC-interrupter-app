#pragma once

#include <stdint.h>

class PulseGenerator {
public:
    static constexpr uint32_t MIN_BPS = 16;

    bool begin(int pin, uint32_t bps, uint32_t onTimeUs, uint32_t maxBps,
               uint32_t maxOnTimeUs);
    bool enable();
    bool disable();
    bool setBps(uint32_t bps);
    bool setOnTimeUs(uint32_t onTimeUs);
    bool set(uint32_t bps, uint32_t onTimeUs);

private:
    static constexpr uint32_t TIMER_RESOLUTION = 1000000;
    static constexpr uint32_t MAX_TIMER_PERIOD_TICKS = 65535;

    uint32_t periodToTicks(uint32_t bps) const;
    uint32_t pulseWidthToTicks(uint32_t onTimeUs) const;

    int outputPin = -1;
    uint32_t currentBps = 0;
    uint32_t currentOnTimeUs = 0;
    uint32_t maxBps = 0;
    uint32_t maxOnTimeUs = 0;
    bool initialized = false;
    bool outputEnabled = false;

    void* timerHandle = nullptr;
    void* operatorHandle = nullptr;
    void* comparatorHandle = nullptr;
    void* generatorHandle = nullptr;
};