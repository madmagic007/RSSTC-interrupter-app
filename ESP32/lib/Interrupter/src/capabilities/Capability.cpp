#include "Capability.h"

Capability::Capability(const String& capabilityName, uint32_t initialValue,
                       ChangeCallback callback, void* context)
    : name(capabilityName), value(initialValue), changeCallback(callback),
      changeContext(context) {}

uint32_t Capability::normalizeValue(uint32_t candidate) const {
    return candidate;
}

void Capability::setMemberChangeCallback(
    std::function<void(const Capability&)> callback) {
    memberChangeCallback = std::move(callback);
}

bool Capability::setValue(uint32_t candidate) {
    return setValue(candidate, true);
}

bool Capability::setValue(uint32_t candidate, bool invokeCallback) {
    candidate = normalizeValue(candidate);
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
    }

    if (changeCallback != nullptr) {
        changeCallback(*this, changeContext);
    }
}

void Capability::writeParams(JsonDocument& document) const {
    document["name"] = name;
    document["type"] = typeName();
    document["value"] = value;
}

String Capability::getDefinitionJson() const {
    JsonDocument document;
    writeParams(document);

    String json;
    serializeJson(document, json);

    return json;
}

String Capability::getValueJson() const {
    JsonDocument document;
    document["name"] = name;
    document["type"] = typeName();
    document["value"] = value;

    String json;
    serializeJson(document, json);

    return json;
}
