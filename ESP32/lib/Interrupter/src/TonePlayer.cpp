#include "TonePlayer.h"

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
        config.flags.update_period_on_empty = true;
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

bool TonePlayer::startPlayingSequence(const ToneDataEntry* sequence, u32_t len) {
    if (sequence == nullptr || len == 0) return false;

    stopPlayingSequence();

    portENTER_CRITICAL(&sequenceMux);
    TonePlayer::sequence = sequence;
    sequenceLen = len;
    sequencePos = 0;
    nextEntry = sequence[0];
    sequenceDelayUS = 1;
    sequencePaused = false;
    sequenceActive = true;

    timerWrite(sequenceTimer, 0);
    timerAlarm(sequenceTimer, sequenceDelayUS, false, 0);
    portEXIT_CRITICAL(&sequenceMux);
    return true;
}

void TonePlayer::pausePlayingSequence() {
    portENTER_CRITICAL(&sequenceMux);
    if (sequenceActive && !sequencePaused) {
        const uint64_t elapsedUS = timerRead(sequenceTimer);
        sequenceDelayUS = elapsedUS < sequenceDelayUS
            ? sequenceDelayUS - static_cast<u32_t>(elapsedUS)
            : 1;
        sequencePaused = true;
        timerStop(sequenceTimer);
        stopOutput(false);
    }
    portEXIT_CRITICAL(&sequenceMux);
}

void TonePlayer::resumePlayingSequence() {
    portENTER_CRITICAL(&sequenceMux);
    if (sequenceActive && sequencePaused) {
        sequencePaused = false;
        timerWrite(sequenceTimer, 0);
        timerAlarm(sequenceTimer, sequenceDelayUS, false, 0);
    }
    portEXIT_CRITICAL(&sequenceMux);
}

void TonePlayer::stopPlayingSequence() {
    portENTER_CRITICAL(&sequenceMux);
    timerStop(sequenceTimer);
    sequence = nullptr;
    sequenceLen = 0;
    sequencePos = 0;
    nextEntry = {};
    sequenceDelayUS = 1;
    sequencePaused = false;
    sequenceActive = false;
    stopOutput(true);
    portEXIT_CRITICAL(&sequenceMux);
}

bool TonePlayer::isSequenceActive() {
    return sequenceActive;
}

bool TonePlayer::isSequencePaused() {
    return sequencePaused;
}

void TonePlayer::setOnTimeUs(uint32_t onTimeUs) {
    onTimeUS = static_cast<u16_t>(onTimeUs);
}

void TonePlayer::setDeadTimeUs(uint32_t deadTimeUs) {
    pulseSkipWindow = static_cast<u16_t>(deadTimeUs);
}

void IRAM_ATTR TonePlayer::handleSequence() {
    portENTER_CRITICAL_ISR(&sequenceMux);
    if (!sequenceActive || sequencePaused) {
        portEXIT_CRITICAL_ISR(&sequenceMux);
        return;
    }

    ToneDataEntry& curEntry = nextEntry;

    setChannelFrequency(curEntry.channel, curEntry.freq);

    sequencePos++;
    if (sequencePos >= sequenceLen) {
        timerStop(sequenceTimer);
        sequence = nullptr;
        sequenceLen = 0;
        sequencePos = 0;
        sequenceActive = false;
        sequencePaused = false;
        stopOutputFromISR(true);
        portEXIT_CRITICAL_ISR(&sequenceMux);
        return;
    }

    nextEntry = sequence[sequencePos];

    sequenceDelayUS = nextEntry.usSinceLast == 0 ? 1 : nextEntry.usSinceLast;
    timerWrite(sequenceTimer, 0);
    timerAlarm(sequenceTimer, sequenceDelayUS, false, 0);
    portEXIT_CRITICAL_ISR(&sequenceMux);
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
    if (channel >= 3) return false;

    if (!channelEnabled[channel]) return false;

    int64_t now = esp_timer_get_time();
    
    portENTER_CRITICAL_ISR(&pulseMux);
    if (sequenceActive && !sequencePaused && !gpioTimerRunning &&
        (now - lastPulseUS) >= pulseSkipWindow) {
        gpioTimerRunning = true;
        lastPulseUS = now;

        timerWrite(gpioTimer, 0);
        timerAlarm(gpioTimer, 1, false, 0);
        timerStart(gpioTimer);
    }

    portEXIT_CRITICAL_ISR(&pulseMux);

    return false;
}

void IRAM_ATTR TonePlayer::handleGpio() {
    portENTER_CRITICAL_ISR(&pulseMux);

    if (!sequenceActive || sequencePaused) {
        GPIO.out_w1tc.val = outputPinMask;
        gpioState = false;
        timerStop(gpioTimer);
        gpioTimerRunning = false;
        portEXIT_CRITICAL_ISR(&pulseMux);
        return;
    }

    if (!gpioState) {
        GPIO.out_w1ts.val = outputPinMask;
        gpioState = true;

        timerWrite(gpioTimer, 0);
        timerAlarm(gpioTimer, onTimeUS, false, 0);

        //pendingPulses = pendingPulses - 1;
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

void TonePlayer::stopOutput(bool disableChannels) {
    portENTER_CRITICAL(&pulseMux);
    resetOutputState(disableChannels);
    portEXIT_CRITICAL(&pulseMux);
}

void IRAM_ATTR TonePlayer::stopOutputFromISR(bool disableChannels) {
    portENTER_CRITICAL_ISR(&pulseMux);
    resetOutputState(disableChannels);
    portEXIT_CRITICAL_ISR(&pulseMux);
}

void IRAM_ATTR TonePlayer::resetOutputState(bool disableChannels) {
    timerStop(gpioTimer);
    GPIO.out_w1tc.val = outputPinMask;
    gpioState = false;
    gpioTimerRunning = false;
    pendingPulses = 0;
    lastPulseUS = 0;
    if (disableChannels) {
        for (uint8_t channel = 0; channel < 3; channel++) {
            channelEnabled[channel] = false;
        }
    }
}