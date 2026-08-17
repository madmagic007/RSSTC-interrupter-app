#pragma once
#include <Arduino.h>

struct VoltageChannel {
    u8_t pin;
    volatile float voltage;
};

static void voltageTask(void *parameter) {
    VoltageChannel *channel = (VoltageChannel *)parameter;

    constexpr uint32_t sampleCount = 10;

    float samples[sampleCount];
    float sum = 0;
    u8_t index = 0;

    float firstReading = analogReadMilliVolts(channel->pin);

    for (uint32_t i = 0; i < sampleCount; i++) {
        samples[i] = firstReading;
        sum += firstReading;
    }

    channel->voltage = sum / sampleCount;

    while (true) {
        float reading = analogReadMilliVolts(channel->pin);

        sum -= samples[index];
        samples[index] = reading;
        sum += reading;

        index++;
        if (index >= sampleCount)
            index = 0;

        float average = sum / sampleCount;
        average = (average / 100UL) * 100UL;

        channel->voltage = average;

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static VoltageChannel channelDelay = {
    .pin = 4,
    .voltage = 0.0
};

VoltageChannel channelPw = {
    .pin = 0,
    .voltage = 0.0
};

VoltageChannel channelPulseSkips = {
    .pin = 6,
    .voltage = 0.0
};

static void startTasks() {
    xTaskCreate(voltageTask, "Voltage1", 2048, &channelDelay, 1, NULL);
    xTaskCreate(voltageTask, "Voltage2", 2048, &channelPw, 1, NULL);
    xTaskCreate(voltageTask, "Voltage3", 2048, &channelPulseSkips, 1, NULL);
}