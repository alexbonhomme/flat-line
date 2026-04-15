#include "ecg_processing.h"

void EcgProcessing::begin() {
  for (uint16_t i = 0; i < Config::WAVE_BUFFER_SIZE; i++) {
    waveY_[i] = Config::GRAPH_TOP + Config::GRAPH_HEIGHT / 2;
  }
  displayHalfRange_ = Config::DISPLAY_HALF_RANGE;
}

EcgSample EcgProcessing::sampleIfDue(uint32_t nowUs, bool leadOff) {
  EcgSample sample;
  if (nowUs - lastSampleUs_ < Config::ECG_SAMPLE_US) {
    return sample;
  }

  lastSampleUs_ += Config::ECG_SAMPLE_US;
  if (lastSampleUs_ == 0) {
    lastSampleUs_ = nowUs;
  }

  sample.updated = true;
  sample.raw = analogRead(Config::ECG_PIN);

  if (baseline_ == 0.0f) {
    baseline_ = static_cast<float>(sample.raw);
  }

  baseline_ += (sample.raw - baseline_) * Config::HP_BASELINE_ALPHA;
  float highPassed = sample.raw - baseline_;
  filtered_ += (highPassed - filtered_) * Config::SIGNAL_SMOOTH_ALPHA;

  sample.filtered = filtered_;
  sample.slope = filtered_ - prevFiltered_;
  prevFiltered_ = filtered_;

  return sample;
}

bool EcgProcessing::updateDisplayWave(bool leadOff, float filteredSignal, uint16_t bpm, bool beatDetected) {
  displayBeatPulse_ = false;
  if (leadOff) {
    pushWaveSample(0.0f);
    return false;
  }

  float signal = syntheticEcgSample(bpm, beatDetected);
  (void)filteredSignal;
  pushWaveSample(signal);
  return displayBeatPulse_;
}

float EcgProcessing::syntheticEcgSample(uint16_t bpm, bool beatDetected) {
  constexpr float kRPhase = 0.245f;
  float targetBpm = static_cast<float>(bpm > 35 ? bpm : static_cast<uint16_t>(displayBpm_));
  targetBpm = constrain(targetBpm, 45.0f, 130.0f);
  // Smooth display tempo so noisy beat-to-beat variation does not change sweep speed abruptly.
  displayBpm_ += (targetBpm - displayBpm_) * 0.02f;

  float effectiveBpm = displayBpm_;
  float samplesPerBeat = (Config::ECG_SAMPLE_HZ * 60.0f) / effectiveBpm;
  if (samplesPerBeat < 120.0f) {
    samplesPerBeat = 120.0f;
  }
  float phaseStep = 1.0f / samplesPerBeat;

  if (beatDetected) {
    float phaseError = kRPhase - syntheticPhase_;
    if (phaseError > 0.5f) {
      phaseError -= 1.0f;
    } else if (phaseError < -0.5f) {
      phaseError += 1.0f;
    }
    // Keep peak alignment smooth: tiny capped nudges prevent visible R-peak jitter.
    constexpr float maxPhaseNudge = 0.015f;
    float correction = constrain(phaseError * 0.12f, -maxPhaseNudge, maxPhaseNudge);
    syntheticPhase_ += correction;
  }

  const float prevPhase = syntheticPhase_;
  syntheticPhase_ += phaseStep;
  if (syntheticPhase_ >= 1.0f) {
    syntheticPhase_ -= 1.0f;
  }
  const bool wrapped = syntheticPhase_ < prevPhase;
  displayBeatPulse_ = wrapped ? (prevPhase < kRPhase || syntheticPhase_ >= kRPhase)
                              : (prevPhase < kRPhase && syntheticPhase_ >= kRPhase);

  const float p = syntheticPhase_;
  float y = 0.0f;

  if (p < 0.10f) {            // baseline
    y = 0.0f;
  } else if (p < 0.16f) {     // P wave
    float t = (p - 0.10f) / 0.06f;
    y = 24.0f * sinf(t * Config::TWO_PI_F);
  } else if (p < 0.20f) {     // PR segment
    y = 0.0f;
  } else if (p < 0.225f) {    // Q
    float t = (p - 0.20f) / 0.025f;
    y = -35.0f * t;
  } else if (p < 0.245f) {    // R upstroke
    float t = (p - 0.225f) / 0.02f;
    y = -35.0f + t * 170.0f;
  } else if (p < 0.28f) {     // S downstroke
    float t = (p - 0.245f) / 0.035f;
    y = 135.0f - t * 205.0f;
  } else if (p < 0.38f) {     // ST segment
    y = -8.0f;
  } else if (p < 0.52f) {     // T wave
    float t = (p - 0.38f) / 0.14f;
    y = 40.0f * sinf(t * Config::TWO_PI_F);
  } else {
    y = 0.0f;
  }

  return y;
}

void EcgProcessing::pushWaveSample(float signal) {
  const float halfRange = fmaxf(80.0f, Config::DISPLAY_HALF_RANGE);
  const float gained = signal * Config::DISPLAY_GAIN;
  float clamped = constrain(gained, -halfRange, halfRange);
  float normalized = (clamped + halfRange) / (2.0f * halfRange);
  float yFloat = Config::GRAPH_BOTTOM - normalized * (Config::GRAPH_HEIGHT - 1);
  waveY_[waveHead_] = static_cast<uint8_t>(
      constrain(yFloat, static_cast<float>(Config::GRAPH_TOP), static_cast<float>(Config::GRAPH_BOTTOM)));
  waveHead_ = (waveHead_ + 1) % Config::WAVE_BUFFER_SIZE;
}
