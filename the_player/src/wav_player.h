#pragma once

#include "SdFat.h"
#include <Adafruit_WavePlayer.h>
#include <I2S.h>

class WavPlayer
{
public:
  WavPlayer();

  void begin();
  void play(File file);
  void process(float currentPitch, float volume);
  bool isPlaying() const;

private:
  wavStatus nextSample(wavSample *sample);

  volatile bool playing_;
  volatile bool load_;
  volatile uint32_t originalSampleRate_;
  wavSample lastSample_;
  wavSample currentSample_;
  volatile float sampleAccumulator_;

  I2S i2s_;
  Adafruit_WavePlayer player_;
};
