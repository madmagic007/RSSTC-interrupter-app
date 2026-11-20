#pragma once

#include <Arduino.h>

enum class TelemetryType : uint8_t {
    Number = 0,
    Boolean = 1,
    String = 2,
};
