#include "TelemetryValue.h"

#include <ArduinoJson.h>
#include <utility>

TelemetryValue::TelemetryValue(const String& telemetryName, uint32_t initialValue): name(telemetryName), value(initialValue) {}

bool TelemetryValue::setValue(uint32_t val) {
    value = val;
    return true;
}

void TelemetryValue::increment() {
    value = value + 1;
}

String TelemetryValue::toJson() const {
    JsonDocument document;
    JsonObject object = document.to<JsonObject>();

    object["name"] = name;
    const uint32_t currentValue = value;
    object["value"] = currentValue;

    String output;
    serializeJson(document, output);

    return output;
}
