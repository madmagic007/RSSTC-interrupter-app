#include "EnumCapability.h"

EnumCapability::EnumCapability(const String& name, uint32_t initialValue,
                               const std::vector<String>& enumOptions,
                               ChangeCallback callback, void* context)
    : Capability(name, initialValue, callback, context), values(enumOptions) {}

bool EnumCapability::validate(uint32_t candidate) const {
    return candidate < values.size();
}

CapabilityType EnumCapability::capabilityType() const {
    return CapabilityType::Enum;
}

const char* EnumCapability::typeName() const {
    return "enum";
}

const std::vector<String>* EnumCapability::enumOptions() const {
    return &values;
}
