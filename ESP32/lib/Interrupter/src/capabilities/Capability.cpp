#include "Capability.h"

Capability::Capability(const String& capabilityName, uint32_t initialValue,
                       ChangeCallback callback, void* context)
    : name(capabilityName), value(initialValue), changeCallback(callback),
      changeContext(context) {}

bool Capability::hasRange() const {
    return false;
}

uint32_t Capability::minValue() const {
    return 0;
}

uint32_t Capability::maxValue() const {
    return 0;
}

const std::vector<String>* Capability::enumOptions() const {
    return nullptr;
}

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
    } else if (changeCallback != nullptr) {
        changeCallback(*this, changeContext);
    }
}
