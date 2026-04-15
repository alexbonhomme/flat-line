#include "app_config.h"
#include "beat_detector.h"
#include "ecg_display.h"
#include "ecg_processing.h"
#include "pwm_beeper.h"

EcgProcessing ecg;
BeatDetector beatDetector;
PwmBeeper beeper;
EcgDisplay ecgDisplay;

uint32_t lastDebugMs = 0;
uint32_t lastRenderMsCore1 = 0;

bool leadsOff() {
  return digitalRead(Config::LO_P_PIN) == HIGH || digitalRead(Config::LO_N_PIN) == HIGH;
}

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

  EcgSample sample = ecg.sampleIfDue(nowUs, leadOff);
  if (sample.updated) {
    bool beatDetected = beatDetector.update(leadOff, sample.filtered, sample.slope, nowMs);
    ecg.updateDisplayWave(leadOff, sample.filtered, beatDetector.bpm(), beatDetected);
    if (beatDetected) {
      beeper.trigger(nowMs);
    }

#if DEBUG
    if (nowMs - lastDebugMs >= 100) {
      lastDebugMs = nowMs;
      Serial.print("raw=");
      Serial.print(sample.raw);
      Serial.print(" filtered=");
      Serial.print(sample.filtered, 1);
      Serial.print(" bpm=");
      Serial.print(beatDetector.bpm());
      Serial.print(" leadOff=");
      Serial.println(leadOff ? 1 : 0);
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

  if (nowMs - lastRenderMsCore1 >= Config::DISPLAY_REFRESH_MS) {
    lastRenderMsCore1 = nowMs;
    ecgDisplay.render(
        leadOff,
        beatDetector.bpm(),
        beatDetector.lastBeatTriggered(),
        ecg.waveform(),
        ecg.waveHead());
  }
}