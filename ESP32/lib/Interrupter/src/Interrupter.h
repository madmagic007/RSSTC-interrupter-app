#pragma once

#include "Capability.h"

#include <esp32-hal-timer.h>

using InterruptCallback = void (*)();

class Interrupter {
public:
    Interrupter();

    bool registerCapability(Capability& capability);
    Capability* getCapability(const String& name);
    const Capability* getCapability(const String& name) const;
    bool getCapabilityValue(const String& name, uint32_t& value) const;
    bool setCapabilityValue(const String& name, uint32_t value);
    void setCapabilityChangeCallback(Capability::ChangeCallback callback,
                                    void* context = nullptr);

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
    hw_timer_t* timer = nullptr;
    bool bleStarted = false;
    int timerUsToTicksFactor;
};
