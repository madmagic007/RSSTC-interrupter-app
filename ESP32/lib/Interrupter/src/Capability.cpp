#include "Capability.h"

#include <ArduinoJson.h>

Capability::Capability(const String& capabilityName, uint32_t initialValue,
                       uint32_t minimum, uint32_t maximum,
                       ChangeCallback callback, void* context)
    : name(capabilityName), value(initialValue), min(minimum), max(maximum),
      changeCallback(callback), changeContext(context) {}

bool Capability::validate(uint32_t candidate) const {
    return candidate >= min && candidate <= max;
}

bool Capability::setValue(uint32_t candidate) {
    return setValue(candidate, true);
}

bool Capability::setValue(uint32_t candidate, bool invokeCallback) {
    if (min > max) {
        return false;
    }

    candidate = constrain(candidate, min, max);
    if (!validate(candidate)) {
        return false;
    }

    const uint32_t previousValue = value;
    value = candidate;
    if (invokeCallback && value != previousValue) {
        invokeChangeCallback();
    }
    return true;
}

void Capability::invokeChangeCallback() const {
    if (memberChangeCallback) {
        memberChangeCallback(*this);
    } else if (changeCallback != nullptr) {
        changeCallback(*this, changeContext);
    }
}

bool CapabilityRegistry::registerCapability(Capability& capability) {
    if (capability.name.isEmpty() || capabilities.count(capability.name) != 0) {
        return false;
    }

    if (capability.min > capability.max || !capability.validate(capability.value)) {
        return false;
    }

    capabilities.emplace(capability.name, &capability);
    return true;
}

Capability* CapabilityRegistry::find(const String& name) {
    const auto it = capabilities.find(name);
    return it == capabilities.end() ? nullptr : it->second;
}

const Capability* CapabilityRegistry::find(const String& name) const {
    const auto it = capabilities.find(name);
    return it == capabilities.end() ? nullptr : it->second;
}

bool CapabilityRegistry::getValue(const String& name, uint32_t& value) const {
    const Capability* capability = find(name);
    if (capability == nullptr) {
        return false;
    }

    value = capability->value;
    return true;
}

bool CapabilityRegistry::setValue(const String& name, uint32_t value) {
    Capability* capability = find(name);
    if (capability == nullptr) {
        return false;
    }

    const uint32_t previousValue = capability->value;
    if (!capability->setValue(value, false)) {
        return false;
    }

    if (capability->value != previousValue) {
        if (globalChangeCallback != nullptr) {
            globalChangeCallback(*capability, globalChangeContext);
        } else {
            capability->invokeChangeCallback();
        }

        if (changeCallback != nullptr) {
            changeCallback(*capability, changeContext);
        }
    }
    return true;
}

void CapabilityRegistry::setGlobalChangeCallback(ChangeCallback callback,
                                                  void* context) {
    globalChangeCallback = callback;
    globalChangeContext = context;
}

void CapabilityRegistry::setChangeCallback(ChangeCallback callback, void* context) {
    changeCallback = callback;
    changeContext = context;
}

const CapabilityRegistry::Map& CapabilityRegistry::all() const {
    return capabilities;
}

String CapabilityRegistry::toJson() const {
    JsonDocument document;

    for (const auto& entry : capabilities) {
        const Capability& capability = *entry.second;
        JsonObject object = document[entry.first].to<JsonObject>();
        object["name"] = capability.name;
        object["value"] = capability.value;
        object["min"] = capability.min;
        object["max"] = capability.max;
    }

    String output;
    serializeJson(document, output);
    return output;
}

bool CapabilityRegistry::applyIncomingJson(const String& json) {
    JsonDocument document;
    const DeserializationError error = deserializeJson(document, json);
    if (error) {
        Serial.printf("Invalid capability JSON: %s\n", error.c_str());
        return false;
    }

    if (!document.is<JsonObject>()) {
        Serial.println("Invalid capability update: expected a JSON object");
        return false;
    }

    bool allUpdatesAccepted = true;
    for (JsonPair entry : document.as<JsonObject>()) {
        const String name = entry.key().c_str();
        if (!entry.value().is<uint32_t>()) {
            Serial.println("Invalid value for capability: " + name);
            allUpdatesAccepted = false;
            continue;
        }

        const uint32_t value = entry.value().as<uint32_t>();
        if (!setValue(name, value)) {
            Serial.println("Capability update rejected: " + name);
            allUpdatesAccepted = false;
        }
    }

    return allUpdatesAccepted;
}
