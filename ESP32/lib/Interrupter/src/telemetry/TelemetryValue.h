#pragma once

#include "../capabilities/CapabilityType.h"

#include <Arduino.h>
#include <vector>

class TelemetryValue {
public:
    TelemetryValue(const String& name, uint32_t value = 0,
                   CapabilityType valueType = CapabilityType::Number,
                   std::vector<String> options = {});

    bool setValue(uint32_t candidate);
    bool setValue(bool candidate);
    const char* typeName() const;
    String toJson() const;

    String name;
    uint32_t value = 0;
    CapabilityType type = CapabilityType::Number;
    std::vector<String> options;
};
