#pragma once

#include "Telemetry.h"

class StringTelemetry : public Telemetry {
public:
    StringTelemetry(const String& name, const String& value = "");

    bool setValue(const String& candidate);
    TelemetryType type() const override;
    String toJson() const override;

    String value;
};
