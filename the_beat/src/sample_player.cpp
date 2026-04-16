#include "sample_player.h"

#include "blackhole.h"
#include <math.h>

void SamplePlayer::begin() {
  const uint16_t pwmCenter = Config::PWM_RANGE / 2;
  pinMode(Config::AUDIO_PWM_PIN, OUTPUT);
  analogWriteRange(Config::PWM_RANGE);
  analogWriteFreq(Config::PWM_FREQ_HZ);
  analogWrite(Config::AUDIO_PWM_PIN, pwmCenter);
  sourceStep_ = static_cast<float>(Config::SAMPLE_SOURCE_RATE_HZ) /
                static_cast<float>(Config::AUDIO_SAMPLE_HZ);
}

void SamplePlayer::trigger(uint32_t nowMs, uint32_t nowUs) {
  (void)nowMs;
  if (active_) {
    const float releaseMs = fmaxf(Config::SAMPLE_RETRIGGER_RELEASE_MS, 0.0f);
    uint16_t releaseSamples = static_cast<uint16_t>(
        (releaseMs * static_cast<float>(Config::AUDIO_SAMPLE_HZ)) / 1000.0f);
    if (releaseSamples == 0) {
      releaseSamples = 1;
    }
    retriggerPending_ = true;
    retriggerReleaseSamplesTotal_ = releaseSamples;
    retriggerReleaseSamplesLeft_ = releaseSamples;
    return;
  }
  active_ = true;
  startUs_ = nowUs;
  lastAudioUs_ = startUs_;
  zeroStartSamplesLeft_ = 0;
  if (Config::SAMPLE_ATTACK_MS > 1.0f) {
    // Force a short zero-output lead-in before reading sampleData.
    // This avoids an initial edge when retriggering.
    zeroStartSamplesLeft_ =
        static_cast<uint16_t>(Config::AUDIO_SAMPLE_HZ / 1000U);
    if (zeroStartSamplesLeft_ == 0) {
      zeroStartSamplesLeft_ = 1;
    }
  }
  sourceIndex_ = 0.0f;
  retriggerPending_ = false;
  retriggerReleaseSamplesLeft_ = 0;
  retriggerReleaseSamplesTotal_ = 0;
}

void SamplePlayer::service(uint32_t nowMs, uint32_t nowUs) {
  (void)nowMs;
  const uint16_t pwmCenter = Config::PWM_RANGE / 2;
  if (nowUs - lastAudioUs_ < Config::AUDIO_SAMPLE_US) {
    return;
  }
  lastAudioUs_ = nowUs;

  if (!active_) {
    analogWrite(Config::AUDIO_PWM_PIN, pwmCenter);
    return;
  }

  if (zeroStartSamplesLeft_ > 0) {
    analogWrite(Config::AUDIO_PWM_PIN, pwmCenter);
    --zeroStartSamplesLeft_;
    return;
  }

  const uint32_t index = static_cast<uint32_t>(sourceIndex_);
  if (index >= static_cast<uint32_t>(SAMPLE_LEN)) {
    active_ = false;
    analogWrite(Config::AUDIO_PWM_PIN, pwmCenter);
    return;
  }

  float env = 1.0f;
  const float elapsedMs = static_cast<float>(nowUs - startUs_) / 1000.0f;
  if (Config::SAMPLE_ATTACK_MS > 0.0f && elapsedMs < Config::SAMPLE_ATTACK_MS) {
    env = elapsedMs / Config::SAMPLE_ATTACK_MS;
  }

  const float remainingMs = static_cast<float>(SAMPLE_LEN - index) * 1000.0f /
                            static_cast<float>(Config::SAMPLE_SOURCE_RATE_HZ);
  if (Config::SAMPLE_RELEASE_MS > 0.0f &&
      remainingMs < Config::SAMPLE_RELEASE_MS) {
    env *= fmaxf(0.0f, remainingMs / Config::SAMPLE_RELEASE_MS);
  }
  if (retriggerPending_ && retriggerReleaseSamplesTotal_ > 0) {
    const float retriggerEnv = static_cast<float>(retriggerReleaseSamplesLeft_) /
                               static_cast<float>(retriggerReleaseSamplesTotal_);
    env *= constrain(retriggerEnv, 0.0f, 1.0f);
  }

  const float sampleNorm = static_cast<float>(sampleData[index]) / 32768.0f;
  const float output = sampleNorm * env;
  const float pwm =
      (output * 0.5f + 0.5f) * static_cast<float>(Config::PWM_RANGE);
  analogWrite(Config::AUDIO_PWM_PIN,
              static_cast<uint16_t>(
                  constrain(pwm, 0.0f, static_cast<float>(Config::PWM_RANGE))));

  sourceIndex_ += sourceStep_;
  if (retriggerPending_) {
    if (retriggerReleaseSamplesLeft_ > 0) {
      --retriggerReleaseSamplesLeft_;
    }
    if (retriggerReleaseSamplesLeft_ == 0) {
      active_ = true;
      startUs_ = nowUs;
      lastAudioUs_ = nowUs;
      zeroStartSamplesLeft_ = 0;
      if (Config::SAMPLE_ATTACK_MS > 1.0f) {
        zeroStartSamplesLeft_ =
            static_cast<uint16_t>(Config::AUDIO_SAMPLE_HZ / 1000U);
        if (zeroStartSamplesLeft_ == 0) {
          zeroStartSamplesLeft_ = 1;
        }
      }
      sourceIndex_ = 0.0f;
      retriggerPending_ = false;
      retriggerReleaseSamplesLeft_ = 0;
      retriggerReleaseSamplesTotal_ = 0;
    }
  }
}
