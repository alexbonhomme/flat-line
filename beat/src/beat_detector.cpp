#include "beat_detector.h"
#include <math.h>

bool BeatDetector::update(bool leadOff, float signal, float slope, uint32_t nowMs) {
  beatTriggered_ = false;

  if (leadOff) {
    peakEnvelope_ = 0.0f;
    prevSignal_ = signal;
    return false;
  }

  peakEnvelope_ = fmaxf(fabsf(signal), peakEnvelope_ * Config::PEAK_DECAY);
  float threshold = fmaxf(Config::MIN_PEAK_THRESHOLD, peakEnvelope_ * Config::THRESHOLD_GAIN);

  bool inRefractory = (nowMs - lastBeatMs_) < Config::REFRACTORY_MS;
  bool risingEnough = slope > Config::MIN_RISE_SLOPE;
  bool crossedThreshold = signal > threshold && prevSignal_ <= threshold;

  if (!inRefractory && risingEnough && crossedThreshold) {
    if (lastBeatMs_ > 0) {
      uint32_t ibiMs = nowMs - lastBeatMs_;
      if (ibiMs > 280 && ibiMs < 1800) {
        recordBpm(static_cast<uint16_t>(60000UL / ibiMs));
      }
    }
    lastBeatMs_ = nowMs;
    beatTriggered_ = true;
  }

  prevSignal_ = signal;
  return beatTriggered_;
}

void BeatDetector::recordBpm(uint16_t bpmValue) {
  bpmHistory_[bpmIndex_] = bpmValue;
  bpmIndex_ = (bpmIndex_ + 1) % Config::BPM_HISTORY_SIZE;
  if (bpmCount_ < Config::BPM_HISTORY_SIZE) {
    bpmCount_++;
  }

  uint32_t sum = 0;
  for (uint8_t i = 0; i < bpmCount_; i++) {
    sum += bpmHistory_[i];
  }
  bpm_ = bpmCount_ > 0 ? static_cast<uint16_t>(sum / bpmCount_) : 0;
}
