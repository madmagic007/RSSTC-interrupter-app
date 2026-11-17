#include "CapabilityRegistry.h"

#include <ArduinoJson.h>

bool CapabilityRegistry::registerCapability(Capability& capability) {
    if (capability.name.isEmpty() || capabilities.count(capability.name) != 0) {
        return false;
    }
    if (capability.hasRange() && capability.minValue() > capability.maxValue()) {
        return false;
    }
    if (!capability.validate(capability.value)) {
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
    if (capability == nullptr || capability->readOnly) {
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

bool CapabilityRegistry::updateValue(const String& name, uint32_t value) {
    Capability* capability = find(name);
    if (capability == nullptr) {
        return false;
    }
    capability->value = value;
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
        JsonObject object = document.createNestedObject(entry.first);
        object["name"] = capability.name;
        object["type"] = capability.typeName();
        object["value"] = capability.value;
        if (capability.hasRange()) {
            object["min"] = capability.minValue();
            object["max"] = capability.maxValue();
        }
        object["readOnly"] = capability.readOnly;
        const std::vector<String>* enumOptions = capability.enumOptions();
        if (enumOptions != nullptr) {
            JsonArray optionsArray = object.createNestedArray("options");
            for (const String& option : *enumOptions) {
                optionsArray.add(option);
            }
        }
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
        uint32_t value = 0;
        if (entry.value().is<bool>()) {
            value = entry.value().as<bool>() ? 1u : 0u;
        } else if (entry.value().is<uint32_t>()) {
            value = entry.value().as<uint32_t>();
        } else if (entry.value().is<int>()) {
            value = static_cast<uint32_t>(entry.value().as<int>());
        } else {
            Serial.println("Invalid value for capability: " + name);
            allUpdatesAccepted = false;
            continue;
        }
        if (!setValue(name, value)) {
            Serial.println("Capability update rejected: " + name);
            allUpdatesAccepted = false;
        }
    }
    return allUpdatesAccepted;
}
