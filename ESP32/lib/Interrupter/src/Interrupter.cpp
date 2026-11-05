#include "Interrupter.h"

#include "BLEEngine.h"

Interrupter::Interrupter() {
    registry.setChangeCallback(onCapabilityChanged, this);
}

bool Interrupter::registerCapability(Capability& capability) {
    return registry.registerCapability(capability);
}

Capability* Interrupter::getCapability(const String& name) {
    return registry.find(name);
}

const Capability* Interrupter::getCapability(const String& name) const {
    return registry.find(name);
}

bool Interrupter::getCapabilityValue(const String& name, uint32_t& value) const {
    return registry.getValue(name, value);
}

bool Interrupter::setCapabilityValue(const String& name, uint32_t value) {
    return registry.setValue(name, value);
}

void Interrupter::setCapabilityChangeCallback(
    Capability::ChangeCallback callback, void* context) {
    registry.setGlobalChangeCallback(callback, context);
}

const CapabilityRegistry::Map& Interrupter::capabilities() const {
    return registry.all();
}

void Interrupter::begin(const String& deviceName) {
    if (bleStarted) {
        return;
    }

    initBLE(deviceName, registry);
    bleStarted = true;
}

void Interrupter::sendHeartbeat() {
    sendBLEHeartbeat();
}

bool Interrupter::registerGpioInterrupt(uint8_t pin, int mode,
                                       InterruptCallback callback) {
    if (callback == nullptr || digitalPinToInterrupt(pin) == NOT_AN_INTERRUPT) {
        return false;
    }

    attachInterrupt(digitalPinToInterrupt(pin), callback, mode);
    return true;
}

bool Interrupter::registerTimer(uint32_t frequencyHz, InterruptCallback callback) {
    if (timer != nullptr || frequencyHz == 0 || callback == nullptr) {
        return false;
    }

    timer = timerBegin(frequencyHz);
    if (timer == nullptr) {
        return false;
    }

    timerAttachInterrupt(timer, callback);
    timerUsToTicksFactor = frequencyHz / 1000000;
    return true;
}

bool Interrupter::setTimerAlarm(uint64_t us, bool autoreload,
                                uint64_t reloadCount) {
    if (timer == nullptr || us == 0) {
        return false;
    }

    resetTimer();
    timerAlarm(timer, us * timerUsToTicksFactor, autoreload, reloadCount);
    return true;
}

void Interrupter::resetTimer() {
    if (timer != nullptr) {
        timerWrite(timer, 0);
    }
}

void Interrupter::onCapabilityChanged(const Capability& capability, void* context) {
    const Interrupter* interrupter = static_cast<const Interrupter*>(context);
    if (interrupter != nullptr && interrupter->bleStarted) {
        reportValue(capability.name, capability.value);
    }
}
