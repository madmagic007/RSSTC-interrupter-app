#include "ToggleCapability.h"

ToggleCapability::ToggleCapability(const String& name, bool initialValue,
                                   ChangeCallback callback, void* context)
    : Capability(name, initialValue ? 1u : 0u, callback, context) {}

bool ToggleCapability::validate(uint32_t candidate) const {
    return candidate == 0 || candidate == 1;
}

CapabilityType ToggleCapability::capabilityType() const {
    return CapabilityType::Toggle;
}

const char* ToggleCapability::typeName() const {
    return "toggle";
}

uint32_t ToggleCapability::normalizeValue(uint32_t candidate) const {
    return candidate ? 1u : 0u;
}
