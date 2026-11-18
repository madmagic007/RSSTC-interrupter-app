#include "BLEEngine.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void BLEEngine::resetControlMessageIds() {
    lastControlMessageId = 0;
    hasControlMessageId = false;
}

bool BLEEngine::isStaleOrDuplicateControlMessage(uint64_t id) {
    if (hasControlMessageId && id <= lastControlMessageId) {
        return true;
    }

    lastControlMessageId = id;
    hasControlMessageId = true;
    return false;
}

void BLEEngine::definitionsRequestTask(void*) {
    uint8_t request;
    if (xQueueReceive(definitionsRequestQueue, &request, 0) == pdTRUE &&
        request == 1) {
        BLEEngine::reportCapabilityDefinitions();
        BLEEngine::reportTelemetryDefinitions();
    }

    vTaskDelete(nullptr);
}

bool BLEEngine::registerCapability(Capability& capability) {
    return CapabilityRegistry::registerCapability(capability);
}

void BLEEngine::reportCapabilityDefinitions() {
    if (!connected || capabilityChars == nullptr) {
        return;
    }

    for (const Capability* entry : CapabilityRegistry::all()) {
        const Capability& capability = *entry;

        capabilityChars->setValue(capability.getDefinitionJson());
        capabilityChars->notify();

        delay(100);
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

        delay(100);
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
        BLEEngine::definitionsRequested = false;
        BLEEngine::resetControlMessageIds();
        xQueueReset(BLEEngine::definitionsRequestQueue);
        Serial.println("Phone connected");
    }

    void onDisconnect(BLEServer*) override {
        BLEEngine::connected = false;
        xQueueReset(BLEEngine::definitionsRequestQueue);
        BLEDevice::getAdvertising()->start();
        Serial.println("Phone disconnected");
    }
};

class ValueCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        const String data = characteristic->getValue();
        JsonDocument document;
        const DeserializationError error = deserializeJson(document, data);
        if (error || !document.is<JsonObject>()) {
            Serial.println("Control message rejected: expected JSON object");
            return;
        }

        JsonObjectConst message = document.as<JsonObjectConst>();
        const JsonVariantConst idValue = message["id"];
        const JsonVariantConst values = message["values"];
        if (!idValue.is<uint64_t>() || !values.is<JsonObjectConst>()) {
            Serial.println("Control message rejected: expected numeric id and values object");
            return;
        }

        if (BLEEngine::isStaleOrDuplicateControlMessage(idValue.as<uint64_t>())) {
            return;
        }

        CapabilityRegistry::applyIncomingJson(values.as<JsonObjectConst>());
    }
};

class RequestCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        const String request = characteristic->getValue();

        if (request == "defs") {
            if (BLEEngine::definitionsRequested) {
                return;
            }

            BLEEngine::definitionsRequested = true;
            const uint8_t queuedRequest = 1;
            if (xQueueSend(BLEEngine::definitionsRequestQueue, &queuedRequest, 0) != pdTRUE) {
                BLEEngine::definitionsRequested = false;
                Serial.println("Failed to queue BLE definitions request");
                return;
            }

            if (xTaskCreate(BLEEngine::definitionsRequestTask, "ble_defs", 4096, nullptr, 1, nullptr) != pdPASS) {
                xQueueReset(BLEEngine::definitionsRequestQueue);
                BLEEngine::definitionsRequested = false;
                Serial.println("Failed to create BLE definitions task");
            }
        } else {
            Serial.println("Unknown BLE request: " + request);
        }
    }
};

void BLEEngine::initBLE(const String& name) {
    definitionsRequestQueue = xQueueCreate(1, sizeof(uint8_t));
    if (definitionsRequestQueue == nullptr) {
        Serial.println("Failed to create BLE definitions request queue");
        return;
    }

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
        CONTROL_WRITE_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
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
