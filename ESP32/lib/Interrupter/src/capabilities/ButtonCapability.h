#pragma once

#include "Capability.h"

class ButtonCapability : public Capability {
public:
    ButtonCapability(const String& name, ChangeCallback callback = nullptr,
                     void* context = nullptr);

    bool validate(uint32_t candidate) const override;

protected:
    uint32_t normalizeValue(uint32_t candidate) const override;
    CapabilityType type() const override;
};
