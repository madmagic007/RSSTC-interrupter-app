#pragma once

#include "Capability.h"
#include <map>

class CapabilityRegistry {
public:
    using ChangeCallback = Capability::ChangeCallback;
    using BeforeChangeCallback = bool (*)(Capability& capability,
                                          uint32_t& candidate);
    using Map = std::map<String, Capability*>;

    static bool registerCapability(Capability& capability);
    static bool setValue(Capability& capability, uint32_t value);

    static void setBeforeChangeCallback(BeforeChangeCallback callback);
    static void setChangeCallback(ChangeCallback callback, void* context = nullptr);
    static const Map& all();
    static bool applyIncomingJson(const String& json);

private:
    CapabilityRegistry() = delete;

    static Capability* find(const String& name);
    static bool isRegistered(const Capability& capability);
    static bool applyValue(Capability& capability, uint32_t value,
                           bool runBeforeChange, bool reportToRemote);

    static inline Map capabilities;
    static inline BeforeChangeCallback beforeChangeCallback = nullptr;
    static inline ChangeCallback changeCallback = nullptr;
    static inline void* changeContext = nullptr;
};
