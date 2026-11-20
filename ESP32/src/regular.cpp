#include "BLEEngine.h"
#include "Interrupter.h"
#include "PulseGenerator.h"
#include "freertos/FreeRTOS.h"
#include "telemetry/NumberTelemetry.h"

static constexpr uint8_t PIN_OUTPUT = 14;
static constexpr uint8_t PIN_UVLO = 20;
static constexpr uint8_t PIN_OCP = 19;

static NumberCapability pulseWidthUS("Pulse width", 30, 0, 200, &PulseGenerator::setOnTimeUs);
static NumberCapability beatsPerSecond("BPS", 16, PulseGenerator::MIN_BPS, 1000, &PulseGenerator::setBps);
static NumberTelemetry uvloTripCount("UVLO trip count");
static NumberTelemetry ocpTripCount("OCP trip count");
static NumberTelemetry averageOcpTripsPerPulse("Average OCP trips per pulse");

static constexpr uint8_t OCP_PULSE_HISTORY_SIZE = 10;
static portMUX_TYPE pulseOcpMux = portMUX_INITIALIZER_UNLOCKED;
static uint8_t currentPulseOcpTrips = 0;
static uint8_t recentPulseOcpTrips[OCP_PULSE_HISTORY_SIZE] = {};
static uint8_t recentOcpTripSum = 0;
static uint8_t recentPulseCount = 0;
static uint8_t nextPulseHistoryIndex = 0;
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
        if (recentPulseCount == OCP_PULSE_HISTORY_SIZE) {
            recentOcpTripSum -= recentPulseOcpTrips[nextPulseHistoryIndex];
        } else {
            recentPulseCount++;
        }

        recentPulseOcpTrips[nextPulseHistoryIndex] = currentPulseOcpTrips;
        recentOcpTripSum += currentPulseOcpTrips;

        nextPulseHistoryIndex = (nextPulseHistoryIndex + 1) % OCP_PULSE_HISTORY_SIZE;

        pulseWindowActive = false;
    }
    portEXIT_CRITICAL_ISR(&pulseOcpMux);
}

void setup() {
    Serial.begin(115200);

    if (!BLEEngine::registerCapability(pulseWidthUS) ||
        !BLEEngine::registerCapability(beatsPerSecond)) {
        Serial.println("Failed to register interrupter capabilities");
        return;
    }

    if (!BLEEngine::registerTelemetry(uvloTripCount) ||
        !BLEEngine::registerTelemetry(ocpTripCount) ||
        !BLEEngine::registerTelemetry(averageOcpTripsPerPulse)) {
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

    if (!PulseGenerator::setPulseEventCallbacks(onMcpwmPulseStart, onMcpwmPulseEnd)) {
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
    delay(1000);

    uint8_t tripSum;
    uint8_t pulseCount;

    portENTER_CRITICAL(&pulseOcpMux);
    tripSum = recentOcpTripSum;
    pulseCount = recentPulseCount;
    portEXIT_CRITICAL(&pulseOcpMux);

    const uint32_t average =
        pulseCount == 0 ? 0 :
        static_cast<uint32_t>((tripSum + pulseCount / 2) / pulseCount);
    averageOcpTripsPerPulse.setValue(average);

    BLEEngine::reportTelemetrySnapshot();
}
