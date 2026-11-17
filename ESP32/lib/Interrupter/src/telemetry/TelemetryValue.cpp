#include "TelemetryValue.h"

#include <ArduinoJson.h>
#include <utility>

TelemetryValue::TelemetryValue(const String& telemetryName, uint32_t initialValue,
                               CapabilityType valueType,
                               std::vector<String> telemetryOptions)
    : name(telemetryName), value(initialValue), type(valueType),
      options(std::move(telemetryOptions)) {}

bool TelemetryValue::setValue(uint32_t candidate) {
    value = candidate;
    return true;
}

bool TelemetryValue::setValue(bool candidate) {
    value = candidate ? 1u : 0u;
    return true;
}

const char* TelemetryValue::typeName() const {
    switch (type) {
        case CapabilityType::Toggle:
            return "toggle";
        case CapabilityType::Button:
            return "button";
        case CapabilityType::Enum:
            return "enum";
        case CapabilityType::Number:
        default:
            return "number";
    }
}

String TelemetryValue::toJson() const {
    JsonDocument document;
    document["name"] = name;
    document["type"] = typeName();
    document["value"] = value;
    if (!options.empty()) {
        JsonArray optionValues = document.createNestedArray("options");
        for (const String& option : options) {
            optionValues.add(option);
        }
    }
    String output;
    serializeJson(document, output);
    return output;
}
