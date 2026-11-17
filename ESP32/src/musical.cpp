#include <TrackStore.h>

#define OUTPUT_PIN 18
static TonePlayer tonePlayer(OUTPUT_PIN);
static TrackStore trackStore;

void setup() {    
    pinMode(OUTPUT_PIN, OUTPUT);
    digitalWrite(OUTPUT_PIN, LOW);

    Serial.begin(115200);
    delay(5000);

    if (!trackStore.begin()) {
        Serial.println(trackStore.error());
        return;
    }
    trackStore.list(Serial);
 
    if (!trackStore.load("nolimit")) {
        Serial.printf("load failed: %s\n", trackStore.error());
        return;
    }

    tonePlayer.startPlayingSequence(trackStore.data(), trackStore.length());
}

void loop() {
    delay(1000);
}