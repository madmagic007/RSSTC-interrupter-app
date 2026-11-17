#include "Interrupter.h"
#include "PulseGenerator.h"

#define PIN_OUTPUT 14
#define PIN_UVLO 20
#define PIN_OCP 19

static Interrupter interrupter;
static PulseGenerator pulseGen;

static NumberCapability pulseWidthUS("Pulse width", 30, 0, 200, pulseGen, &PulseGenerator::setOnTimeUs);
static NumberCapability beatsPerSecond("BPS", 16, PulseGenerator::MIN_BPS, 1000, pulseGen, &PulseGenerator::setBps);

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
    delay(100);
    interrupter.sendHeartbeat();
}
