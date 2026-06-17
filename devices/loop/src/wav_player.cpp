#include "wav_player.h"
#include "app_config.h"
#include "control_updater.h"

namespace
{
float toSigned(uint16_t sample)
{
  return static_cast<float>(sample) - Config::SILENCE_OFFSET;
}

float lerpSigned(uint16_t a, uint16_t b, float t)
{
  const float signedA = toSigned(a);
  return signedA + t * (toSigned(b) - signedA);
}
} // namespace

WavPlayer::WavPlayer(ControlUpdater &controls)
    : playing_(false),
      load_(false),
      originalSampleRate_(0),
      prevSample_{},
      nextSample_{},
      sourceFrac_(0.0f),
      i2s_(OUTPUT),
      player_(false, Config::AUDIO_BITS_PER_SAMPLE),
      controls_(controls)
{
}

void WavPlayer::begin()
{
  i2s_.setDATA(Config::I2S_DATA_PIN);
  i2s_.setBCLK(Config::I2S_BCLK_PIN);
  i2s_.setBitsPerSample(Config::AUDIO_BITS_PER_SAMPLE);

  pinMode(Config::I2S_WS_PIN, OUTPUT);
  digitalWrite(Config::I2S_WS_PIN, LOW);
}

bool WavPlayer::isPlaying() const
{
  return playing_;
}

void WavPlayer::play(File file)
{
  uint32_t rate;
  wavStatus status = player_.start(file, &rate);
  if ((status == WAV_OK) || (status == WAV_LOAD))
  {
    noInterrupts();
    originalSampleRate_ = rate;
    sourceFrac_ = 0.0f;
    interrupts();

    if (i2s_.begin(rate))
    {
      wavSample initSample{};
      if (player_.nextSample(&initSample) == WAV_OK)
      {
        noInterrupts();
        prevSample_ = initSample;
        interrupts();

        if (player_.nextSample(&initSample) == WAV_OK)
        {
          noInterrupts();
          nextSample_ = initSample;
          interrupts();
        }
        else
        {
          noInterrupts();
          nextSample_ = prevSample_;
          interrupts();
        }
      }

      playing_ = load_ = true;
      while (playing_)
      {
        controls_.update();

        if (load_ || (status == WAV_LOAD))
        {
          load_ = false;
          status = player_.read();
          if (status == WAV_ERR_READ)
            playing_ = false;
        }
      }

      i2s_.write((int32_t)0);
      i2s_.write((int32_t)0);
      i2s_.end();
    }
#if DEBUG
    else
    {
      Serial.println("Failed to initialize I2S!");
    }
#endif
  }
}

wavStatus WavPlayer::nextSample(wavSample *sample)
{
  return player_.nextSample(sample);
}

void WavPlayer::process()
{
  if (!playing_)
  {
    return;
  }

  const float pitch = controls_.pitch();
  const float volume = controls_.volume();

  // Interpolate first, then advance — matches the_loop's pos-based resampler.
  const float ch0 = lerpSigned(prevSample_.channel0, nextSample_.channel0, sourceFrac_);
  const float ch1 = lerpSigned(prevSample_.channel1, nextSample_.channel1, sourceFrac_);

  i2s_.write((int32_t)(ch0 * volume));
  i2s_.write((int32_t)(ch1 * volume));

  sourceFrac_ += pitch;
  while (sourceFrac_ >= 1.0f)
  {
    prevSample_ = nextSample_;
    const wavStatus status = nextSample(&nextSample_);
    if (status == WAV_LOAD)
    {
      load_ = true;
      sourceFrac_ -= 1.0f;
      break;
    }
    else if (status == WAV_EOF || status == WAV_ERR_READ)
    {
      playing_ = load_ = false;
      sourceFrac_ = 0.0f;
      break;
    }
    else if (status == WAV_OK)
    {
      sourceFrac_ -= 1.0f;
    }
  }
}
