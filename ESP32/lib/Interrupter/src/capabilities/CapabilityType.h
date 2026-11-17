#pragma once

#include <Arduino.h>

enum class CapabilityType : uint8_t {
    Number = 0,
    Toggle = 1,
    Button = 2,
    Enum = 3,
};
