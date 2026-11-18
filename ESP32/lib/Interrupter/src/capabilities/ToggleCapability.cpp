#include "ToggleCapability.h"

ToggleCapability::ToggleCapability(const String& name, bool initialValue,
                                   ChangeCallback callback, void* context)
    : Capability(name, initialValue ? 1u : 0u, callback, context) {}

bool ToggleCapability::validate(uint32_t candidate) const {
    return candidate == 0 || candidate == 1;
}

uint32_t ToggleCapability::normalizeValue(uint32_t candidate) const {
    return candidate ? 1u : 0u;
}

CapabilityType ToggleCapability::type() const {
    return CapabilityType::Toggle;
}
