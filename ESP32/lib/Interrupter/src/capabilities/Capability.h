#pragma once

#include <functional>
#include <ArduinoJson.h>

class Capability {
public:
    using ChangeCallback = void (*)(const Capability& capability, void* context);

    virtual ~Capability() = default;

    virtual bool validate(uint32_t candidate) const = 0;
    bool setValue(uint32_t candidate);
    String getDefinitionJson() const;
    String getValueJson() const;

    String name;
    uint32_t value;


protected:
    Capability(const String& name, uint32_t value, ChangeCallback callback = nullptr, void* context = nullptr);

    virtual const char* typeName() const = 0;
    virtual uint32_t normalizeValue(uint32_t candidate) const;
    virtual void writeParams(JsonDocument& document) const;
    void setMemberChangeCallback(std::function<void(const Capability&)> callback);

private:
    ChangeCallback changeCallback = nullptr;
    void* changeContext = nullptr;
    std::function<void(const Capability&)> memberChangeCallback;

    bool setValue(uint32_t candidate, bool invokeCallback);
    void invokeChangeCallback() const;

    friend class CapabilityRegistry;
};
