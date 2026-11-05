#include <TonePlayer.h>

TonePlayer::TonePlayer(u8_t outputPin) {
    outputPinMask = 1UL << outputPin;

    sequenceTimer = timerBegin(1000000);
    timerAttachInterrupt(sequenceTimer, &TonePlayer::handleSequence);

    gpioTimer = timerBegin(1000000);
    timerAttachInterrupt(gpioTimer, &TonePlayer::handleGpio);
    timerStop(gpioTimer);

    for (uint8_t i = 0; i < 3; i++) {
        mcpwm_timer_config_t config = {};

        config.group_id = 0;
        config.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT;
        config.resolution_hz = 1000000;
        config.count_mode = MCPWM_TIMER_COUNT_MODE_UP;
        config.period_ticks = 1000;

        ESP_ERROR_CHECK(mcpwm_new_timer(&config, &mcpwmTimers[i]));

        mcpwm_timer_event_callbacks_t callbacks = {};
        callbacks.on_full = onMcpwmTimer;

        ESP_ERROR_CHECK(
            mcpwm_timer_register_event_callbacks(
                mcpwmTimers[i],
                &callbacks,
                reinterpret_cast<void*>(static_cast<uintptr_t>(i))
            )
        );

        ESP_ERROR_CHECK(mcpwm_timer_enable(mcpwmTimers[i]));

        ESP_ERROR_CHECK(mcpwm_timer_start_stop(
            mcpwmTimers[i],
            MCPWM_TIMER_START_NO_STOP
        ));
    }
}

void TonePlayer::startPlayingSequence(ToneDataEntry* sequence, u32_t len) {
    TonePlayer::sequence = sequence;
    sequenceLen = len;
    sequencePos = 0;

    nextEntry = sequence[0];

    timerWrite(sequenceTimer, 0);
    timerAlarm(sequenceTimer, 1, false, 0);
}

void IRAM_ATTR TonePlayer::handleSequence() {
    ToneDataEntry& curEntry = nextEntry;

    setChannelFrequency(curEntry.channel, curEntry.freq);

    sequencePos++;
    if (sequencePos >= sequenceLen) {
        timerStop(sequenceTimer);
        return;
    }

    nextEntry = sequence[sequencePos];

    timerWrite(sequenceTimer, 0);
    timerAlarm(sequenceTimer, nextEntry.usSinceLast, false, 0);
}

void IRAM_ATTR TonePlayer::setChannelFrequency(u8_t channel, uint32_t freq) {
    if (channel >= 3) return;

    if (freq < 16) {
        channelEnabled[channel] = false;
        return;
    }

    uint32_t period = 1000000 / freq;

    if (mcpwm_timer_set_period(mcpwmTimers[channel], period) == ESP_OK) {
        channelEnabled[channel] = true;
    }
}

bool IRAM_ATTR TonePlayer::onMcpwmTimer(mcpwm_timer_handle_t timer, const mcpwm_timer_event_data_t* eventData, void* userData) {
    u32_t channel = (u32_t)(uintptr_t)userData;

    if (!channelEnabled[channel]) return false;
    
    portENTER_CRITICAL_ISR(&pulseMux);
    pendingPulses = pendingPulses + 1;

    if (!gpioTimerRunning) {
        gpioTimerRunning = true;
        timerWrite(gpioTimer, 0);
        timerAlarm(gpioTimer, 1, false, 0);
        timerStart(gpioTimer);
    }

    portEXIT_CRITICAL_ISR(&pulseMux);

    return false;
}

void IRAM_ATTR TonePlayer::handleGpio() {
    portENTER_CRITICAL_ISR(&pulseMux);
    gpioCallbackCount = gpioCallbackCount + 1;

    if (!gpioState) {
        GPIO.out_w1ts.val = outputPinMask;
        gpioState = true;

        timerWrite(gpioTimer, 0);
        timerAlarm(gpioTimer, onTimeUS, false, 0);

        pendingPulses = pendingPulses - 1;
    } else {
        GPIO.out_w1tc.val = outputPinMask;
        gpioState = false;

        if (pendingPulses > 0) {
            timerWrite(gpioTimer, 0);
            timerAlarm(gpioTimer, minOffTimeUS, false, 0);
        } else {
            timerStop(gpioTimer);
            gpioTimerRunning = false;
        }
    }

    portEXIT_CRITICAL_ISR(&pulseMux);
}