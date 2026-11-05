#include "Interrupter.h"
#include "PulseGenerator.h"

static constexpr uint8_t PIN_OUTPUT = 18;

static Interrupter interrupter;
static PulseGenerator pulseGen;

static Capability pulseWidthUS(
    "Pulse width", 0, 0, 200, pulseGen, &PulseGenerator::setOnTimeUs);
static Capability beatsPerSecond(
    "BPS", 16, PulseGenerator::MIN_BPS, 500,
    pulseGen, &PulseGenerator::setBps);

void setup() {
    Serial.begin(115200);
    delay(2000);

    if (!interrupter.registerCapability(pulseWidthUS) ||
        !interrupter.registerCapability(beatsPerSecond)) {
        Serial.println("Failed to register interrupter capabilities");
        return;
    }

    if (!pulseGen.begin(PIN_OUTPUT, beatsPerSecond.value, pulseWidthUS.value,
                        beatsPerSecond.max, pulseWidthUS.max)) {
        Serial.println("Failed to initialize pulse generator");
        return;
    }
    interrupter.begin("DRSSTC Interrupter");
}

void loop() {
    delay(1000);
    interrupter.sendHeartbeat();
}
