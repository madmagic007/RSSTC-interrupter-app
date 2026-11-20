#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "TelemetryType.h"

class Telemetry {
public:
    virtual ~Telemetry() = default;

    virtual TelemetryType type() const = 0;
    virtual String toJson() const = 0;

    String name;

protected:
    explicit Telemetry(const String& telemetryName) : name(telemetryName) {}

    template <typename T>
    String toJsonValue(const T& value) const {
        JsonDocument document;
        JsonObject object = document.to<JsonObject>();
        object["name"] = name;
        object["type"] = static_cast<uint8_t>(type());
        object["value"] = value;

        String output;
        serializeJson(document, output);
        return output;
    }
};
