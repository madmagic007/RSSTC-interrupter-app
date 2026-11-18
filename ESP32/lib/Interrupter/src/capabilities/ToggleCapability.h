#pragma once

#include "Capability.h"

class ToggleCapability : public Capability {
public:
    ToggleCapability(const String& name, bool initialValue = false, ChangeCallback callback = nullptr, void* context = nullptr);

    bool validate(uint32_t candidate) const override;

protected:
    uint32_t normalizeValue(uint32_t candidate) const override;
    CapabilityType type() const override;
};
