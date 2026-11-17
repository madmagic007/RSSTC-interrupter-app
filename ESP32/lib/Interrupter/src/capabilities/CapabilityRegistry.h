#pragma once

#include "Capability.h"

#include <map>

class CapabilityRegistry {
public:
    using ChangeCallback = Capability::ChangeCallback;
    using Map = std::map<String, Capability*>;

    bool registerCapability(Capability& capability);
    Capability* find(const String& name);
    const Capability* find(const String& name) const;
    bool getValue(const String& name, uint32_t& value) const;
    bool setValue(const String& name, uint32_t value);
    bool updateValue(const String& name, uint32_t value);

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
