#pragma once

#include <Arduino.h>
#include <map>

struct Capability {
    String name;
    u32_t  value = 0;
    u8_t   min   = 0;
    u32_t  max   = UINT32_MAX;
};

extern Capability delayUS;
extern Capability pwUS;
extern Capability zcSkipCount;

extern std::map<String, Capability*> capabilityRegistry;

typedef void (*CapabilityChangeCallback)(const String& name, uint32_t value);
void setCapabilityChangeCallback(CapabilityChangeCallback cb);

bool        setCapabilityValue(const String& name, uint32_t value);
Capability* getCapability(const String& name);

String getCapabilities(); 
void   applyIncomingJson(const String& json);
