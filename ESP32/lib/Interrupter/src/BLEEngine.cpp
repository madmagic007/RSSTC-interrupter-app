#include "BLEEngine.h"
#include "capabilities/CapabilityRegistry.h"
#include "telemetry/TelemetryValue.h"

void BLEEngine::reportCapabilityDefinitions() {
    if (!connected || capabilityChars == nullptr) {
        return;
    }

    for (const auto& entry : CapabilityRegistry::all()) {
        const Capability& capability = *entry.second;

        capabilityChars->setValue(capability.getDefinitionJson());
        capabilityChars->notify();

        delay(20);
    }
}

void BLEEngine::reportCapability(const Capability& capability) {
    if (!connected || reportChars == nullptr) {
        return;
    }

    reportChars->setValue(capability.getValueJson());
    reportChars->notify();
}

bool BLEEngine::registerTelemetry(const TelemetryValue& telemetry) {
    if (telemetry.name.isEmpty()) {
        return false;
    }

    telemetries[telemetry.name] = &telemetry;
    return true;
}

void BLEEngine::reportTelemetryDefinitions() {
    if (!connected || telemetryDefinitionsChars == nullptr) {
        return;
    }

    for (const auto& entry : telemetries) {
        const TelemetryValue& telemetry = *entry.second;

        telemetryDefinitionsChars->setValue(telemetry.toJson());
        telemetryDefinitionsChars->notify();

        delay(20);
    }
}

void BLEEngine::reportTelemetry(const TelemetryValue& telementry) {
    if (!connected || telemetryUpdatesChars == nullptr) {
        return;
    }

    telemetryUpdatesChars->setValue(telementry.toJson());
    telemetryUpdatesChars->notify();
}

void BLEEngine::reportTelemetrySnapshot() {
    if (!connected || telemetryUpdatesChars == nullptr) {
        return;
    }

    for (const auto& entry : telemetries) {
        reportTelemetry(*entry.second);
        delay(20);
    }
}

void BLEEngine::sendBLEHeartbeat() {
    if (!connected || reportChars == nullptr) {
        return;
    }

    reportChars->setValue("hb");
    reportChars->notify();
}

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer*) override {
        BLEDevice::getAdvertising()->stop();
        BLEEngine::connected = true;
        Serial.println("Phone connected");
    }

    void onDisconnect(BLEServer*) override {
        BLEEngine::connected = false;
        BLEDevice::getAdvertising()->start();
        Serial.println("Phone disconnected");
    }
};

class ValueCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        const String data = characteristic->getValue();
        CapabilityRegistry::applyIncomingJson(data);
    }
};

class RequestCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        const String request = characteristic->getValue();

        if (request == "caps" || request == "capDefs") {
            BLEEngine::reportCapabilityDefinitions();
        } else if (request == "telemetry" || request == "telDefs") {
            BLEEngine::reportTelemetryDefinitions();
            BLEEngine::reportTelemetrySnapshot();
        } else {
            Serial.println("Unknown BLE request: " + request);
        }
    }
};

void BLEEngine::initBLE(const String& name) {
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
