#pragma once

#include <Arduino.h>
#include <ResponsiveAnalogRead.h>

class ControlUpdater
{
public:
  ControlUpdater();

  void update();
  void setPitchBounds(float minPitch, float maxPitch);
  float pitch() const;
  float volume() const;

private:
  static constexpr uint8_t kPitchPotPin = A0;
  static constexpr uint8_t kVolumePotPin = A1;
  static constexpr float kAdcMax = 1023.0f;

  volatile float pitch_;
  volatile float volume_;
  float minPitch_;
  float maxPitch_;
  unsigned long lastPotReadMs_;
  ResponsiveAnalogRead pitchPot_;
  ResponsiveAnalogRead volumePot_;
};
