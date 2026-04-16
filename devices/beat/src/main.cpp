#include "beat_detector.h"
#include "ecg_display.h"
#include "ecg_processing.h"
#if DEBUG_USE_PWM_BEEPER
#include "pwm_beeper.h"
#else
#include "sample_player.h"
#endif

EcgProcessing ecg;
BeatDetector beatDetector;
#if DEBUG_USE_PWM_BEEPER
PwmBeeper beeper;
#else
SamplePlayer beeper;
#endif
EcgDisplay ecgDisplay;

uint32_t lastDebugMs = 0;
uint32_t lastRenderMsCore1 = 0;
#if DEBUG_FAKE_BPM
uint32_t lastFakeBeatMs = 0;
#endif
volatile uint16_t latestBpm = 0;
volatile bool latestBeatTriggered = false;

bool effectiveLeadOff(bool leadOff) {
#if DEBUG_FAKE_BPM
  (void)leadOff;
  return false;
#else
  return leadOff;
#endif
}

bool leadsOff() {
  return digitalRead(Config::LO_P_PIN) == HIGH || digitalRead(Config::LO_N_PIN) == HIGH;
}

void triggerBeeper(uint32_t nowMs, uint32_t nowUs) {
#if DEBUG_USE_PWM_BEEPER
  (void)nowUs;
  beeper.trigger(nowMs);
#else
  (void)nowMs;
  beeper.trigger(nowUs);
#endif
}

void serviceBeeper(uint32_t nowMs, uint32_t nowUs) {
#if DEBUG_USE_PWM_BEEPER
  beeper.service(nowMs, nowUs);
#else
  (void)nowMs;
  beeper.service(nowUs);
#endif
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
  const bool leadOff = effectiveLeadOff(leadsOff());

  EcgSample sample = ecg.sampleIfDue(nowUs, leadOff);
  if (sample.updated) {
    bool beatDetected =
#if DEBUG_FAKE_BPM
        debugBeatAtFixedRate(nowMs);
#else
        beatDetector.update(leadOff, sample.filtered, sample.slope, nowMs);
#endif
    uint16_t bpm =
#if DEBUG_FAKE_BPM
        DEBUG_FAKE_BPM;
#else
        beatDetector.bpm();
#endif

    latestBeatTriggered = beatDetected;
    latestBpm = bpm;

    bool displayBeat = ecg.updateDisplayWave(leadOff, bpm, beatDetected);
    if (displayBeat) {
      triggerBeeper(nowMs, nowUs);
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
      Serial.println(leadOff ? 1 : 0);
    }
#endif
  }

  serviceBeeper(nowMs, nowUs);
}

void setup1() {
  ecgDisplay.begin();
}

void loop1() {
  const uint32_t nowMs = millis();
  const bool leadOff = effectiveLeadOff(leadsOff());

  if (nowMs - lastRenderMsCore1 >= Config::DISPLAY_REFRESH_MS) {
    lastRenderMsCore1 = nowMs;
    ecgDisplay.render(
        leadOff,
        latestBpm,
        latestBeatTriggered,
        ecg.waveform(),
        ecg.waveHead());
  }
}