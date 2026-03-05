#include <Arduino.h>
#include <ResponsiveAnalogRead.h>

#include "file_reader.h"
#include "wav_player.h"

volatile float volume = 1.0f;
volatile float pitch = 1.0f;

WavPlayer wavPlayer;
FileReader fileReader;

#define PITCH_POT_PIN A0
#define VOLUME_POT_PIN A1

constexpr float ADC_MAX = 1023.0f;

// Smoothed pot readings to avoid audio glitches (no zipper noise)
ResponsiveAnalogRead pitchPot(PITCH_POT_PIN, true);
ResponsiveAnalogRead volumePot(VOLUME_POT_PIN, true);

// read and smooth pots at control rate, update globals
void updateControls() {
  static unsigned long lastPotRead = 0;
  unsigned long now = millis();

  // ~100 Hz control rate is plenty for knobs
  if (now - lastPotRead > 10) {
    pitchPot.update();
    volumePot.update();
    lastPotRead = now;

    if (pitchPot.hasChanged()) {
      // Map smoothed 0–1023 to pitch (0.5x–2.0x) and volume (0.0–1.0)
      pitch = 0.5f + (pitchPot.getValue() / ADC_MAX) * 1.5f;
    }

    if (volumePot.hasChanged()) {
      volume = volumePot.getValue() / ADC_MAX;
    }
  }
}

void setup() {
#if DEBUG
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }
  Serial.println("Starting...");
#endif

  if (!fileReader.begin()) {
    return;
  }
}

void setup1() {
  wavPlayer.begin();
}

uint8_t num;

// Core 0 main loop - scans folder for WAV files to play
void loop() {
  while (fileReader.nextWav()) {
    File &file = fileReader.currentFile();
    wavPlayer.play(file);
    delay(1000);
    fileReader.closeCurrent();
  }
}

// Core 1 main loop handles I2S audio playback. This allows concurrency
// when pre-loading the next audio buffer without getting into hairy DMA.
// Global 'load' flag is set here when core 0 should load next chunk.
void loop1() {
  // Read potentiometer continuously during playback (runs on core 1, concurrent
  // with core 0)
  if (wavPlayer.isPlaying()) {
    wavPlayer.process(pitch, volume);
  }
}
