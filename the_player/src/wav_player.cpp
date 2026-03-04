#include "wav_player.h"

#define pBCLK 6  // QT Py Audio BFF default BITCLOCK
#define pWS 7    // QT Py Audio BFF default LRCLOCK
#define pDOUT A3 // QT Py Audio BFF default DATA

// Control update function implemented in main.cpp (runs on core 0)
extern void updateControls();

WavPlayer::WavPlayer()
    : playing_(false),
      load_(false),
      originalSampleRate_(0),
      lastSample_{},
      currentSample_{},
      sampleAccumulator_(0.0f),
      i2s_(OUTPUT),
      player_(false, 16)
{
}

void WavPlayer::begin()
{
  // Configure I2S, enable audio amp
  i2s_.setDATA(pDOUT);
  i2s_.setBCLK(pBCLK);
  i2s_.setBitsPerSample(16);

  pinMode(pWS, OUTPUT);
  digitalWrite(pWS, LOW);
}

bool WavPlayer::isPlaying() const
{
  return playing_;
}

// Play WAV file (already opened above). Runs on core 0.
void WavPlayer::play(File file)
{
  uint32_t rate;
  wavStatus status = player_.start(file, &rate);
  if ((status == WAV_OK) || (status == WAV_LOAD))
  {
    // Store original sample rate (make it accessible to core 1)
    noInterrupts();
    originalSampleRate_ = rate;
    sampleAccumulator_ = 0.0f;
    interrupts();

    // Initialize I2S at original sample rate (never change it - use software pitch shifting)
    if (i2s_.begin(rate))
    {
      // Initialize sample buffer - read first two samples for interpolation
      wavSample initSample1, initSample2;
      if (player_.nextSample(&initSample1) == WAV_OK)
      {
        noInterrupts();
        lastSample_ = initSample1;
        interrupts();
        // Try to read second sample
        if (player_.nextSample(&initSample2) == WAV_OK)
        {
          noInterrupts();
          currentSample_ = initSample2;
          interrupts();
        }
        else
        {
          // Only one sample available, use same for both
          noInterrupts();
          currentSample_ = initSample1;
          interrupts();
        }
      }

      playing_ = load_ = true;
      while (playing_)
      {
        // Core 0: handle control updates (pots) at low rate while core 1
        // handles the actual audio generation in loop1().
        updateControls();

        if (load_ || (status == WAV_LOAD))
        {
          load_ = false;
          status = player_.read();
          if (status == WAV_ERR_READ)
            playing_ = false;
        } // end load
      } // end playing
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

void WavPlayer::process(float currentPitch, float volume)
{
  if (!playing_)
  {
    return;
  }

  // Accumulate pitch to determine when to read next sample
  // For pitch > 1.0: we read samples faster (advance more per output)
  // For pitch < 1.0: we read samples slower (advance less per output)
  sampleAccumulator_ += currentPitch;

  // When accumulator >= 1.0, we need to read a new sample
  while (sampleAccumulator_ >= 1.0f)
  {
    // Read one sample from player
    wavStatus status = nextSample(&currentSample_);
    if (status == WAV_LOAD)
    {
      load_ = true;
      sampleAccumulator_ -= 1.0f;
      break; // Let core 0 handle loading
    }
    else if (status == WAV_EOF || status == WAV_ERR_READ)
    {
      playing_ = load_ = false;
      sampleAccumulator_ = 0.0f;
      break;
    }
    else if (status == WAV_OK)
    {
      // Store previous sample for interpolation
      lastSample_ = currentSample_;
      sampleAccumulator_ -= 1.0f;
    }
  }

  // Calculate interpolated sample based on fractional position
  // When sampleAccumulator is between 0 and 1, interpolate between lastSample and currentSample
  float frac = sampleAccumulator_;
  float ch0 = lastSample_.channel0 + frac * (currentSample_.channel0 - lastSample_.channel0);
  float ch1 = lastSample_.channel1 + frac * (currentSample_.channel1 - lastSample_.channel1);

  // Output interpolated sample to I2S
  i2s_.write((int32_t)(ch0 * volume - 32768));
  i2s_.write((int32_t)(ch1 * volume - 32768));
}
