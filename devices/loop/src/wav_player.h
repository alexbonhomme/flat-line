#pragma once

#include "SdFat.h"
#include <Adafruit_WavePlayer.h>
#include <I2S.h>

class ControlUpdater;

class WavPlayer
{
public:
  explicit WavPlayer(ControlUpdater &controls);

  void begin();
  void play(File file);
  void process();
  bool isPlaying() const;

private:
  wavStatus nextSample(wavSample *sample);

  volatile bool playing_;
  volatile bool load_;
  volatile uint32_t originalSampleRate_;
  wavSample prevSample_;
  wavSample nextSample_;
  float sourceFrac_;

  I2S i2s_;
  Adafruit_WavePlayer player_;
  ControlUpdater &controls_;
};
