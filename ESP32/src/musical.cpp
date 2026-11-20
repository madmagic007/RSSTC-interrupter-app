#include <TrackStore.h>
#include "BLEEngine.h"
#include "capabilities/ButtonCapability.h"
#include "capabilities/CapabilityRegistry.h"
#include "capabilities/EnumCapability.h"
#include "capabilities/NumberCapability.h"
#include "telemetry/BooleanTelemetry.h"
#include "telemetry/StringTelemetry.h"

#include <vector>

#define OUTPUT_PIN 18

static TonePlayer tonePlayer(OUTPUT_PIN);
static TrackStore trackStore;
static std::vector<String> trackNames;
static EnumCapability selectedTrackCapability("Track");
static volatile bool trackLoaded = false;
static String loadedTrackName;
static NumberCapability onTimeCapability(
    "On time", 100, 10, 100, &TonePlayer::setOnTimeUs);
static NumberCapability deadTimeCapability(
    "Dead time", 300, 5, 500, &TonePlayer::setDeadTimeUs);
static StringTelemetry trackLoadedTelemetry("Track loaded");
static BooleanTelemetry playbackTelemetry("Playing");

static void onPlay(const Capability& capability, void* context);
static void onPauseResume(const Capability& capability, void* context);
static void onStop(const Capability& capability, void* context);

static ButtonCapability playButton("Play", onPlay);
static ButtonCapability pauseResumeButton("Pause / Resume", onPauseResume);
static ButtonCapability stopButton("Stop", onStop);

static void acknowledgeButton(ButtonCapability& button) {
    CapabilityRegistry::setValue(button, 0);
}

static void updatePlaybackTelemetry() {
    const String loadedTrack = trackLoaded ? loadedTrackName : "";
    const bool playing =
        TonePlayer::isSequenceActive() && !TonePlayer::isSequencePaused();

    if (trackLoadedTelemetry.value != loadedTrack) {
        trackLoadedTelemetry.setValue(loadedTrack);
        BLEEngine::reportTelemetry(trackLoadedTelemetry);
    }
    if (playbackTelemetry.value != playing) {
        playbackTelemetry.setValue(playing);
        BLEEngine::reportTelemetry(playbackTelemetry);
    }
}

static void onPlay(const Capability& capability, void*) {
    if (capability.value == 0) return;

    TonePlayer::stopPlayingSequence();
    trackStore.unload();
    trackLoaded = false;
    updatePlaybackTelemetry();

    if (selectedTrackCapability.value >= trackNames.size()) {
        Serial.println("Playback failed: no track is selected");
        acknowledgeButton(playButton);
        return;
    }

    const String& trackName = trackNames[selectedTrackCapability.value];
    if (!trackStore.load(trackName.c_str())) {
        Serial.printf("Playback failed: %s\n", trackStore.error());
        acknowledgeButton(playButton);
        return;
    }

    if (!TonePlayer::startPlayingSequence(trackStore.data(), trackStore.length())) {
        Serial.println("Playback failed: sequence could not be started");
        trackStore.unload();
        acknowledgeButton(playButton);
        return;
    }

    trackLoaded = true;
    loadedTrackName = trackName;
    updatePlaybackTelemetry();
    Serial.printf("Playing track: %s\n", trackName.c_str());
    acknowledgeButton(playButton);
}

static void onPauseResume(const Capability& capability, void*) {
    if (capability.value == 0) return;

    if (trackLoaded && TonePlayer::isSequenceActive()) {
        if (TonePlayer::isSequencePaused()) {
            TonePlayer::resumePlayingSequence();
            Serial.println("Playback resumed");
        } else {
            TonePlayer::pausePlayingSequence();
            Serial.println("Playback paused");
        }
    }

    updatePlaybackTelemetry();
    acknowledgeButton(pauseResumeButton);
}

static void onStop(const Capability& capability, void*) {
    if (capability.value == 0) return;

    TonePlayer::stopPlayingSequence();
    trackStore.unload();
    trackLoaded = false;
    updatePlaybackTelemetry();
    Serial.println("Playback stopped");
    acknowledgeButton(stopButton);
}

void setup() {
    pinMode(OUTPUT_PIN, OUTPUT);
    digitalWrite(OUTPUT_PIN, LOW);

    Serial.begin(115200);
    delay(5000);

    bool trackStoreReady = trackStore.begin();
    if (!trackStoreReady) {
        Serial.println(trackStore.error());
    } else if (!trackStore.getTrackNames(trackNames)) {
        Serial.println(trackStore.error());
    } else if (trackNames.empty()) {
        Serial.println("No .tsq tracks found in LittleFS");
    }

    if (trackStoreReady) {
        trackStore.list(Serial);
    }

    std::vector<String> trackOptions = trackNames;
    if (trackOptions.empty()) {
        trackOptions.push_back("No tracks available");
    }
    if (!selectedTrackCapability.setOptions(trackOptions)) {
        Serial.println("Failed to set track capability options");
        return;
    }

    if (!BLEEngine::registerCapability(selectedTrackCapability) ||
        !BLEEngine::registerCapability(playButton) ||
        !BLEEngine::registerCapability(pauseResumeButton) ||
        !BLEEngine::registerCapability(stopButton) ||
        !BLEEngine::registerCapability(onTimeCapability) ||
        !BLEEngine::registerCapability(deadTimeCapability)) {
        Serial.println("Failed to register musical capabilities");
        return;
    }

    if (!BLEEngine::registerTelemetry(trackLoadedTelemetry) ||
        !BLEEngine::registerTelemetry(playbackTelemetry)) {
        Serial.println("Failed to register playback telemetry");
        return;
    }

    BLEEngine::initBLE("Musical Interrupter");
}

void loop() {
    if (trackLoaded && !TonePlayer::isSequenceActive() &&
        !TonePlayer::isSequencePaused()) {
        trackStore.unload();
        trackLoaded = false;
        updatePlaybackTelemetry();
    }

    static uint32_t lastHeartbeatMs = 0;
    const uint32_t now = millis();
    if (now - lastHeartbeatMs >= 1000) {
        BLEEngine::sendBLEHeartbeat();
        lastHeartbeatMs = now;
    }

    delay(200);
}
