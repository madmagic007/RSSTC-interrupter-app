#include "BLEEngine.h"
#include "Interrupter.h"
#include "PulseGenerator.h"
#include "freertos/FreeRTOS.h"

static constexpr uint8_t PIN_OUTPUT = 14;
static constexpr uint8_t PIN_UVLO = 20;
static constexpr uint8_t PIN_OCP = 19;

static NumberCapability pulseWidthUS("Pulse width", 30, 0, 200, &PulseGenerator::setOnTimeUs);
static NumberCapability beatsPerSecond("BPS", 16, PulseGenerator::MIN_BPS, 1000, &PulseGenerator::setBps);
static TelemetryValue uvloTripCount("UVLO trip count");
static TelemetryValue ocpTripCount("OCP trip count");
static TelemetryValue averageOcpTripsPerPulseX100("Average OCP trips per pulse (x100)");

static portMUX_TYPE pulseOcpMux = portMUX_INITIALIZER_UNLOCKED;
static uint32_t currentPulseOcpTrips = 0;
static uint64_t intervalOcpTripSum = 0;
static uint32_t intervalPulseCount = 0;
static bool pulseWindowActive = false;

static void IRAM_ATTR onUvloTrip() {
    uvloTripCount.increment();
}

static void IRAM_ATTR onOcpTrip() {
    ocpTripCount.increment();

    portENTER_CRITICAL_ISR(&pulseOcpMux);
    if (pulseWindowActive) {
        currentPulseOcpTrips++;
    }
    portEXIT_CRITICAL_ISR(&pulseOcpMux);
}

static void IRAM_ATTR onMcpwmPulseStart() {
    portENTER_CRITICAL_ISR(&pulseOcpMux);
    currentPulseOcpTrips = 0;
    pulseWindowActive = true;
    portEXIT_CRITICAL_ISR(&pulseOcpMux);
}

static void IRAM_ATTR onMcpwmPulseEnd() {
    portENTER_CRITICAL_ISR(&pulseOcpMux);
    if (pulseWindowActive) {
        intervalOcpTripSum += currentPulseOcpTrips;
        intervalPulseCount++;
        pulseWindowActive = false;
    }
    portEXIT_CRITICAL_ISR(&pulseOcpMux);
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    if (!BLEEngine::registerCapability(pulseWidthUS) ||
        !BLEEngine::registerCapability(beatsPerSecond)) {
        Serial.println("Failed to register interrupter capabilities");
        return;
    }

    if (!BLEEngine::registerTelemetry(uvloTripCount) ||
        !BLEEngine::registerTelemetry(ocpTripCount) ||
        !BLEEngine::registerTelemetry(averageOcpTripsPerPulseX100)) {
        Serial.println("Failed to register fault telemetry");
        return;
    }

    pinMode(PIN_UVLO, INPUT_PULLUP);
    pinMode(PIN_OCP, INPUT_PULLDOWN);
    if (!Interrupter::registerGpioInterrupt(PIN_UVLO, FALLING, onUvloTrip)) {
        Serial.println("Failed to register UVLO interrupt");
        return;
    }
    if (!Interrupter::registerGpioInterrupt(PIN_OCP, RISING, onOcpTrip)) {
        Serial.println("Failed to register OCP interrupt");
        return;
    }

    if (!PulseGenerator::setPulseEventCallbacks(onMcpwmPulseStart,
                                                 onMcpwmPulseEnd)) {
        Serial.println("Failed to register MCPWM pulse callbacks");
        return;
    }
    if (!PulseGenerator::begin(PIN_OUTPUT, beatsPerSecond, pulseWidthUS)) {
        Serial.println("Failed to initialize pulse generator");
        return;
    }
    
    BLEEngine::initBLE("DRSSTC Interrupter");
}

void loop() {
    static uint32_t lastTelemetryMs = 0;
    const uint32_t now = millis();

    if (now - lastTelemetryMs >= 1000) {
        lastTelemetryMs = now;

        uint64_t tripSum;
        uint32_t pulseCount;
        portENTER_CRITICAL(&pulseOcpMux);
        tripSum = intervalOcpTripSum;
        pulseCount = intervalPulseCount;
        intervalOcpTripSum = 0;
        intervalPulseCount = 0;
        portEXIT_CRITICAL(&pulseOcpMux);

        const uint32_t averageX100 = pulseCount == 0
            ? 0
            : static_cast<uint32_t>(
                  (tripSum * 100 + pulseCount / 2) / pulseCount);
        averageOcpTripsPerPulseX100.setValue(averageX100);
        BLEEngine::reportTelemetrySnapshot();
    }
}
