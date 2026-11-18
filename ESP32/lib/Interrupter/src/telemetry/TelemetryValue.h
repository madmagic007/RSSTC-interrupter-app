#pragma once

#include <Arduino.h>

class TelemetryValue {
public:
    TelemetryValue(const String& name, uint32_t value = 0);

    bool setValue(uint32_t candidate);
    void IRAM_ATTR increment();
    String toJson() const;

    String name;
    volatile uint32_t value = 0;
};
