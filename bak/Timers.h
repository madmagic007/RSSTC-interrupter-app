#pragma once
#include <Arduino.h>
#include <BLEEngine.h>
#include <soc/gpio_struct.h>
#include <map>
#include <ArduinoJson.h>

static hw_timer_t *timer = nullptr;
static volatile u8_t stage = 0;
static volatile u8_t zcCount = 0;

struct Capability {
    String name;
    u32_t value = 0;
    u8_t min = 0;
    u32_t max = UINT32_MAX;
};

static Capability delayUS = {
    .name = "Delay after zero cross",
    .max = 10000
};

static Capability pwUS = {
    .name = "Pulse width",
    .max = 20000
};

static Capability zcSkipCount = {
    .name = "Zero cross skips",
    .value = 50,
    .max = 100
};

static std::map<String, Capability*> capabilityRegistry = {
    { delayUS.name,      &delayUS },
    { pwUS.name,         &pwUS },
    { zcSkipCount.name,  &zcSkipCount }
};

static bool setCapabilityValue(const String& name, uint32_t value) {
    auto it = capabilityRegistry.find(name);
    if (it == capabilityRegistry.end()) return false;

    Capability* cap = it->second;
    cap->value = constrain(value, cap->min, cap->max);

    //prevent permament HIGH edge case
    if (delayUS.value + pwUS.value > 10000 && zcSkipCount.value < 2) {
        zcSkipCount.value = 3;
        reportValue(zcSkipCount.name, zcSkipCount.value);
    }

    return true;
}

static Capability* getCapability(const String& name) {
    auto it = capabilityRegistry.find(name);
    return (it != capabilityRegistry.end()) ? it->second : nullptr;
}

static void applyIncomingJson(const String& json) {
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) return;

    for (JsonPair kv : doc.as<JsonObject>()) {
        String key = kv.key().c_str();
        uint32_t value = kv.value().as<uint32_t>();

        if (!setCapabilityValue(key, value)) {
            Serial.println("Unknown capability received: " + key + ", " + value);
        }
    }
}

static String getCapabilities() {
    StaticJsonDocument<1024> doc;

    for (auto& pair : capabilityRegistry) {
        JsonObject obj = doc.createNestedObject(pair.first); // key = short machine name
        Capability* cap = pair.second;

        obj["name"]  = cap->name;
        obj["value"] = cap->value;
        obj["min"]   = cap->min;
        obj["max"]   = cap->max;
    }

    String output;
    serializeJson(doc, output);
    return output;
}

static void IRAM_ATTR timerISR() {
    switch (stage) {
        case 0: {
            stage = 1;
            timerWrite(timer, 0);
            timerAlarm(timer, delayUS.value, false, 0);

            break;
        }
        case 1: {
            stage = 2;
            timerWrite(timer, 0);
            timerAlarm(timer, pwUS.value, false, 0);

            GPIO.out_w1ts.val = (1 << 1);
        
            break;
        }
        case 2: {
            GPIO.out_w1tc.val = (1 << 1);
            stage = 99;

            break;
        }
    }
}

static void IRAM_ATTR gpioISR() {
    zcCount = zcCount + 1;
    if (zcCount < zcSkipCount.value) return;

    zcCount = 0;
    stage = 0;
    timerWrite(timer, 1);
    timerAlarm(timer, 1, false, 0);
}


static void setupTimers() {
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);

    pinMode(16, INPUT_PULLDOWN); //zero cross
    attachInterrupt(digitalPinToInterrupt(16), gpioISR, RISING);

    timer = timerBegin(1000000); // 1mhz?
    timerAttachInterrupt(timer, timerISR);
    timerAlarm(timer, 1, false, 0);
}