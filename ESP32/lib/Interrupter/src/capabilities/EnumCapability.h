#pragma once

#include "Capability.h"

class EnumCapability : public Capability {
public:
    EnumCapability(const String& name, uint32_t initialValue,
                   const std::vector<String>& options,
                   ChangeCallback callback = nullptr, void* context = nullptr);

    bool validate(uint32_t candidate) const override;
    CapabilityType capabilityType() const override;
    const char* typeName() const override;
    const std::vector<String>* enumOptions() const override;

private:
    std::vector<String> values;
};
