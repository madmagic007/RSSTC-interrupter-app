#pragma once

#include "Telemetry.h"

class BooleanTelemetry : public Telemetry {
public:
    BooleanTelemetry(const String& name, bool value = false);

    bool setValue(bool candidate);
    TelemetryType type() const override;
    String toJson() const override;

    volatile bool value = false;
};
