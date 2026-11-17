#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <map>

class Capability;
class TelemetryValue;

#define INTERRUPTER_SERVICE_UUID       "00000000-0000-0000-0000-000000000000"
#define CAPABILITY_DEFINITIONS_UUID    "11111111-1111-1111-1111-111111111111"
#define DEVICE_REPORTS_UUID            "22222222-2222-2222-2222-222222222222"
#define CONTROL_WRITE_UUID             "33333333-3333-3333-3333-333333333333"
#define TELEMETRY_UPDATES_UUID         "44444444-4444-4444-4444-444444444444"
#define TELEMETRY_DEFINITIONS_UUID     "55555555-5555-5555-5555-555555555555"
#define HOST_REQUESTS_UUID             "66666666-6666-6666-6666-666666666666"

class BLEEngine {
    public:
        static void initBLE(const String& name);

        static void reportCapabilityDefinitions();
        static void reportCapability(const Capability& capability);

        static bool registerTelemetry(const TelemetryValue& telemetry);
        static void reportTelemetryDefinitions();
        static void reportTelemetry(const TelemetryValue& value);
        static void reportTelemetrySnapshot();

        static void sendBLEHeartbeat();

    private:
        BLEEngine() = delete;

        static inline bool connected = false;

        static inline std::map<String, const TelemetryValue*> telemetries;

        static inline BLECharacteristic* capabilityChars = nullptr;
        static inline BLECharacteristic* reportChars = nullptr;
        static inline BLECharacteristic* telemetryDefinitionsChars = nullptr;
        static inline BLECharacteristic* telemetryUpdatesChars = nullptr;

        friend class ServerCallbacks;
        friend class ValueCallbacks;
        friend class RequestCallbacks;
};
