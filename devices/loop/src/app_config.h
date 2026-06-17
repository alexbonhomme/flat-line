#pragma once

#include <Arduino.h>

#ifndef SD_FAT_TYPE
#define SD_FAT_TYPE 3
#endif

namespace Config {
// I2S pins — QT Py Audio BFF defaults
constexpr uint8_t I2S_BCLK_PIN = 6;
constexpr uint8_t I2S_WS_PIN = 7;
constexpr uint8_t I2S_DATA_PIN = A3;
constexpr uint8_t AUDIO_BITS_PER_SAMPLE = 16;

// Potentiometer pins
constexpr uint8_t PITCH_POT_PIN = A0;
constexpr uint8_t VOLUME_POT_PIN = A1;
constexpr float ADC_MAX = 1023.0f;

// Pitch / volume defaults
constexpr float DEFAULT_PITCH = 1.0f;
constexpr float DEFAULT_VOLUME = 0.0f;
constexpr float MIN_PITCH = 0.1f;
constexpr float MAX_PITCH = 3.0f;

// ~100 Hz control rate is plenty for knobs.
constexpr unsigned long POT_READ_INTERVAL_MS = 10;

// Sample conversion
constexpr float SILENCE_OFFSET = 32768.0f;

// SD card
constexpr uint8_t SD_CS_PIN = A2;
constexpr uint32_t SD_SPI_CLOCK_MHZ = 50;

// Playback
constexpr unsigned long FILE_GAP_MS = 1000;
} // namespace Config
