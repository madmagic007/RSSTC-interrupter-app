#pragma once
#include <Arduino.h>
#include <Interrupter.h>
#include "driver/mcpwm_prelude.h"
#include <soc/gpio_struct.h>
#include "freertos/FreeRTOS.h"

typedef struct {
    u32_t usSinceLast;
    u8_t channel;
    u16_t freq;

} ToneDataEntry;

class TonePlayer {
    public:
        TonePlayer(u8_t outputPin);
        static void startPlayingSequence(ToneDataEntry* sequence, u32_t len);
        static void handleToneDataEntry(ToneDataEntry toneData);

        static inline volatile uint32_t gpioCallbackCount = 0;

    private:
        static inline u32_t outputPinMask = -1;

        static inline ToneDataEntry* sequence = nullptr;
        static inline u32_t sequenceLen = 0;
        static inline u32_t sequencePos = 0;

        static inline ToneDataEntry nextEntry = {};
        
        static inline hw_timer_t* sequenceTimer = nullptr;
        static inline hw_timer_t* gpioTimer = nullptr;

        static void IRAM_ATTR handleSequence();

        static void IRAM_ATTR setChannelFrequency(u8_t channel, uint32_t freq);

        static inline mcpwm_timer_handle_t mcpwmTimers[3] = {};

        static bool IRAM_ATTR onMcpwmTimer(
            mcpwm_timer_handle_t timer,
            const mcpwm_timer_event_data_t* eventData,
            void* userData
        );

        static inline volatile bool channelEnabled[3] = {};
        static inline volatile uint32_t pendingPulses = 0;
        static void IRAM_ATTR handleGpio();
        static inline volatile bool gpioTimerRunning = false;
        static inline bool gpioState = false;
        static inline portMUX_TYPE pulseMux = portMUX_INITIALIZER_UNLOCKED;

        static inline u16_t onTimeUS = 200;
        static inline u8_t minOffTimeUS = 50;
};
