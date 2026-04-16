#include "beat_detector.h"
#include "ecg_display.h"
#include "ecg_processing.h"
#include "sample_player.h"

EcgProcessing ecg;
BeatDetector beatDetector;
SamplePlayer beeper;
EcgDisplay ecgDisplay;

uint32_t lastDebugMs = 0;
uint32_t lastRenderMsCore1 = 0;
#if DEBUG_FAKE_BPM
uint32_t lastFakeBeatMs = 0;
#endif
volatile uint16_t latestBpm = 0;
volatile bool latestBeatTriggered = false;

bool leadsOff() {
  return digitalRead(Config::LO_P_PIN) == HIGH || digitalRead(Config::LO_N_PIN) == HIGH;
}

#if DEBUG_FAKE_BPM
bool debugBeatAtFixedRate(uint32_t nowMs) {
  const uint32_t intervalMs = 60000UL / DEBUG_FAKE_BPM;
  if (nowMs - lastFakeBeatMs < intervalMs) {
    return false;
  }

  lastFakeBeatMs += intervalMs;
  if (nowMs - lastFakeBeatMs >= intervalMs) {
    lastFakeBeatMs = nowMs;
  }
  return true;
}
#endif

void setup() {
  analogReadResolution(12);
  pinMode(Config::LO_P_PIN, INPUT);
  pinMode(Config::LO_N_PIN, INPUT);

  ecg.begin();
  beeper.begin();

#if DEBUG
  Serial.begin(115200);
#endif
}

void loop() {
  const uint32_t nowUs = micros();
  const uint32_t nowMs = millis();
  const bool leadOff = leadsOff();
  const bool effectiveLeadOff =
#if DEBUG_FAKE_BPM
      false;
#else
      leadOff;
#endif

  EcgSample sample = ecg.sampleIfDue(nowUs, effectiveLeadOff);
  if (sample.updated) {
    bool beatDetected =
#if DEBUG_FAKE_BPM
        debugBeatAtFixedRate(nowMs);
#else
        beatDetector.update(effectiveLeadOff, sample.filtered, sample.slope, nowMs);
#endif
    uint16_t bpm =
#if DEBUG_FAKE_BPM
        DEBUG_FAKE_BPM;
#else
        beatDetector.bpm();
#endif

    latestBeatTriggered = beatDetected;
    latestBpm = bpm;

    bool displayBeat = ecg.updateDisplayWave(effectiveLeadOff, sample.filtered, bpm, beatDetected);
    if (displayBeat) {
      beeper.trigger(nowMs, nowUs);
    }

#if DEBUG
    if (nowMs - lastDebugMs >= 100) {
      lastDebugMs = nowMs;
      Serial.print("raw=");
      Serial.print(sample.raw);
      Serial.print(" filtered=");
      Serial.print(sample.filtered, 1);
      Serial.print(" bpm=");
      Serial.print(bpm);
      Serial.print(" beat=");
      Serial.print(beatDetected ? 1 : 0);
      Serial.print(" leadOff=");
      Serial.println(effectiveLeadOff ? 1 : 0);
    }
#endif
  }

  beeper.service(nowMs, nowUs);
}

void setup1() {
  ecgDisplay.begin();
}

void loop1() {
  const uint32_t nowMs = millis();
  const bool leadOff = leadsOff();
  const bool effectiveLeadOff =
#if DEBUG_FAKE_BPM
      false;
#else
      leadOff;
#endif

  if (nowMs - lastRenderMsCore1 >= Config::DISPLAY_REFRESH_MS) {
    lastRenderMsCore1 = nowMs;
    ecgDisplay.render(
        effectiveLeadOff,
        latestBpm,
        latestBeatTriggered,
        ecg.waveform(),
        ecg.waveHead());
  }
}