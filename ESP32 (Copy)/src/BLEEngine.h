#pragma once

#include <Arduino.h>

class BLECharacteristic;

void initBLE(const String& name);
void reportValue(const String& name, u32_t value);
void sendHB();