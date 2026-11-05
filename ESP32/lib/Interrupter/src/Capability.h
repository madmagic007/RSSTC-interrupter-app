#pragma once

#include <Arduino.h>
#include <functional>
#include <map>

class Capability {
public:
    using ChangeCallback = void (*)(const Capability& capability, void* context);

    Capability(const String& name, uint32_t value = 0, uint32_t min = 0,
              uint32_t max = UINT32_MAX, ChangeCallback callback = nullptr,
              void* context = nullptr);

    template <typename T>
    Capability(const String& name, uint32_t value, uint32_t min, uint32_t max,
               T& object, void (T::*callback)(uint32_t))
        : Capability(name, value, min, max) {
        memberChangeCallback = [&object, callback](const Capability& capability) {
            (object.*callback)(capability.value);
        };
    }

    template <typename T>
    Capability(const String& name, uint32_t value, uint32_t min, uint32_t max,
               T& object, bool (T::*callback)(uint32_t))
        : Capability(name, value, min, max) {
        memberChangeCallback = [&object, callback](const Capability& capability) {
            (object.*callback)(capability.value);
        };
    }

    virtual ~Capability() = default;

    virtual bool validate(uint32_t candidate) const;
    bool setValue(uint32_t candidate);

    String name;
    uint32_t value;
    uint32_t min;
    uint32_t max;

private:
    ChangeCallback changeCallback = nullptr;
    void* changeContext = nullptr;
    std::function<void(const Capability&)> memberChangeCallback;

    bool setValue(uint32_t candidate, bool invokeCallback);
    void invokeChangeCallback() const;

    friend class CapabilityRegistry;
};

class CapabilityRegistry {
public:
    using ChangeCallback = Capability::ChangeCallback;
    using Map = std::map<String, Capability*>;

    bool registerCapability(Capability& capability);
    Capability* find(const String& name);
    const Capability* find(const String& name) const;
    bool getValue(const String& name, uint32_t& value) const;
    bool setValue(const String& name, uint32_t value);

    void setGlobalChangeCallback(ChangeCallback callback, void* context = nullptr);
    void setChangeCallback(ChangeCallback callback, void* context = nullptr);
    const Map& all() const;
    String toJson() const;
    bool applyIncomingJson(const String& json);

private:
    Map capabilities;
    ChangeCallback globalChangeCallback = nullptr;
    void* globalChangeContext = nullptr;
    ChangeCallback changeCallback = nullptr;
    void* changeContext = nullptr;
};
