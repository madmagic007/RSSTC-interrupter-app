#include "NumberCapability.h"

NumberCapability::NumberCapability(const String& capabilityName,
                                   uint32_t initialValue, uint32_t minimum,
                                   uint32_t maximum, ChangeCallback callback,
                                   void* context)
    : Capability(capabilityName, initialValue, callback, context),
      min(minimum), max(maximum) {}

bool NumberCapability::validate(uint32_t candidate) const {
    return min <= max && candidate >= min && candidate <= max;
}

bool NumberCapability::hasRange() const {
    return true;
}

uint32_t NumberCapability::minValue() const {
    return min;
}

uint32_t NumberCapability::maxValue() const {
    return max;
}

CapabilityType NumberCapability::capabilityType() const {
    return CapabilityType::Number;
}

const char* NumberCapability::typeName() const {
    return "number";
}

uint32_t NumberCapability::normalizeValue(uint32_t candidate) const {
    if (min > max) {
        return candidate;
    }
    return constrain(candidate, min, max);
}
