#include "Interrupter.h"

#include "BLEEngine.h"
#include "capabilities/CapabilityRegistry.h"

Interrupter::Interrupter() {
    registry.setChangeCallback(onCapabilityChanged, this);
}

bool Interrupter::registerCapability(Capability& capability) {
    return registry.registerCapability(capability);
}

bool Interrupter::registerReadOnlyCapability(Capability& capability) {
    capability.readOnly = true;
    return registry.registerCapability(capability);
}

bool Interrupter::registerTelemetry(TelemetryValue& telemetryValue) {
    if (telemetryValue.name.isEmpty() || telemetryValue.type == CapabilityType::Button) {
        return false;
    }

    telemetry[telemetryValue.name] = &telemetryValue;
    return true;
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

bool Interrupter::setCapabilityValue(const String& name, bool value) {
    return registry.setValue(name, value ? 1u : 0u);
}

void Interrupter::setCapabilityChangeCallback(
    Capability::ChangeCallback callback, void* context) {
    registry.setGlobalChangeCallback(callback, context);
}

bool Interrupter::reportTelemetry(const String& name, uint32_t value,
                                 CapabilityType type) {
    auto it = telemetry.find(name);
    if (it == telemetry.end()) {
        return false;
    }

    it->second->type = type;
    it->second->setValue(value);
    reportTelemetryValue(name, *it->second);
    return true;
}

bool Interrupter::reportTelemetry(const String& name, bool value,
                                 CapabilityType type) {
    auto it = telemetry.find(name);
    if (it == telemetry.end()) {
        return false;
    }

    it->second->type = type;
    it->second->setValue(value);
    reportTelemetryValue(name, *it->second);
    return true;
}

const CapabilityRegistry::Map& Interrupter::capabilities() const {
    return registry.all();
}

void Interrupter::begin(const String& deviceName) {
    if (bleStarted) {
        return;
    }

    initBLE(deviceName, registry, &telemetry);
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
        reportValue(capability.name, capability.value,
                    capability.capabilityType(), false);
    }
}
