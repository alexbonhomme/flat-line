#pragma once

#include <Arduino.h>
#include "app_config.h"

class BeatDetector {
 public:
  bool update(bool leadOff, float signal, float slope, uint32_t nowMs);
  uint16_t bpm() const { return bpm_; }

 private:
  void recordBpm(uint16_t bpmValue);

  float peakEnvelope_ = 0.0f;
  float prevSignal_ = 0.0f;
  uint32_t lastBeatMs_ = 0;
  bool beatTriggered_ = false;

  uint16_t bpmHistory_[Config::BPM_HISTORY_SIZE];
  uint8_t bpmIndex_ = 0;
  uint8_t bpmCount_ = 0;
  uint16_t bpm_ = 0;
};
