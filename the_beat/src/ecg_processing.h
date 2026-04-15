#pragma once

#include <Arduino.h>
#include "app_config.h"

struct EcgSample {
  bool updated = false;
  uint16_t raw = 0;
  float filtered = 0.0f;
  float slope = 0.0f;
};

class EcgProcessing {
 public:
  void begin();
  EcgSample sampleIfDue(uint32_t nowUs, bool leadOff);
  void updateDisplayWave(bool leadOff, float filteredSignal, uint16_t bpm, bool beatDetected);

  const uint8_t* waveform() const { return waveY_; }
  uint16_t waveHead() const { return waveHead_; }

 private:
  void pushWaveSample(float signal);
  float syntheticEcgSample(uint16_t bpm, bool beatDetected);

  float baseline_ = 0.0f;
  float filtered_ = 0.0f;
  float prevFiltered_ = 0.0f;
  float displayHalfRange_ = 220.0f;
  float syntheticPhase_ = 0.0f;
  float displayBpm_ = 72.0f;
  uint32_t lastSampleUs_ = 0;

  uint8_t waveY_[Config::WAVE_BUFFER_SIZE];
  uint16_t waveHead_ = 0;
};
