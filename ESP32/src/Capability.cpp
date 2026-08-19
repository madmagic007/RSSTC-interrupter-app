#include "Capability.h"
#include <BLEEngine.h>
#include <ArduinoJson.h>

Capability delayUS = {
    .name = "Delay after zero cross",
    .max  = 10000
};

Capability pwUS = {
    .name = "Pulse width",
    .max  = 20000
};

Capability zcSkipCount = {
    .name  = "Zero cross skips",
    .value = 50,
    .max   = 100
};

std::map<String, Capability*> capabilityRegistry = {
    { delayUS.name,     &delayUS },
    { pwUS.name,        &pwUS },
    { zcSkipCount.name, &zcSkipCount }
};

bool setCapabilityValue(const String& name, uint32_t value) {
    auto it = capabilityRegistry.find(name);
    if (it == capabilityRegistry.end()) return false;
 
    Capability* cap = it->second;
    uint32_t requested = constrain(value, cap->min, cap->max);
 
    uint32_t projectedDelay = (cap == &delayUS)     ? requested : delayUS.value;
    uint32_t projectedPW    = (cap == &pwUS)        ? requested : pwUS.value;
    uint32_t safeSkip       = (cap == &zcSkipCount) ? requested : zcSkipCount.value;
 

    bool conflict = false;
    if (projectedDelay + projectedPW > 10000 && safeSkip < 2) {
        safeSkip = 3;
        conflict = true;
    }
 
    if (safeSkip != zcSkipCount.value) {
        zcSkipCount.value = safeSkip;
    }
 
    if (cap != &zcSkipCount) {
        cap->value = requested;
    }
 
    if (conflict) {
        reportValue(zcSkipCount.name, zcSkipCount.value);
    }

    Serial.printf("%d, %d\n", delayUS.value + pwUS.value, zcSkipCount.value);
 
    return true;
}


Capability* getCapability(const String& name) {
    auto it = capabilityRegistry.find(name);
    return (it != capabilityRegistry.end()) ? it->second : nullptr;
}

void applyIncomingJson(const String& json) {
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) return;

    for (JsonPair kv : doc.as<JsonObject>()) {
        String key = kv.key().c_str();
        uint32_t value = kv.value().as<uint32_t>();

        if (!setCapabilityValue(key, value)) {
            Serial.println("Unknown capability received: " + key + ", " + value);
        }
    }
}

String getCapabilities() {
    StaticJsonDocument<1024> doc;

    for (auto& pair : capabilityRegistry) {
        JsonObject obj = doc.createNestedObject(pair.first);
        Capability* cap = pair.second;

        obj["name"]  = cap->name;
        obj["value"] = cap->value;
        obj["min"]   = cap->min;
        obj["max"]   = cap->max;
    }

    String output;
    serializeJson(doc, output);
    return output;
}
