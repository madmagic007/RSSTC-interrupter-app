#include <TrackStore.h>
#include "BLEEngine.h"
#include "capabilities/ButtonCapability.h"
#include "capabilities/CapabilityRegistry.h"
#include "capabilities/EnumCapability.h"
#include "capabilities/NumberCapability.h"
#include "telemetry/TelemetryValue.h"

#include <vector>

#define OUTPUT_PIN 18

static TonePlayer tonePlayer(OUTPUT_PIN);
static TrackStore trackStore;
static std::vector<String> trackNames;
static EnumCapability selectedTrackCapability("Track");
static volatile bool trackLoaded = false;
static NumberCapability onTimeCapability(
    "On time", 100, 10, 100, &TonePlayer::setOnTimeUs);
static NumberCapability deadTimeCapability(
    "Dead time", 300, 5, 500, &TonePlayer::setDeadTimeUs);
static TelemetryValue trackLoadedTelemetry("Track loaded");
static TelemetryValue playbackTelemetry("Playing");

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
    const uint32_t loaded = trackLoaded ? 1u : 0u;
    const uint32_t playing =
        TonePlayer::isSequenceActive() && !TonePlayer::isSequencePaused()
            ? 1u
            : 0u;

    if (trackLoadedTelemetry.value != loaded) {
        trackLoadedTelemetry.setValue(loaded);
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

    if (!trackStore.begin()) {
        Serial.println(trackStore.error());
        return;
    }

    if (!trackStore.getTrackNames(trackNames)) {
        Serial.println(trackStore.error());
        return;
    }
    if (trackNames.empty()) {
        Serial.println("No .tsq tracks found in LittleFS");
        return;
    }

    trackStore.list(Serial);
    if (!selectedTrackCapability.setOptions(trackNames)) {
        Serial.println("Failed to set track capability options");
        return;
    }

    if (!CapabilityRegistry::registerCapability(selectedTrackCapability) ||
        !CapabilityRegistry::registerCapability(playButton) ||
        !CapabilityRegistry::registerCapability(pauseResumeButton) ||
        !CapabilityRegistry::registerCapability(stopButton) ||
        !CapabilityRegistry::registerCapability(onTimeCapability) ||
        !CapabilityRegistry::registerCapability(deadTimeCapability)) {
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

    delay(50);
}
