#pragma once

#include "capabilities/ButtonCapability.h"
#include "capabilities/Capability.h"
#include "capabilities/CapabilityRegistry.h"
#include "capabilities/EnumCapability.h"
#include "capabilities/NumberCapability.h"
#include "capabilities/ToggleCapability.h"
#include "telemetry/TelemetryValue.h"

#include <esp32-hal-timer.h>
#include <map>

using InterruptCallback = void (*)();

class Interrupter {
public:
    Interrupter();

    bool registerCapability(Capability& capability);
    bool registerReadOnlyCapability(Capability& capability);
    bool registerTelemetry(TelemetryValue& telemetry);
    Capability* getCapability(const String& name);
    const Capability* getCapability(const String& name) const;
    bool getCapabilityValue(const String& name, uint32_t& value) const;
    bool setCapabilityValue(const String& name, uint32_t value);
    bool setCapabilityValue(const String& name, bool value);
    void setCapabilityChangeCallback(Capability::ChangeCallback callback,
                                    void* context = nullptr);
    bool reportTelemetry(const String& name, uint32_t value,
                        CapabilityType type = CapabilityType::Number);
    bool reportTelemetry(const String& name, bool value,
                        CapabilityType type = CapabilityType::Toggle);

    const CapabilityRegistry::Map& capabilities() const;
    void begin(const String& deviceName);
    void sendHeartbeat();

    bool registerGpioInterrupt(uint8_t pin, int mode, InterruptCallback callback);
    bool registerTimer(uint32_t frequencyHz, InterruptCallback callback);
    bool IRAM_ATTR setTimerAlarm(uint64_t us, bool autoreload = false,
                                 uint64_t reloadCount = 0);
    void IRAM_ATTR resetTimer();

private:
    static void onCapabilityChanged(const Capability& capability, void* context);

    CapabilityRegistry registry;
    std::map<String, TelemetryValue*> telemetry;
    hw_timer_t* timer = nullptr;
    bool bleStarted = false;
    int timerUsToTicksFactor;
};
