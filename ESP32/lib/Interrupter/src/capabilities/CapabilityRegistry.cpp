#include "CapabilityRegistry.h"
#include "BLEEngine.h"

bool CapabilityRegistry::registerCapability(Capability& capability) {
    if (capability.name.isEmpty() || capabilities.count(capability.name) != 0 ||
        !capability.validate(capability.value)) {
        return false;
    }

    capabilities.emplace(capability.name, &capability);
    return true;
}

Capability* CapabilityRegistry::find(const String& name) {
    const auto it = capabilities.find(name);
    return it == capabilities.end() ? nullptr : it->second;
}

bool CapabilityRegistry::setValue(Capability& capability, uint32_t value) {
    return isRegistered(capability) && applyValue(capability, value, false, true);
}

bool CapabilityRegistry::isRegistered(const Capability& capability) {
    for (const auto& entry : capabilities) {
        if (entry.second == &capability) {
            return true;
        }
    }
    return false;
}

bool CapabilityRegistry::applyValue(Capability& capability, uint32_t value,
                                    bool runBeforeChange,
                                    bool reportToRemote) {
    const uint32_t requestedValue = value;
    value = capability.normalizeValue(value);
    if (!capability.validate(value)) {
        return false;
    }

    if (runBeforeChange && beforeChangeCallback != nullptr) {
        const bool accepted = beforeChangeCallback(capability, value);
        if (!accepted) {
            return false;
        }

        value = capability.normalizeValue(value);
        if (!capability.validate(value)) {
            return false;
        }
    }

    const uint32_t previousValue = capability.value;
    if (!capability.setValue(value, false)) {
        return false;
    }

    const bool changed = capability.value != previousValue;
    const bool shouldReport = (reportToRemote && changed) ||
                              (!reportToRemote && value != requestedValue);
    if (changed) {
        capability.invokeChangeCallback();
    }

    if (changed && changeCallback != nullptr) {
        changeCallback(capability, changeContext);
    }
    if (shouldReport) {
        BLEEngine::reportCapability(capability);
    }
    return true;
}

void CapabilityRegistry::setBeforeChangeCallback(
    BeforeChangeCallback callback) {
    beforeChangeCallback = callback;
}

void CapabilityRegistry::setChangeCallback(ChangeCallback callback, void* context) {
    changeCallback = callback;
    changeContext = context;
}

const CapabilityRegistry::Map& CapabilityRegistry::all() {
    return capabilities;
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
        const JsonVariantConst incomingValue = entry.value();
        uint32_t value;

        if (incomingValue.is<bool>()) {
            value = incomingValue.as<bool>() ? 1u : 0u;
        } else if (incomingValue.is<uint32_t>()) {
            value = incomingValue.as<uint32_t>();
        } else {
            Serial.println("Capability update rejected: invalid value for " + name);
            allUpdatesAccepted = false;
            continue;
        }

        Capability* capability = find(name);
        if (capability == nullptr || !applyValue(*capability, value, true, false)) {
            Serial.println("Capability update rejected: " + name);
            allUpdatesAccepted = false;
        }
    }

    return allUpdatesAccepted;
}
