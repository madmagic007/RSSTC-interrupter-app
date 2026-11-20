#pragma once

#include "Telemetry.h"

class NumberTelemetry : public Telemetry {
public:
    NumberTelemetry(const String& name, uint32_t value = 0);

    bool setValue(uint32_t candidate);
    void IRAM_ATTR increment();
    TelemetryType type() const override;
    String toJson() const override;

    volatile uint32_t value = 0;
};
