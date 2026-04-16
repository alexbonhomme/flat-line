#pragma once

#include "app_config.h"
#include <Arduino.h>

class SamplePlayer {
public:
  void begin();
  void trigger(uint32_t nowUs);
  void service(uint32_t nowUs);

private:
  void startPlayback(uint32_t nowUs);

  bool active_ = false;
  uint32_t startUs_ = 0;
  uint32_t lastAudioUs_ = 0;
  uint16_t zeroStartSamplesLeft_ = 0;
  bool retriggerPending_ = false;
  uint16_t retriggerReleaseSamplesLeft_ = 0;
  uint16_t retriggerReleaseSamplesTotal_ = 0;
  float sourceIndex_ = 0.0f;
  float sourceStep_ = 0.0f;
};
