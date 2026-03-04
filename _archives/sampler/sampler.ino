#include <Audio.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <SerialFlash.h>
#include <ResponsiveAnalogRead.h>

// GUItool: begin automatically generated code
AudioPlaySdRaw playRaw;
AudioAmplifier amp;
AudioOutputI2S2 i2s;
AudioConnection patchCord2(playRaw, 0, amp, 0);
AudioConnection patchCord5(amp, 0, i2s, 0);
AudioConnection patchCord6(amp, 0, i2s, 1);

// AudioOutputI2S i2s; // xy=1283.636432647705,373.6363763809204
// AudioConnection patchCord4(playRaw, 0, mixer, 0);
// AudioConnection patchCord5(mixer, 0, i2s, 0);
// AudioControlSGTL5000 sgtl5000; // xy=1142.727310180664,559.9999742507935
// GUItool: end automatically generated code

// Pin definitions for potentiometers
#define VOLUME_POT_PIN A9

// Use these with the Teensy Audio Shield
// #define SDCARD_CS_PIN    10
// #define SDCARD_MOSI_PIN  7   // Teensy 4 ignores this, uses pin 11
// #define SDCARD_SCK_PIN   14  // Teensy 4 ignores this, uses pin 13

// Use these with the Teensy 3.5 & 3.6 & 4.1 SD card
#define SDCARD_CS_PIN BUILTIN_SDCARD
#define SDCARD_MOSI_PIN 11 // not actually used
#define SDCARD_SCK_PIN 13  // not actually used

ResponsiveAnalogRead volumePot(VOLUME_POT_PIN, true);

void setup()
{
  Serial.begin(9600);
  Serial.println("Audio Sampler with Headphone Output");

  AudioMemory(12);

  // sgtl5000.enable();
  // sgtl5000.volume(0.8);
  // sgtl5000.unmuteHeadphone();

  // Configure SPI
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);

  if (!SD.begin(SDCARD_CS_PIN))
  {
    Serial.println("SD Card initialization failed!");
    while (1)
      ;
  }
  Serial.println("SD Card initialized successfully");

  playRaw.play("output.raw");
  if (!playRaw.isPlaying())
  {
    Serial.println("Failed to open audio file!");
    while (1)
      ;
  }
  Serial.println("Audio file loaded successfully");
}

void loop()
{
  volumePot.update();

  if (volumePot.hasChanged())
  {
    amp.gain((float)volumePot.getValue() / 1023.0);
  }

  // Check if playback has stopped and restart if needed
  if (!playRaw.isPlaying())
  {
    playRaw.play("output.raw");
  }

  delay(10);
}