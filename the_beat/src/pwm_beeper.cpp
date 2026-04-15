#include "pwm_beeper.h"
#include <math.h>

void PwmBeeper::begin() {
  pinMode(Config::AUDIO_PWM_PIN, OUTPUT);
  analogWriteRange(Config::PWM_RANGE);
  analogWriteFreq(Config::PWM_FREQ_HZ);
  analogWrite(Config::AUDIO_PWM_PIN, 0);
  tonePhaseStep_ = Config::TWO_PI_F * Config::BEEP_FREQ_HZ / static_cast<float>(Config::AUDIO_SAMPLE_HZ);
}

void PwmBeeper::trigger(uint32_t nowMs) {
  active_ = true;
  startMs_ = nowMs;
  tonePhase_ = 0.0f;
}

void PwmBeeper::service(uint32_t nowMs, uint32_t nowUs) {
  if (nowUs - lastAudioUs_ < Config::AUDIO_SAMPLE_US) {
    return;
  }
  lastAudioUs_ = nowUs;

  if (!active_) {
    analogWrite(Config::AUDIO_PWM_PIN, 0);
    return;
  }

  uint32_t elapsedMs = nowMs - startMs_;
  if (elapsedMs >= Config::BEEP_DURATION_MS) {
    active_ = false;
    analogWrite(Config::AUDIO_PWM_PIN, 0);
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

  float sample = sinf(tonePhase_) * env;
  tonePhase_ += tonePhaseStep_;
  if (tonePhase_ >= Config::TWO_PI_F) {
    tonePhase_ -= Config::TWO_PI_F;
  }

  float pwm = (sample * 0.5f + 0.5f) * Config::PWM_RANGE;
  analogWrite(Config::AUDIO_PWM_PIN, static_cast<uint16_t>(constrain(pwm, 0.0f, static_cast<float>(Config::PWM_RANGE))));
}
