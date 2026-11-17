#pragma once

#include "CapabilityType.h"

#include <functional>
#include <vector>

class CapabilityRegistry;

class Capability {
public:
    using ChangeCallback = void (*)(const Capability& capability, void* context);

    virtual ~Capability() = default;

    virtual bool validate(uint32_t candidate) const = 0;
    virtual bool hasRange() const;
    virtual uint32_t minValue() const;
    virtual uint32_t maxValue() const;
    virtual CapabilityType capabilityType() const = 0;
    virtual const char* typeName() const = 0;
    virtual const std::vector<String>* enumOptions() const;
    bool setValue(uint32_t candidate);

    String name;
    uint32_t value;
    bool readOnly = false;

protected:
    Capability(const String& name, uint32_t value,
               ChangeCallback callback = nullptr, void* context = nullptr);

    virtual uint32_t normalizeValue(uint32_t candidate) const;
    void setMemberChangeCallback(
        std::function<void(const Capability&)> callback);

private:
    ChangeCallback changeCallback = nullptr;
    void* changeContext = nullptr;
    std::function<void(const Capability&)> memberChangeCallback;

    bool setValue(uint32_t candidate, bool invokeCallback);
    void invokeChangeCallback() const;

    friend class CapabilityRegistry;
};
