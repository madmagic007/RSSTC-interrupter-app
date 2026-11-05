#pragma once

#include <Arduino.h>

class CapabilityRegistry;

void initBLE(const String& name, CapabilityRegistry& capabilities);
void reportValue(const String& name, uint32_t value);
void sendBLEHeartbeat();
