#pragma once

#include <Arduino.h>

#include <map>

#include "capabilities/CapabilityType.h"
#include "capabilities/CapabilityRegistry.h"
#include "telemetry/TelemetryValue.h"

class CapabilityRegistry;

void initBLE(const String& name, CapabilityRegistry& capabilities,
             std::map<String, TelemetryValue*>* telemetry = nullptr);
void reportValue(const String& name, uint32_t value);
void reportValue(const String& name, uint32_t value, CapabilityType type,
                 bool readOnly = false);
void reportTelemetryDefinitions();
void reportTelemetryValue(const String& name, const TelemetryValue& value);
void sendBLEHeartbeat();
