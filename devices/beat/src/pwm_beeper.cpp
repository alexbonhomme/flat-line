#include "pwm_beeper.h"
#include <math.h>

void PwmBeeper::begin() {
  const uint16_t pwmCenter = Config::PWM_RANGE / 2;
  pinMode(Config::AUDIO_PWM_PIN, OUTPUT);
  analogWriteRange(Config::PWM_RANGE);
  analogWriteFreq(Config::PWM_FREQ_HZ);
  analogWrite(Config::AUDIO_PWM_PIN, pwmCenter);
  tonePhaseStep_ = Config::TWO_PI_F * Config::BEEP_FREQ_HZ / 1000000.0f;
}

void PwmBeeper::trigger(uint32_t nowMs) {
  active_ = true;
  startMs_ = nowMs;
  lastAudioUs_ = micros();
  tonePhase_ = 0.0f;
}

void PwmBeeper::service(uint32_t nowMs, uint32_t nowUs) {
  const uint16_t pwmCenter = Config::PWM_RANGE / 2;
  uint32_t elapsedUs = nowUs - lastAudioUs_;
  if (elapsedUs < Config::AUDIO_SAMPLE_US) {
    return;
  }
  lastAudioUs_ = nowUs;

  if (!active_) {
    analogWrite(Config::AUDIO_PWM_PIN, pwmCenter);
    return;
  }

  uint32_t elapsedMs = nowMs - startMs_;
  if (elapsedMs >= Config::BEEP_DURATION_MS) {
    active_ = false;
    analogWrite(Config::AUDIO_PWM_PIN, pwmCenter);
    return;
  }

  float env = 1.0f;
  float t = static_cast<float>(elapsedMs);
  if (t < Config::BEEP_ATTACK_MS) {
    env = t / Config::BEEP_ATTACK_MS;
  } else {
    float decayT = (t - Config::BEEP_ATTACK_MS) / Config::BEEP_DECAY_MS;
    env = expf(-2.8f * fmaxf(0.0f, decayT));
  }
  float tailMs = static_cast<float>(Config::BEEP_DURATION_MS) - t;
  if (tailMs < Config::BEEP_RELEASE_MS) {
    env *= fmaxf(0.0f, tailMs / Config::BEEP_RELEASE_MS);
  }

  float sample = sinf(tonePhase_) * env;
  tonePhase_ += tonePhaseStep_ * static_cast<float>(elapsedUs);
  if (tonePhase_ >= Config::TWO_PI_F) {
    tonePhase_ = fmodf(tonePhase_, Config::TWO_PI_F);
  }

  float pwm = (sample * 0.5f + 0.5f) * Config::PWM_RANGE;
  analogWrite(Config::AUDIO_PWM_PIN,
              static_cast<uint16_t>(
                  constrain(pwm, 0.0f, static_cast<float>(Config::PWM_RANGE))));
}
