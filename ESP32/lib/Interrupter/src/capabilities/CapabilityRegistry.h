#pragma once

#include <map>
#include <vector>

#include "Capability.h"
#include "NumberCapability.h"
#include "ButtonCapability.h"
#include "ToggleCapability.h"
#include "EnumCapability.h"


class CapabilityRegistry {
public:
    using ChangeCallback = Capability::ChangeCallback;
    using BeforeChangeCallback = bool (*)(Capability& capability,
                                          uint32_t& candidate);
    using Map = std::map<String, Capability*>;
    using List = std::vector<Capability*>;

    static bool registerCapability(Capability& capability);
    static bool setValue(Capability& capability, uint32_t value);

    static void setBeforeChangeCallback(BeforeChangeCallback callback);
    static void setChangeCallback(ChangeCallback callback, void* context = nullptr);
    static const List& all();
    static bool applyIncomingJson(const String& json);
    static bool applyIncomingJson(JsonObjectConst updates);

private:
    CapabilityRegistry() = delete;

    static Capability* find(const String& name);
    static bool isRegistered(const Capability& capability);
    static bool applyValue(Capability& capability, uint32_t value,
                           bool runBeforeChange, bool reportToRemote);

    static inline Map capabilities;
    static inline List registrationOrder;
    static inline BeforeChangeCallback beforeChangeCallback = nullptr;
    static inline ChangeCallback changeCallback = nullptr;
    static inline void* changeContext = nullptr;
};
