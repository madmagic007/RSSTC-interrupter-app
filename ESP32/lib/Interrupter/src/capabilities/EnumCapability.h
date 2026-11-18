#pragma once

#include "Capability.h"

class EnumCapability : public Capability {
public:
    EnumCapability(const String& name, uint32_t initialValue = 0, ChangeCallback callback = nullptr, void* context = nullptr);
    EnumCapability(const String& name, uint32_t initialValue, const std::vector<String>& options, ChangeCallback callback = nullptr, void* context = nullptr);

    bool setOptions(const std::vector<String>& options);
    bool validate(uint32_t candidate) const override;
    const std::vector<String>* enumOptions() const;

protected:
    CapabilityType type() const override;
    void writeParams(JsonDocument& document) const override;

private:
    std::vector<String> values;
};
