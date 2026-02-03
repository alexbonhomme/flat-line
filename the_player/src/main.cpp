#include <Arduino.h>

#include "file_reader.h"
#include "wav_player.h"

volatile float volume = 1.0f;
volatile float pitch = 1.0f; // Pitch multiplier (1.0 = normal, 0.5 = half speed, 2.0 = double speed)

WavPlayer wavPlayer;
FileReader fileReader;

#define PITCH_POT_PIN A1 // Analog pin for pitch potentiometer

void setup()
{
  if (!fileReader.begin())
  {
    return;
  }
}

void setup1()
{
  wavPlayer.begin();
}

uint8_t num;

// Core 0 main loop - scans folder for WAV files to play
void loop()
{
  while (fileReader.nextWav())
  {
    File &file = fileReader.currentFile();
    wavPlayer.play(file);
    delay(1000);
    fileReader.closeCurrent();
  }
}

// Core 1 main loop handles I2S audio playback. This allows concurrency
// when pre-loading the next audio buffer without getting into hairy DMA.
// Global 'load' flag is set here when core 0 should load next chunk.
void loop1()
{
  // Read potentiometer continuously during playback (runs on core 1, concurrent with core 0)
  if (wavPlayer.isPlaying())
  {
    static unsigned long lastPotRead = 0;
    unsigned long now = millis();
    // Read potentiometer every 10ms to avoid excessive reads
    if (now - lastPotRead > 10)
    {
      int rawPitch = analogRead(PITCH_POT_PIN);
      noInterrupts();
      pitch = 0.5f + (rawPitch / 4095.0f) * 2.0f; // Range: 0.5x to 2.5x
      interrupts();
      lastPotRead = now;
    }

    // Software-based pitch shifting: control sample reading rate based on pitch
    noInterrupts();
    float currentPitch = pitch;
    interrupts();

    wavPlayer.process(currentPitch, volume);
  }
}
