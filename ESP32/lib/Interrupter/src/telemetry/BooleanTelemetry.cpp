#include "BooleanTelemetry.h"

BooleanTelemetry::BooleanTelemetry(const String& telemetryName, bool initialValue)
    : Telemetry(telemetryName), value(initialValue) {}

bool BooleanTelemetry::setValue(bool candidate) {
    value = candidate;
    return true;
}

TelemetryType BooleanTelemetry::type() const {
    return TelemetryType::Boolean;
}

String BooleanTelemetry::toJson() const {
    const bool currentValue = value;
    return toJsonValue(currentValue);
}
