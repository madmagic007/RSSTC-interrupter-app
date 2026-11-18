#pragma once

#include "Capability.h"

class NumberCapability : public Capability {
public:
    NumberCapability(const String& name, uint32_t value = 0, uint32_t min = 0, uint32_t max = INT32_MAX, ChangeCallback callback = nullptr);

    NumberCapability(const String& name, uint32_t value, uint32_t min, uint32_t max, bool (*callback)(uint32_t)) : NumberCapability(name, value, min, max) {
        setMemberChangeCallback([callback](const Capability& capability) {
            callback(capability.value);
        });
    }

    NumberCapability(const String& name, uint32_t value, uint32_t min, uint32_t max, void (*callback)(uint32_t)) : NumberCapability(name, value, min, max) {
        setMemberChangeCallback([callback](const Capability& capability) {
            callback(capability.value);
        });
    }

    bool validate(uint32_t candidate) const override;

    uint32_t min;
    uint32_t max;
protected:
    uint32_t normalizeValue(uint32_t candidate) const override;
    CapabilityType type() const override;
    void writeParams(JsonDocument& document) const override;
};
