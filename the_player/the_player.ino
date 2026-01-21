// Audio BFF example for QT Py RP2040. Loops through all WAV files
// in specified directory of the SD Card. Files are
// not alphabetized and will usu. play in order they were installed.
// Requires SdFat - Adafruit Fork and Adafruit_WavePlayer libraries.

#include "SdFat.h"
#include <Adafruit_WavePlayer.h>
#include <I2S.h>

I2S i2s(OUTPUT);                       // I2S peripheral is...
Adafruit_WavePlayer player(false, 16); // mono speaker, 16-bit out.

volatile bool playing = false; // For syncing cores.
volatile bool load = false;
volatile float volume = 1.0f;
volatile float pitch = 1.0f;              // Pitch multiplier (1.0 = normal, 0.5 = half speed, 2.0 = double speed)
volatile uint32_t originalSampleRate = 0; // Store original WAV file sample rate (volatile for core 1 access)

// Software pitch shifting: use sample buffer and interpolation
// Note: wavSample struct is used across cores, so we need to protect access with interrupts
wavSample lastSample;
wavSample currentSample;
volatile float sampleAccumulator = 0.0f; // Accumulator for pitch-based sample reading

#if SD_FAT_TYPE == 0
SdFat sd;
File dir;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 dir;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile dir;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile dir;
FsFile file;
#else // SD_FAT_TYPE
#error invalid SD_FAT_TYPE
#endif // SD_FAT_TYPE

// I2S GPIO pin numbers
#define pBCLK 6 // QT Py Audio BFF default BITCLOCK
#define pWS   7 // QT Py Audio BFF default LRCLOCK
#define pDOUT A3 // QT Py Audio BFF default DATA

#define SD_CS_PIN A2 // QT Py Audio BFF SD chip select

#define PITCH_POT_PIN A1 // Analog pin for pitch potentiometer

#define SPI_CLOCK SD_SCK_MHZ(50)

// #if HAS_SDIO_CLASS
// #define SD_CONFIG SdioConfig(FIFO_SDIO)
#if ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
#else // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
#endif // HAS_SDIO_CLASS

void setup()
{
  // debug output at 115200 baud
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  // setup SD-card
  Serial.print("Initializing SD card...");
  if (!sd.begin(SD_CONFIG))
  {
    Serial.println(" failed!");
    return;
  }
  Serial.println(" done.");

  // Print pitch control instructions
  Serial.println("Pitch control: Send 'pitch <value>' via serial (e.g., 'pitch 1.5' for 1.5x speed)");
  Serial.println("Pitch range: 0.0 to 4.0 (1.0 = normal speed)");

  pinMode(pWS, OUTPUT);
  digitalWrite(pWS, LOW);
}

void setup1()
{
  // Configure I2S, enable audio amp
  i2s.setDATA(pDOUT);
  i2s.setBCLK(pBCLK);
  i2s.setBitsPerSample(16);
}

uint8_t num;

// Core 0 main loop - scans folder for WAV files to play
void loop()
{
  if (dir.open("/"))
  {
    while (file.openNext(&dir, O_RDONLY))
    {
      char filename[EXFAT_MAX_NAME_LENGTH + 1];
      file.getName(filename, sizeof filename);
      Serial.println(filename);
      if (file.isFile() && (filename[0] != '.') && // Skip directories & dotfiles
          !strcasecmp(&filename[strlen(filename) - 4], ".wav"))
      {
        play_i2s(file);
        delay(1000);
      } // end WAV
      file.close();
    } // end file open
  } // end dir open
} // end loop()

// Core 1 main loop handles I2S audio playback. This allows concurrency
// when pre-loading the next audio buffer without getting into hairy DMA.
// Global 'load' flag is set here when core 0 should load next chunk.
void loop1()
{
  // Read potentiometer continuously during playback (runs on core 1, concurrent with core 0)
  if (playing)
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

    // Accumulate pitch to determine when to read next sample
    // For pitch > 1.0: we read samples faster (advance more per output)
    // For pitch < 1.0: we read samples slower (advance less per output)
    sampleAccumulator += currentPitch;

    // When accumulator >= 1.0, we need to read a new sample
    while (sampleAccumulator >= 1.0f)
    {
      // Read one sample from player
      wavStatus status = player.nextSample(&currentSample);
      if (status == WAV_LOAD)
      {
        load = true;
        sampleAccumulator -= 1.0f;
        break; // Let core 0 handle loading
      }
      else if (status == WAV_EOF || status == WAV_ERR_READ)
      {
        playing = load = false;
        sampleAccumulator = 0.0f;
        break;
      }
      else if (status == WAV_OK)
      {
        // Store previous sample for interpolation
        lastSample = currentSample;
        sampleAccumulator -= 1.0f;
      }
    }

    // Calculate interpolated sample based on fractional position
    // When sampleAccumulator is between 0 and 1, interpolate between lastSample and currentSample
    float frac = sampleAccumulator;
    float ch0 = lastSample.channel0 + frac * (currentSample.channel0 - lastSample.channel0);
    float ch1 = lastSample.channel1 + frac * (currentSample.channel1 - lastSample.channel1);

    // Output interpolated sample to I2S
    i2s.write((int32_t)(ch0 * volume - 32768));
    i2s.write((int32_t)(ch1 * volume - 32768));
  } // end playing
} // end loop1()

// Play WAV file (already opened above). Runs on core 0.
void play_i2s(File file)
{
  digitalWrite(LED_BUILTIN, HIGH);
  uint32_t rate;
  wavStatus status = player.start(file, &rate);
  if ((status == WAV_OK) || (status == WAV_LOAD))
  {
    // Store original sample rate (make it accessible to core 1)
    noInterrupts();
    originalSampleRate = rate;
    sampleAccumulator = 0.0f;
    interrupts();

    // Initialize I2S at original sample rate (never change it - use software pitch shifting)
    if (i2s.begin(rate))
    {
      // Initialize sample buffer - read first two samples for interpolation
      wavSample initSample1, initSample2;
      if (player.nextSample(&initSample1) == WAV_OK)
      {
        noInterrupts();
        lastSample = initSample1;
        interrupts();
        // Try to read second sample
        if (player.nextSample(&initSample2) == WAV_OK)
        {
          noInterrupts();
          currentSample = initSample2;
          interrupts();
        }
        else
        {
          // Only one sample available, use same for both
          noInterrupts();
          currentSample = initSample1;
          interrupts();
        }
      }

      playing = load = true;
      while (playing)
      {
        if (load || (status == WAV_LOAD))
        {
          load = false;
          status = player.read();
          if (status == WAV_ERR_READ)
            playing = false;
        } // end load
      } // end playing
      i2s.write((int16_t)0);
      i2s.write((int16_t)0);
      i2s.end();
    }
    else
    {
      Serial.println("Failed to initialize I2S!");
    } // end i2s
  } // end WAV_OK
  digitalWrite(LED_BUILTIN, LOW);
} // end play()
