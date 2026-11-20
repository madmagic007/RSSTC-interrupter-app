#include "StringTelemetry.h"

StringTelemetry::StringTelemetry(const String& telemetryName, const String& initialValue)
    : Telemetry(telemetryName), value(initialValue) {}

bool StringTelemetry::setValue(const String& candidate) {
    value = candidate;
    return true;
}

TelemetryType StringTelemetry::type() const {
    return TelemetryType::String;
}

String StringTelemetry::toJson() const {
    return toJsonValue(value);
}
