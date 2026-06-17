#include "control_updater.h"
#include "app_config.h"

ControlUpdater::ControlUpdater()
    : pitch_(Config::DEFAULT_PITCH), volume_(Config::DEFAULT_VOLUME),
      minPitch_(Config::MIN_PITCH), maxPitch_(Config::MAX_PITCH),
      lastPotReadMs_(0), pitchPot_(Config::PITCH_POT_PIN, true),
      volumePot_(Config::VOLUME_POT_PIN, true) {}

void ControlUpdater::update() {
  unsigned long now = millis();

  if (now - lastPotReadMs_ <= Config::POT_READ_INTERVAL_MS) {
    return;
  }

  pitchPot_.update();
  volumePot_.update();
  lastPotReadMs_ = now;

  if (pitchPot_.hasChanged()) {
    // Map smoothed 0-1023 to configured pitch range.
    const float normalizedPitch = pitchPot_.getValue() / Config::ADC_MAX;
    pitch_ = minPitch_ + normalizedPitch * (maxPitch_ - minPitch_);
  }

  if (volumePot_.hasChanged()) {
    volume_ = volumePot_.getValue() / Config::ADC_MAX;
  }
}

void ControlUpdater::setPitchBounds(float minPitch, float maxPitch) {
  if (minPitch > maxPitch) {
    const float temp = minPitch;
    minPitch = maxPitch;
    maxPitch = temp;
  }

  minPitch_ = minPitch;
  maxPitch_ = maxPitch;
  pitch_ = constrain(pitch_, minPitch_, maxPitch_);
}

float ControlUpdater::pitch() const { return pitch_; }

float ControlUpdater::volume() const { return volume_; }
