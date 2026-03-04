
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <string.h>
#include "hardware/timer.h"
#include <I2S.h>

// I2S GPIO pin numbers
#define pBCLK A1 // QT Py Audio BFF default BITCLOCK
// #define pWS   A2 // QT Py Audio BFF default LRCLOCK
#define pDOUT A0 // QT Py Audio BFF default DATA

// === Pin configuration ===
// #define PITCH_PIN A0 // Pitch potentiometer
#define SD_CS_PIN 7 // QT Py Audio BFF SD chip select

I2S i2s(OUTPUT); // I2S peripheral is...

// === Sample data ===
// Progressive loading: file stays open, samples read on-demand
File sampleFile;                          // SD file handle (kept open)
volatile int SAMPLE_LEN = 0;              // Sample length (volatile for ISR access)
volatile uint32_t SAMPLE_RATE_HZ = 44100; // Sample rate from WAV file (volatile for ISR access)

// Circular buffer for caching samples (reduces SD card reads)
#define BUFFER_SIZE 512                  // Buffer size in samples (must be power of 2)
int16_t sampleBuffer[BUFFER_SIZE];       // Sample buffer
volatile int bufferStartSample = 0;      // First sample index in buffer
volatile int bufferSamplesAvailable = 0; // Number of samples available in buffer

// === Playback state (volatile for ISR access) ===
volatile bool playing = false;
volatile float volume = 0.0f;
volatile float pos = 0.0f;
volatile float pitch = 1.0f;
volatile float decay = 1.0f; // Volume decay factor (1.0 = no decay, use 0.9999 for slight decay)

// === Timer configuration ===
// Using hardware timer for precise timing
// Timer interval will be set based on RAW_SAMPLE_RATE
struct repeating_timer sampleTimer;

// === SD card sample loading ===
const char *SAMPLE_FILENAME = "output.raw";

const uint32_t RAW_SAMPLE_RATE = 44100; // Sample rate for raw files (adjust as needed)
const int RAW_BIT_DEPTH = 16;           // Bit depth: 8 for 8-bit unsigned (0-255), 16 for 16-bit
const bool RAW_BIG_ENDIAN = false;      // true for big-endian, false for little-endian (most files are little-endian)
const bool RAW_16BIT_SIGNED = true;     // true for signed 16-bit (-32768 to 32767), false for unsigned (0 to 65535)

// === Helper function to read sample from buffer ===
int16_t readSampleFromBuffer(int sampleIndex)
{
  noInterrupts();
  int bufferStart = bufferStartSample;
  int bufferAvailable = bufferSamplesAvailable;
  interrupts();

  // Check if sample is in current buffer
  if (sampleIndex >= bufferStart && sampleIndex < bufferStart + bufferAvailable)
  {
    // Sample is in buffer
    int offset = sampleIndex - bufferStart;
    int bufferIdx = offset & (BUFFER_SIZE - 1);
    return sampleBuffer[bufferIdx];
  }

  // Sample not in buffer - read directly from file (slow, but works)
  // This should rarely happen if buffer is refilled properly
  return readSampleFromFile(sampleIndex);
}

// === Interrupt Service Routine for sample playback ===
bool onTimer(struct repeating_timer *t)
{
  // Check if sample file is open
  if (!sampleFile || SAMPLE_LEN == 0)
  {
    // Output silence to I2S if no sample loaded
    i2s.write((int32_t)0); // Left channel
    i2s.write((int32_t)0); // Right channel
    return true;
  }

  if (playing)
  {
    int idx = (int)pos;
    int sampleLen = SAMPLE_LEN; // Local copy for safety

    if (idx >= sampleLen)
    {
      playing = false;
      // Output silence to I2S when done
      i2s.write((int32_t)0); // Left channel
      i2s.write((int32_t)0); // Right channel
    }
    else
    {
      // Linear interpolation for smoother pitch
      float frac = pos - idx;
      int16_t s0 = readSampleFromBuffer(idx);
      int16_t s1 = (idx + 1 < sampleLen) ? readSampleFromBuffer(idx + 1) : 0;
      float sample = s0 + frac * (s1 - s0);

      // Clamp sample to 16-bit signed range
      if (sample > 32767.0f)
        sample = 32767.0f;
      if (sample < -32768.0f)
        sample = -32768.0f;

      // Output to I2S (stereo - same sample to both channels)
      // Our samples are already signed 16-bit (-32768 to 32767), so we don't subtract 32768
      // like the_player.ino does (which uses unsigned samples from Adafruit_WavePlayer)
      int32_t sampleValue = (int32_t)(sample * volume);
      i2s.write(sampleValue);
      i2s.write(sampleValue);

      // Update envelope and position
      volume *= decay;
      pos += pitch;
    }
  }
  else
  {
    // Output silence to I2S when idle
    i2s.write((int32_t)0); // Left channel
    i2s.write((int32_t)0); // Right channel
  }

  return true; // Return true to keep repeating
}

// === Function to read and convert a single sample from file ===
int16_t readSampleFromFile(int sampleIndex)
{
  if (!sampleFile || sampleIndex < 0 || sampleIndex >= SAMPLE_LEN)
  {
    return 0;
  }

  // Calculate byte position in file
  size_t bytePos;
  if (RAW_BIT_DEPTH == 8)
  {
    bytePos = sampleIndex;
  }
  else // 16-bit
  {
    bytePos = sampleIndex * 2;
  }

  // Seek to position
  sampleFile.seek(bytePos);

  if (RAW_BIT_DEPTH == 8)
  {
    // Read 8-bit sample
    int readResult = sampleFile.read();
    if (readResult == -1)
    {
      return 0; // Read error
    }
    uint8_t sample8 = (uint8_t)readResult;
    int16_t sample = (int16_t)((int16_t)sample8 - 128) * 256; // Convert to signed and scale
    return sample;
  }
  else // 16-bit
  {
    // Read 16-bit sample
    int byte0 = sampleFile.read();
    int byte1 = sampleFile.read();

    if (byte0 == -1 || byte1 == -1)
    {
      return 0; // Read error
    }

    uint16_t sample16;
    if (RAW_BIG_ENDIAN)
    {
      sample16 = ((uint8_t)byte0 << 8) | (uint8_t)byte1;
    }
    else
    {
      sample16 = (uint8_t)byte0 | ((uint8_t)byte1 << 8);
    }

    if (RAW_16BIT_SIGNED)
    {
      // For signed 16-bit, cast directly
      return (int16_t)sample16;
    }
    else
    {
      // For unsigned, convert from 0-65535 to -32768 to 32767
      return (int16_t)(sample16 - 32768);
    }
  }
}

// === Function to refill buffer from SD card ===
void refillBuffer()
{
  if (!sampleFile || SAMPLE_LEN == 0)
    return;

  noInterrupts();
  int currentPos = (int)pos;
  int bufferStart = bufferStartSample;
  int bufferAvailable = bufferSamplesAvailable;
  interrupts();

  // Check if we need to refill buffer
  // Refill if buffer is empty, or if current position is getting close to buffer end
  bool needRefill = false;
  if (bufferAvailable == 0)
  {
    needRefill = true;
    bufferStart = currentPos; // Start buffer at current position
  }
  else if (currentPos >= bufferStart + bufferAvailable - BUFFER_SIZE / 4)
  {
    // Current position is within 1/4 of buffer end - refill ahead
    needRefill = true;
    bufferStart = currentPos; // Move buffer start to current position
  }

  if (!needRefill)
    return;

  // Read samples into buffer starting from bufferStart
  // Calculate byte position and seek once, then read sequentially
  size_t startBytePos;
  if (RAW_BIT_DEPTH == 8)
  {
    startBytePos = bufferStart;
  }
  else // 16-bit
  {
    startBytePos = bufferStart * 2;
  }

  // Seek to start position
  if (!sampleFile.seek(startBytePos))
  {
    Serial.print("ERROR: Buffer refill seek failed at byte ");
    Serial.println(startBytePos);
    return;
  }

  int samplesRead = 0;
  for (int i = 0; i < BUFFER_SIZE; i++)
  {
    int sampleIndex = bufferStart + i;
    if (sampleIndex >= SAMPLE_LEN)
      break; // End of file

    int16_t sample;
    if (RAW_BIT_DEPTH == 8)
    {
      int readResult = sampleFile.read();
      if (readResult == -1)
        break; // Read error
      uint8_t sample8 = (uint8_t)readResult;
      sample = (int16_t)((int16_t)sample8 - 128) * 256;
    }
    else // 16-bit
    {
      int byte0 = sampleFile.read();
      int byte1 = sampleFile.read();
      if (byte0 == -1 || byte1 == -1)
        break; // Read error

      uint16_t sample16;
      if (RAW_BIG_ENDIAN)
      {
        sample16 = ((uint8_t)byte0 << 8) | (uint8_t)byte1;
      }
      else
      {
        sample16 = (uint8_t)byte0 | ((uint8_t)byte1 << 8);
      }

      if (RAW_16BIT_SIGNED)
      {
        sample = (int16_t)sample16;
      }
      else
      {
        sample = (int16_t)(sample16 - 32768);
      }
    }

    int bufferIdx = i & (BUFFER_SIZE - 1);
    sampleBuffer[bufferIdx] = sample;
    samplesRead++;
  }

  // Update buffer state
  noInterrupts();
  bufferStartSample = bufferStart;
  bufferSamplesAvailable = samplesRead;
  interrupts();
}

bool loadSampleFromSD(const char *sampleFilename)
{
  // Close previous file if open
  if (sampleFile)
  {
    sampleFile.close();
  }

  sampleFile = SD.open(sampleFilename, FILE_READ);
  if (!sampleFile)
  {
    Serial.print("ERROR: Failed to open file '");
    Serial.print(sampleFilename);
    Serial.println("'");
    return false;
  }

  // Get file size
  size_t fileSize = sampleFile.size();
  Serial.print("File opened successfully, size: ");
  Serial.print(fileSize);
  Serial.println(" bytes");

  // Ensure we're at the beginning of the file
  sampleFile.seek(0);

  // Calculate number of samples based on bit depth
  int numSamples;
  if (RAW_BIT_DEPTH == 8)
  {
    numSamples = fileSize; // 1 byte per sample
  }
  else if (RAW_BIT_DEPTH == 16)
  {
    if (fileSize % 2 != 0)
    {
      sampleFile.close();
      Serial.println("ERROR: 16-bit file size must be even (file size must be multiple of 2)");
      return false;
    }
    numSamples = fileSize / 2; // 2 bytes per sample
  }
  else
  {
    sampleFile.close();
    Serial.print("ERROR: Unsupported bit depth: ");
    Serial.println(RAW_BIT_DEPTH);
    return false;
  }

  Serial.print("Bit depth: ");
  Serial.print(RAW_BIT_DEPTH);
  Serial.print("-bit, Samples: ");
  Serial.println(numSamples);

  // Initialize buffer state
  noInterrupts();
  SAMPLE_LEN = numSamples;
  SAMPLE_RATE_HZ = RAW_SAMPLE_RATE;
  bufferStartSample = 0;
  bufferSamplesAvailable = 0;
  interrupts();

  // Debug: Verify file is readable
  Serial.print("File size: ");
  Serial.println(fileSize);
  sampleFile.seek(0);
  Serial.print("File position after seek(0): ");
  Serial.println(sampleFile.position());

  // Read first few raw bytes to verify file reading works
  Serial.println("First 20 raw bytes from file (hex and decimal):");
  sampleFile.seek(0);
  for (int i = 0; i < 20 && i < fileSize; i++)
  {
    int readResult = sampleFile.read();
    if (readResult == -1)
    {
      Serial.print("Byte ");
      Serial.print(i);
      Serial.println(": READ ERROR (-1)");
      break;
    }
    uint8_t byte = (uint8_t)readResult;
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": 0x");
    if (byte < 16)
      Serial.print("0");
    Serial.print(byte, HEX);
    Serial.print(" (");
    Serial.print(byte);
    Serial.println(")");
  }
  sampleFile.seek(0); // Reset to beginning

  // Pre-fill buffer with initial samples
  Serial.println("Refilling buffer...");
  refillBuffer();

  noInterrupts();
  int bufAvail = bufferSamplesAvailable;
  int bufStart = bufferStartSample;
  interrupts();
  Serial.print("Buffer: ");
  Serial.print(bufAvail);
  Serial.print(" samples available, starting at sample ");
  Serial.println(bufStart);

  // Debug: Print first few samples from buffer
  Serial.println("First 10 samples from buffer:");
  for (int i = 0; i < 10 && i < bufAvail; i++)
  {
    int bufferIdx = i & (BUFFER_SIZE - 1);
    Serial.print("Buffer[");
    Serial.print(i);
    Serial.print("] = ");
    Serial.println(sampleBuffer[bufferIdx]);
  }

  // Debug: Print first few samples using readSampleFromFile
  Serial.println("First 10 samples using readSampleFromFile():");
  for (int i = 0; i < 10 && i < numSamples; i++)
  {
    int16_t sample = readSampleFromFile(i);
    Serial.print("Sample ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(sample);
  }

  Serial.println("Sample file opened for progressive loading!");
  return true;
}

void setup1()
{
  // Configure I2S on core 1 (matching the_player.ino approach)
  i2s.setDATA(pDOUT);
  i2s.setBCLK(pBCLK);
  i2s.setBitsPerSample(16);
  // Note: i2s.begin() will be called when starting playback with the sample rate
}

void setup()
{
  // Initialize serial for debugging
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println("Serial initialized");

  analogReadResolution(12);

  // Initialize SD card
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN))
  {
    Serial.println(" failed!");
    return;
  }
  Serial.println(" done.");

  // List files on SD card for debugging
  // Serial.println("\nFiles on SD card:");
  // File root = SD.open("/");
  // if (root)
  // {
  //   File entry = root.openNextFile();
  //   int fileCount = 0;
  //   while (entry)
  //   {
  //     fileCount++;
  //     Serial.print("  ");
  //     Serial.print(entry.name());
  //     if (!entry.isDirectory())
  //     {
  //       Serial.print(" (");
  //       Serial.print(entry.size());
  //       Serial.println(" bytes)");
  //     }
  //     else
  //     {
  //       Serial.println(" [DIR]");
  //     }
  //     entry.close();
  //     entry = root.openNextFile();
  //   }
  //   root.close();
  //   if (fileCount == 0)
  //   {
  //     Serial.println("  (no files found)");
  //   }
  //   Serial.println();
  // }
  // else
  // {
  //   Serial.println("  (could not open root directory)");
  // }

  // Load sample from SD card
  Serial.print("Attempting to load: ");
  Serial.println(SAMPLE_FILENAME);

  if (!loadSampleFromSD(SAMPLE_FILENAME))
  {
    Serial.println("ERROR: Sample loading failed!");
  }
  else
  {
    // Sample loaded successfully - setup timer with the file's sample rate
    // Calculate timer interval based on loaded sample rate
    uint32_t sampleRate = SAMPLE_RATE_HZ; // Read once (atomic)
    int sampleIntervalUs = 1000000 / sampleRate;

    Serial.print("Sample loaded: ");
    Serial.print(SAMPLE_LEN);
    Serial.print(" samples at ");
    Serial.print(sampleRate);
    Serial.println(" Hz");

    // Initialize I2S with the sample rate (matching the_player.ino approach)
    if (!i2s.begin(sampleRate))
    {
      Serial.println("Failed to initialize I2S!");
      return;
    }

    // Setup hardware timer for sample playback interrupt
    // Negative interval means delay in microseconds
    add_repeating_timer_us(-sampleIntervalUs, onTimer, NULL, &sampleTimer);
  }

  // Start playback
  playing = true;
  pos = 0.0f;
  volume = 0.5f;
}

void loop()
{
  // === Refill buffer from SD card ===
  refillBuffer();

  // === Read potentiometers ===
  // int rawPitch = analogRead(PITCH_PIN);

  // Map pot values to useful ranges
  // Use noInterrupts/interrupts to safely update shared variables
  noInterrupts();
  // pitch = 0.5f + (rawPitch / 4095.0f) * 1.5f; // 0.25× to 2× playback rate
  interrupts();
}