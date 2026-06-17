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
  volatile float pitch_;
  volatile float volume_;
  float minPitch_;
  float maxPitch_;
  unsigned long lastPotReadMs_;
  ResponsiveAnalogRead pitchPot_;
  ResponsiveAnalogRead volumePot_;
};
