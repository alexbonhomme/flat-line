#include "control_updater.h"

ControlUpdater::ControlUpdater()
    : pitch_(1.0f),
      volume_(1.0f),
      minPitch_(0.1f),
      maxPitch_(3.0f),
      lastPotReadMs_(0),
      pitchPot_(kPitchPotPin, true),
      volumePot_(kVolumePotPin, true)
{
}

void ControlUpdater::update()
{
  unsigned long now = millis();

  // ~100 Hz control rate is plenty for knobs.
  if (now - lastPotReadMs_ <= 10)
  {
    return;
  }

  pitchPot_.update();
  volumePot_.update();
  lastPotReadMs_ = now;

  if (pitchPot_.hasChanged())
  {
    // Map smoothed 0-1023 to configured pitch range.
    const float normalizedPitch = pitchPot_.getValue() / kAdcMax;
    pitch_ = minPitch_ + normalizedPitch * (maxPitch_ - minPitch_);
  }

  if (volumePot_.hasChanged())
  {
    volume_ = volumePot_.getValue() / kAdcMax;
  }
}

void ControlUpdater::setPitchBounds(float minPitch, float maxPitch)
{
  if (minPitch > maxPitch)
  {
    const float temp = minPitch;
    minPitch = maxPitch;
    maxPitch = temp;
  }

  minPitch_ = minPitch;
  maxPitch_ = maxPitch;
  pitch_ = constrain(pitch_, minPitch_, maxPitch_);
}

float ControlUpdater::pitch() const
{
  return pitch_;
}

float ControlUpdater::volume() const
{
  return volume_;
}
