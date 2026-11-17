#include "BLEEngine.h"

#include "capabilities/Capability.h"
#include "capabilities/CapabilityRegistry.h"
#include "telemetry/TelemetryValue.h"

#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define INTERRUPTER_SERVICE_UUID       "00000000-0000-0000-0000-000000000000"
#define CAPABILITY_DEFINITIONS_UUID    "11111111-1111-1111-1111-111111111111"
#define DEVICE_REPORTS_UUID            "22222222-2222-2222-2222-222222222222"
#define CONTROL_WRITE_UUID             "33333333-3333-3333-3333-333333333333"
#define TELEMETRY_UPDATES_UUID         "44444444-4444-4444-4444-444444444444"
#define TELEMETRY_DEFINITIONS_UUID     "55555555-5555-5555-5555-555555555555"
#define HOST_REQUESTS_UUID             "66666666-6666-6666-6666-666666666666"

static bool connected = false;
static CapabilityRegistry* capabilityRegistry = nullptr;
static std::map<String, TelemetryValue*>* telemetryRegistry = nullptr;
static BLECharacteristic* capabilityChars = nullptr;
static BLECharacteristic* reportChars = nullptr;
static BLECharacteristic* telemetryDefinitionsChars = nullptr;
static BLECharacteristic* telemetryUpdatesChars = nullptr;

static void reportCapabilities() {
    if (!connected || capabilityChars == nullptr || capabilityRegistry == nullptr) {
        return;
    }

    for (const auto& entry : capabilityRegistry->all()) {
        const Capability& capability = *entry.second;
        StaticJsonDocument<256> document;
        document["kind"] = "definition";
        document["name"] = capability.name;
        document["type"] = capability.typeName();
        document["value"] = capability.value;
        if (capability.hasRange()) {
            document["min"] = capability.minValue();
            document["max"] = capability.maxValue();
        }
        document["readOnly"] = capability.readOnly;
        const std::vector<String>* enumOptions = capability.enumOptions();
        if (enumOptions != nullptr) {
            JsonArray optionsArray = document.createNestedArray("options");
            for (const String& option : *enumOptions) {
                optionsArray.add(option);
            }
        }

        String json;
        serializeJson(document, json);
        capabilityChars->setValue(
            reinterpret_cast<const uint8_t*>(json.c_str()), json.length());
        capabilityChars->notify();
        delay(20);
    }

    JsonDocument completion;
    completion["kind"] = "end";
    completion["count"] = capabilityRegistry->all().size();
    String json;
    serializeJson(completion, json);
    capabilityChars->setValue(
        reinterpret_cast<const uint8_t*>(json.c_str()), json.length());
    capabilityChars->notify();
}

void reportTelemetryDefinitions() {
    if (!connected || telemetryDefinitionsChars == nullptr || telemetryRegistry == nullptr) {
        return;
    }

    for (const auto& entry : *telemetryRegistry) {
        const TelemetryValue& telemetry = *entry.second;
        JsonDocument document;
        document["kind"] = "definition";
        document["name"] = telemetry.name;
        document["type"] = telemetry.typeName();
        if (!telemetry.options.empty()) {
            JsonArray options = document.createNestedArray("options");
            for (const String& option : telemetry.options) {
                options.add(option);
            }
        }

        String json;
        serializeJson(document, json);
        telemetryDefinitionsChars->setValue(
            reinterpret_cast<const uint8_t*>(json.c_str()), json.length());
        telemetryDefinitionsChars->notify();
        delay(20);
    }

    JsonDocument completion;
    completion["kind"] = "end";
    completion["count"] = telemetryRegistry->size();
    String json;
    serializeJson(completion, json);
    telemetryDefinitionsChars->setValue(
        reinterpret_cast<const uint8_t*>(json.c_str()), json.length());
    telemetryDefinitionsChars->notify();
}

void reportTelemetryValue(const String& name, const TelemetryValue& value) {
    if (!connected || telemetryUpdatesChars == nullptr) {
        return;
    }

    JsonDocument document;
    document["kind"] = "value";
    document["name"] = name;
    document["type"] = value.typeName();
    document["value"] = value.value;

    String json;
    serializeJson(document, json);
    telemetryUpdatesChars->setValue(
        reinterpret_cast<const uint8_t*>(json.c_str()), json.length());
    telemetryUpdatesChars->notify();
}

static void reportTelemetrySnapshot() {
    if (!connected || telemetryUpdatesChars == nullptr || telemetryRegistry == nullptr) {
        return;
    }

    for (const auto& entry : *telemetryRegistry) {
        reportTelemetryValue(entry.first, *entry.second);
        delay(20);
    }

    JsonDocument completion;
    completion["kind"] = "snapshot_end";
    completion["count"] = telemetryRegistry->size();
    String json;
    serializeJson(completion, json);
    telemetryUpdatesChars->setValue(
        reinterpret_cast<const uint8_t*>(json.c_str()), json.length());
    telemetryUpdatesChars->notify();
}

void sendBLEHeartbeat() {
    if (!connected || reportChars == nullptr) {
        return;
    }

    reportChars->setValue("hb");
    reportChars->notify();
}

void reportValue(const String& name, uint32_t value) {
    reportValue(name, value, CapabilityType::Number, false);
}

void reportValue(const String& name, uint32_t value, CapabilityType type,
                 bool readOnly) {
    if (!connected || reportChars == nullptr) {
        return;
    }

    StaticJsonDocument<128> document;
    document["kind"] = "value";
    document["name"] = name;
    document["type"] = type == CapabilityType::Toggle ? "toggle"
        : type == CapabilityType::Button ? "button"
        : type == CapabilityType::Enum ? "enum"
        : "number";
    document["value"] = value;
    document["readOnly"] = readOnly;

    String json;
    serializeJson(document, json);
    reportChars->setValue(
        reinterpret_cast<const uint8_t*>(json.c_str()), json.length());
    reportChars->notify();
}

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer*) override {
        BLEDevice::getAdvertising()->stop();
        connected = true;
        Serial.println("Phone connected");
    }

    void onDisconnect(BLEServer*) override {
        connected = false;
        BLEDevice::getAdvertising()->start();
        Serial.println("Phone disconnected");
    }
};

class ValueCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        const String data = characteristic->getValue();
        if (capabilityRegistry != nullptr) {
            capabilityRegistry->applyIncomingJson(data);
        }
    }
};

class RequestCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        const String request = characteristic->getValue();
        if (request == "caps") {
            reportCapabilities();
        } else if (request == "telemetry") {
            reportTelemetryDefinitions();
            reportTelemetrySnapshot();
        } else {
            Serial.println("Unknown BLE request: " + request);
        }
    }
};

void initBLE(const String& name, CapabilityRegistry& capabilities,
             std::map<String, TelemetryValue*>* telemetry) {
    capabilityRegistry = &capabilities;
    telemetryRegistry = telemetry;
    BLEDevice::init(name);
    BLEDevice::setMTU(247);

    BLEServer* server = BLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    BLEService* service = server->createService(INTERRUPTER_SERVICE_UUID);
    capabilityChars = service->createCharacteristic(
        CAPABILITY_DEFINITIONS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    reportChars = service->createCharacteristic(
        DEVICE_REPORTS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    telemetryDefinitionsChars = service->createCharacteristic(
        TELEMETRY_DEFINITIONS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    telemetryUpdatesChars = service->createCharacteristic(
        TELEMETRY_UPDATES_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    BLECharacteristic* valueChars = service->createCharacteristic(
        CONTROL_WRITE_UUID, BLECharacteristic::PROPERTY_WRITE);
    valueChars->setCallbacks(new ValueCallbacks());

    BLECharacteristic* requestChars = service->createCharacteristic(
        HOST_REQUESTS_UUID, BLECharacteristic::PROPERTY_WRITE);
    requestChars->setCallbacks(new RequestCallbacks());

    service->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(INTERRUPTER_SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMaxPreferred(0x12);
    advertising->start();
}
