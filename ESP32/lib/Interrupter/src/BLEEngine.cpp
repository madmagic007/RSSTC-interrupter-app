#include "BLEEngine.h"

#include "Capability.h"

#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define SERVICE_UUID    "00000000-0000-0000-0000-000000000000"
#define TX_CAPABILITIES "11111111-1111-1111-1111-111111111111"
#define TX_REPORT       "22222222-2222-2222-2222-222222222222"
#define RX_VALUES       "33333333-3333-3333-3333-333333333333"

static bool connected = false;
static CapabilityRegistry* capabilityRegistry = nullptr;
static BLECharacteristic* capabilityChars = nullptr;
static BLECharacteristic* reportChars = nullptr;

static void reportCapabilities() {
    if (!connected || capabilityChars == nullptr || capabilityRegistry == nullptr) {
        return;
    }

    for (const auto& entry : capabilityRegistry->all()) {
        const Capability& capability = *entry.second;
        StaticJsonDocument<128> document;
        document["name"] = capability.name;
        document["value"] = capability.value;
        document["min"] = capability.min;
        document["max"] = capability.max;

        String json;
        serializeJson(document, json);
        capabilityChars->setValue(
            reinterpret_cast<const uint8_t*>(json.c_str()), json.length());
        capabilityChars->notify();
        delay(20);
    }
}

void sendBLEHeartbeat() {
    if (!connected || reportChars == nullptr) {
        return;
    }

    reportChars->setValue("hb");
    reportChars->notify();
}

void reportValue(const String& name, uint32_t value) {
    if (!connected || reportChars == nullptr) {
        return;
    }

    StaticJsonDocument<128> document;
    document["name"] = name;
    document["value"] = value;

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
        if (data == "caps") {
            reportCapabilities();
        } else if (capabilityRegistry != nullptr) {
            capabilityRegistry->applyIncomingJson(data);
        }
    }
};

void initBLE(const String& name, CapabilityRegistry& capabilities) {
    capabilityRegistry = &capabilities;
    BLEDevice::init(name);
    BLEDevice::setMTU(247);

    BLEServer* server = BLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    BLEService* service = server->createService(SERVICE_UUID);
    capabilityChars = service->createCharacteristic(
        TX_CAPABILITIES,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    reportChars = service->createCharacteristic(
        TX_REPORT,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    BLECharacteristic* valueChars = service->createCharacteristic(
        RX_VALUES, BLECharacteristic::PROPERTY_WRITE);
    valueChars->setCallbacks(new ValueCallbacks());

    service->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMaxPreferred(0x12);
    advertising->start();
}
