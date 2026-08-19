#include <Timers.h>
#include <BLEEngine.h>

static u16_t delayUs = 0;
static u16_t pwUs = 0;
static u8_t freqPulseSkips = 100;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("started");

  setupTimers();
  initBLE();
}

void loop() {
  String str = "hb";

  reportChars->setValue((uint8_t*)str.c_str(), str.length());
  reportChars->notify();

  delay(1000);
}