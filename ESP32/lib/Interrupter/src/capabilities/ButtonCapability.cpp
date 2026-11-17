#include "ButtonCapability.h"

ButtonCapability::ButtonCapability(const String& name, ChangeCallback callback,
                                   void* context)
    : Capability(name, 0u, callback, context) {}

bool ButtonCapability::validate(uint32_t candidate) const {
    return candidate == 0 || candidate == 1;
}

CapabilityType ButtonCapability::capabilityType() const {
    return CapabilityType::Button;
}

const char* ButtonCapability::typeName() const {
    return "button";
}

uint32_t ButtonCapability::normalizeValue(uint32_t candidate) const {
    return candidate ? 1u : 0u;
}
