#include "Capability.h"
#include <BLEEngine.h>
#include <ArduinoJson.h>

Capability delayUS = {
    .name = "Delay after zero cross",
    .value = 0,
    .max  = 10000
};

Capability pwUS = {
    .name = "Pulse width",
    .max  = 10000
};

Capability zcSkipCount = {
    .name  = "Zero cross skips",
    .value = 100,
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
    uint32_t projectedSkip  = (cap == &zcSkipCount) ? requested : zcSkipCount.value;
 
    // Too high pulse duration combined with too frequent BPS (low zero cross skips) caused alot of IGBT failures, some checks in place to prevent this
    bool conflict = false;
    if (projectedDelay + projectedPW > 10000 && projectedSkip <= 2) { // prevents "permanent on situation"
        projectedSkip = 3;
        conflict = true;
    }

    if (projectedPW > 300 && projectedSkip < 20) { // limit to 300us at 5Hz
        projectedSkip = 20;
        projectedPW = 300;
        conflict = true;
    }
 
    if (projectedSkip != zcSkipCount.value) {
        zcSkipCount.value = projectedSkip;
    }

    if (projectedPW != pwUS.value) {
        pwUS.value = projectedPW;
    }
 
    if (projectedDelay != delayUS.value) {
        delayUS.value = projectedDelay;
    }
 
    if (conflict) {
        reportValue(delayUS.name, delayUS.value);
        delay(1);
        reportValue(pwUS.name, pwUS.value);
        delay(1);
        reportValue(zcSkipCount.name, zcSkipCount.value);
    }
 
    return true;
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
