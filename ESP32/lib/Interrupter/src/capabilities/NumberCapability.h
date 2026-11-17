#pragma once

#include "Capability.h"

class NumberCapability : public Capability {
public:
    NumberCapability(const String& name, uint32_t value = 0, uint32_t min = 0,
                     uint32_t max = UINT32_MAX,
                     ChangeCallback callback = nullptr, void* context = nullptr);

    template <typename T>
    NumberCapability(const String& name, uint32_t value, uint32_t min, uint32_t max,
                     T& object, void (T::*callback)(uint32_t))
        : NumberCapability(name, value, min, max) {
        setMemberChangeCallback([&object, callback](const Capability& capability) {
            (object.*callback)(capability.value);
        });
    }

    template <typename T>
    NumberCapability(const String& name, uint32_t value, uint32_t min, uint32_t max,
                     T& object, bool (T::*callback)(uint32_t))
        : NumberCapability(name, value, min, max) {
        setMemberChangeCallback([&object, callback](const Capability& capability) {
            (object.*callback)(capability.value);
        });
    }

    bool validate(uint32_t candidate) const override;
    bool hasRange() const override;
    uint32_t minValue() const override;
    uint32_t maxValue() const override;
    CapabilityType capabilityType() const override;
    const char* typeName() const override;

    uint32_t min;
    uint32_t max;

protected:
    uint32_t normalizeValue(uint32_t candidate) const override;
};
