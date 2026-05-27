#include <Arduino.h>

#include "file_reader.h"
#include "wav_player.h"
#include "control_updater.h"

ControlUpdater controls;
WavPlayer wavPlayer(controls);
FileReader fileReader;

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
  if (wavPlayer.isPlaying()) {
    wavPlayer.process();
  }
}
