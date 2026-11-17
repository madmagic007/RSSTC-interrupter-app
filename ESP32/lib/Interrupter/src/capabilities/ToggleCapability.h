#pragma once

#include "Capability.h"

class ToggleCapability : public Capability {
public:
    ToggleCapability(const String& name, bool initialValue,
                     ChangeCallback callback = nullptr, void* context = nullptr);

    bool validate(uint32_t candidate) const override;

protected:
    uint32_t normalizeValue(uint32_t candidate) const override;
    const char* typeName() const override;
};
