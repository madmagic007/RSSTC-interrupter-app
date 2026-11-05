#include <Timers.h>
#include <BLEEngine.h>
#include <Capability.h>

#define PIN_ZC     16
#define PIN_OUTPUT 17

void setup() {
  Serial.begin(115200);

  setupTimers(PIN_ZC, PIN_OUTPUT);
  initBLE("Mains Ramped Interrupter");
}

void loop() {
  delay(1000);
  sendHB();
}