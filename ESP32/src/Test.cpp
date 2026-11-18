#include "BLEEngine.h"

#include <vector>


static NumberCapability num("num");
static ButtonCapability btn("btn");
static ToggleCapability toggle("toggle");
static EnumCapability enumCap("enum", 0, std::vector<String>{"First", "Second"});

static TelemetryValue t1("t1", 0);
static TelemetryValue t2("t2", 1654968);

void setup() {
    Serial.begin(115200);

    if (!BLEEngine::registerCapability(num) ||
        !BLEEngine::registerCapability(btn) ||
        !BLEEngine::registerCapability(toggle) ||
        !BLEEngine::registerCapability(enumCap)) {
        Serial.println("Failed to register test capabilities");
        return;
    }

    BLEEngine::registerTelemetry(t1);
    BLEEngine::registerTelemetry(t2);

    BLEEngine::initBLE("Test");
}

void loop() {
    delay(1000);
    BLEEngine::sendBLEHeartbeat();
}