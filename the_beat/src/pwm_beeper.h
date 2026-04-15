#pragma once

#include <Arduino.h>
#include "app_config.h"

class PwmBeeper {
 public:
  void begin();
  void trigger(uint32_t nowMs);
  void service(uint32_t nowMs, uint32_t nowUs);

 private:
  bool active_ = false;
  uint32_t startMs_ = 0;
  uint32_t lastAudioUs_ = 0;
  float tonePhase_ = 0.0f;
  float tonePhaseStep_ = 0.0f;
};
