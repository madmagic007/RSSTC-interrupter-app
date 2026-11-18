#include "NumberCapability.h"

NumberCapability::NumberCapability(const String& capabilityName, uint32_t initialValue, uint32_t minimum, uint32_t maximum, ChangeCallback callback)
    : Capability(capabilityName, initialValue, callback), min(minimum), max(maximum) {}

bool NumberCapability::validate(uint32_t candidate) const {
    return min <= max && candidate >= min && candidate <= max;
}

uint32_t NumberCapability::normalizeValue(uint32_t candidate) const {
    return constrain(candidate, min, max);
}

CapabilityType NumberCapability::type() const {
    return CapabilityType::Number;
}

void NumberCapability::writeParams(JsonDocument& document) const {
    Capability::writeParams(document);
    document["min"] = min;
    document["max"] = max;
}
