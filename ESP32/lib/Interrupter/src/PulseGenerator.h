#pragma once

#include <Arduino.h>
#include "driver/mcpwm_prelude.h"
#include "capabilities/NumberCapability.h"

class PulseGenerator {
public:
    static constexpr uint32_t MIN_BPS = 16;
    using PulseEventCallback = void (*)();

    static bool begin(uint8_t pin, uint32_t bps, uint32_t onTimeUs, uint32_t maxBps, uint32_t maxOnTimeUs);
    static bool begin(uint8_t pin, const NumberCapability& bpsCapability,
                      const NumberCapability& onTimeCapability);
    static bool setPulseEventCallbacks(PulseEventCallback onStart,
                                       PulseEventCallback onEnd);
    static bool enable();
    static bool disable();
    static bool setBps(uint32_t bps);
    static bool setOnTimeUs(uint32_t onTimeUs);
    static bool set(uint32_t bps, uint32_t onTimeUs);

private:
    static constexpr uint32_t TIMER_RESOLUTION = 1000000;
    static constexpr uint32_t MAX_TIMER_PERIOD_TICKS = 65535;

    static uint32_t periodToTicks(uint32_t bps);
    static uint32_t pulseWidthToTicks(uint32_t onTimeUs);

    static  inline int outputPin = -1;
    static inline uint32_t currentBps = 0;
    static inline uint32_t currentOnTimeUs = 0;
    static inline uint32_t maxBps = 0;
    static inline uint32_t maxOnTimeUs = 0;
    static inline bool initialized = false;
    static inline bool outputEnabled = false;

    static inline void* timerHandle = nullptr;
    static inline void* operatorHandle = nullptr;
    static inline void* comparatorHandle = nullptr;
    static inline void* generatorHandle = nullptr;
    static inline PulseEventCallback pulseStartCallback = nullptr;
    static inline PulseEventCallback pulseEndCallback = nullptr;

    static bool IRAM_ATTR onTimerEmpty(mcpwm_timer_handle_t timer,
                                       const mcpwm_timer_event_data_t* eventData,
                                       void* userData);
    static bool IRAM_ATTR onComparatorReached(
        mcpwm_cmpr_handle_t comparator,
        const mcpwm_compare_event_data_t* eventData, void* userData);
};