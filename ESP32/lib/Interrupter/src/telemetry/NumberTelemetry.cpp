#include "NumberTelemetry.h"

NumberTelemetry::NumberTelemetry(const String& telemetryName, uint32_t initialValue)
    : Telemetry(telemetryName), value(initialValue) {}

bool NumberTelemetry::setValue(uint32_t candidate) {
    value = candidate;
    return true;
}

void NumberTelemetry::increment() {
    value = value + 1;
}

TelemetryType NumberTelemetry::type() const {
    return TelemetryType::Number;
}

String NumberTelemetry::toJson() const {
    const uint32_t currentValue = value;
    return toJsonValue(currentValue);
}
