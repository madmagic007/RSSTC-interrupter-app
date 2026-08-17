#pragma once

#include <Timers.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define SERVICE_UUID    "00000000-0000-0000-0000-000000000000"
#define TX_CAPABILITIES "11111111-1111-1111-1111-111111111111"
#define TX_REPORT       "22222222-2222-2222-2222-222222222222"
#define RX_VALUES       "33333333-3333-3333-3333-333333332222"

static bool connected = false;
static unsigned long lastResponse = 0;

static BLECharacteristic* capabilityChars = nullptr;
static BLECharacteristic* reportChars = nullptr;
static BLECharacteristic* valueChars = nullptr;

static void reportCapabilities() {
    if (!connected || capabilityChars == nullptr) return;

    size_t total = capabilityRegistry.size();
    size_t i = 0;

    for (auto& pair : capabilityRegistry) {
        Capability* cap = pair.second;

        StaticJsonDocument<128> doc;
        doc["key"]   = pair.first;
        doc["name"]  = cap->name;
        doc["value"] = cap->value;
        doc["min"]   = cap->min;
        doc["max"]   = cap->max;

        String json;
        serializeJson(doc, json);

        capabilityChars->setValue((uint8_t*)json.c_str(), json.length());
        capabilityChars->notify();

        i++;
        delay(20);
    }
}

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        BLEDevice::getAdvertising()->stop();
        connected = true;
        lastResponse = millis();

        Serial.println("Phone connected");

        delay(20);
        reportCapabilities();
    }

    void onDisconnect(BLEServer* pServer) {
        BLEDevice::getAdvertising()->start();
        connected = false;

        Serial.println("Phone disconnected");
    }
};

class ValueCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pChar) {
        String data = pChar->getValue();
        applyIncomingJson(data);
    }
};

static void initBLE() {
    BLEDevice::init("ESP32-SSTC-interrupter");
    BLEDevice::setMTU(247);
    
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    capabilityChars = pService->createCharacteristic(
        TX_CAPABILITIES,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );

    reportChars = pService->createCharacteristic(
        TX_REPORT,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );

    valueChars = pService->createCharacteristic(
        RX_VALUES,
        BLECharacteristic::PROPERTY_WRITE
    );
    valueChars->setCallbacks(new ValueCallbacks());

    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMaxPreferred(0x12);
    pAdvertising->start();
}