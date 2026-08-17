#include <PotReader.h>
#include <Timers.h>

static u16_t delayUs = 0;
static u16_t pwUs = 0;
static u8_t freqPulseSkips = 100;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("started");
  // Serial.println(pdMS_TO_TICKS);

  startTasks();
  setupTimers();
}

void loop() {
  u8_t zcSkips = channelPulseSkips.voltage * zcSkipsMax / vMax;
  Serial.println(zcSkips);
  delay(500);
}