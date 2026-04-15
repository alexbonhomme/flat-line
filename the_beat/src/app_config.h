#pragma once

#include <Arduino.h>

namespace Config {
constexpr float TWO_PI_F = 6.28318530717958647692f;

// Pins
constexpr uint8_t ECG_PIN = A0;
constexpr uint8_t LO_P_PIN = D2;
constexpr uint8_t LO_N_PIN = D3;
constexpr uint8_t AUDIO_PWM_PIN = D10;

// Sampling and render rates
constexpr uint16_t ECG_SAMPLE_HZ = 500;
constexpr uint32_t ECG_SAMPLE_US = 1000000UL / ECG_SAMPLE_HZ;
constexpr uint16_t DISPLAY_REFRESH_HZ = 24;
constexpr uint32_t DISPLAY_REFRESH_MS = 1000UL / DISPLAY_REFRESH_HZ;

// ECG filtering
constexpr float HP_BASELINE_ALPHA = 0.0025f;
constexpr float SIGNAL_SMOOTH_ALPHA = 0.20f;

// Beat detection
constexpr float PEAK_DECAY = 0.995f;
constexpr float THRESHOLD_GAIN = 0.55f;
constexpr float MIN_PEAK_THRESHOLD = 28.0f;
constexpr float MIN_RISE_SLOPE = 5.0f;
constexpr uint32_t REFRACTORY_MS = 260;
constexpr uint8_t BPM_HISTORY_SIZE = 6;

// Audio
constexpr uint16_t AUDIO_SAMPLE_HZ = 8000;
constexpr uint32_t AUDIO_SAMPLE_US = 1000000UL / AUDIO_SAMPLE_HZ;
constexpr float BEEP_FREQ_HZ = 880.0f;
constexpr uint16_t BEEP_DURATION_MS = 90;
constexpr float BEEP_ATTACK_MS = 12.0f;
constexpr float BEEP_DECAY_MS = 60.0f;
constexpr uint16_t PWM_RANGE = 255;
constexpr uint16_t PWM_FREQ_HZ = 62500;

// Display layout
constexpr uint8_t SCREEN_W = 128;
constexpr uint8_t SCREEN_H = 64;
constexpr uint32_t I2C_CLOCK_HZ = 400000UL;
constexpr uint16_t WAVE_BUFFER_SIZE = 2048;
constexpr uint8_t HORIZONTAL_COMPRESS = 5;
constexpr uint8_t GRAPH_TOP = 10;
constexpr uint8_t GRAPH_BOTTOM = SCREEN_H - 1;
constexpr uint8_t GRAPH_HEIGHT = GRAPH_BOTTOM - GRAPH_TOP + 1;
constexpr float DISPLAY_HALF_RANGE = 160.0f;
constexpr float DISPLAY_GAIN = 1.35f;
}  // namespace Config
