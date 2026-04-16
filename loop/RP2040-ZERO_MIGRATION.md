# Migration Guide: XIAO RP2040 → RP2040-Zero

## Overview
This document outlines the changes needed to port the WAV player code from Seeed Studio XIAO RP2040 to Waveshare RP2040-Zero.

## ⚠️ IMPORTANT: Physical PCB Compatibility

**If you have a PCB designed for XIAO RP2040, it will NOT work with RP2040-Zero without redesign.**

The boards have:
- **Different physical dimensions** (XIAO: 21×17.8mm, RP2040-Zero: ~23×18mm)
- **Different pin layouts** (XIAO: 14 pads, RP2040-Zero: 20 pins + castellated holes)
- **Different pin spacing** (XIAO: custom, RP2040-Zero: standard 2.54mm)

**However**, the GPIO numbers are identical, so your electrical connections remain the same - you'll just need a new PCB layout.

See **[PCB_PHYSICAL_COMPATIBILITY.md](PCB_PHYSICAL_COMPATIBILITY.md)** for detailed physical compatibility analysis and PCB redesign guidance.

## Pin Mapping Comparison

### Current Pin Usage (XIAO RP2040)

| Function | XIAO Pin | GPIO Number | Notes |
|----------|----------|-------------|-------|
| I2S BCLK | 6 (D6) | GPIO6 | Bit Clock |
| I2S WS/LRCLK | 7 (D7) | GPIO7 | Word Select/Left-Right Clock |
| I2S DATA | A3 (D3) | GPIO29 | Data Output |
| SD Card CS | 0 (D0/TX) | GPIO0 | Chip Select for microSD |
| Pitch Pot | A1 (D1) | GPIO27 | Analog input for pitch control |

### RP2040-Zero Pin Mapping

The RP2040-Zero uses **direct GPIO numbers** (0-29) rather than Arduino-style pin names. All GPIO pins are available, and GPIO26-29 are ADC-capable.

| Function | RP2040-Zero GPIO | Notes |
|----------|------------------|-------|
| I2S BCLK | GPIO6 | Same GPIO number ✓ |
| I2S WS/LRCLK | GPIO7 | Same GPIO number ✓ |
| I2S DATA | GPIO29 | Same GPIO number ✓ |
| SD Card CS | GPIO0 | Same GPIO number ✓ |
| Pitch Pot | GPIO27 | Same GPIO number ✓ |

**Good News:** All GPIO numbers are identical! However, code changes are needed because:
1. XIAO RP2040 uses Arduino pin aliases (A3, A1, etc.)
2. RP2040-Zero uses direct GPIO numbers
3. PlatformIO board configuration differs

## Required Code Changes

### 1. Update `wav_player.cpp`

**Current code:**
```cpp
#define pBCLK 6  // QT Py Audio BFF default BITCLOCK
#define pWS 7    // QT Py Audio BFF default LRCLOCK
#define pDOUT A3 // QT Py Audio BFF default DATA
```

**Change to:**
```cpp
#define pBCLK 6   // I2S Bit Clock
#define pWS 7     // I2S Word Select/LRCLK
#define pDOUT 29  // I2S Data Output (GPIO29, was A3 on XIAO)
```

**Reason:** `A3` is an Arduino alias for GPIO29 on XIAO RP2040. RP2040-Zero uses direct GPIO numbers.

### 2. Update `main.cpp`

**Current code:**
```cpp
#define PITCH_POT_PIN A1 // Analog pin for pitch potentiometer
```

**Change to:**
```cpp
#define PITCH_POT_PIN 27 // Analog pin for pitch potentiometer (GPIO27, was A1 on XIAO)
```

**Reason:** `A1` is an Arduino alias for GPIO27 on XIAO RP2040. RP2040-Zero uses direct GPIO numbers.

### 3. Update `platformio.ini`

**Current configuration:**
```ini
[env:seeed_xiao_rp2040]
platform = https://github.com/Seeed-Studio/platform-seeedboards.git
board = seeed-xiao-rp2040
framework = arduino
```

**Change to:**
```ini
[env:rp2040_zero]
platform = https://github.com/earlephilhower/arduino-pico/releases/download/4.5.2/package_rp2040_index.json
board = rpipico
framework = arduino
board_build.core = earlephilhower
```

**Alternative (if using standard Pico core):**
```ini
[env:rp2040_zero]
platform = raspberrypi
board = pico
framework = arduino
```

**Note:** The RP2040-Zero is compatible with Raspberry Pi Pico pinout, so you can use the standard Pico board definition.

## Hardware Considerations

### Physical Differences

1. **Form Factor:**
   - XIAO RP2040: Compact board with castellated pads
   - RP2040-Zero: Even smaller, also castellated, designed for embedding

2. **Pin Headers:**
   - XIAO RP2040: 11 pads (numbered 0-10) + power pins
   - RP2040-Zero: 20 pins available via headers + additional pins via castellated holes

3. **Power:**
   - Both use USB-C
   - Both operate at 3.3V logic levels
   - No changes needed for power requirements

### Pin Availability

- **GPIO0-GPIO29:** All available on RP2040-Zero
- **ADC Channels:** GPIO26-29 (ADC0-ADC3) are ADC-capable on both boards
- **I2S:** Hardware I2S is available on both boards

## Migration Steps

1. **Update pin definitions** in `wav_player.cpp` and `main.cpp`
2. **Update PlatformIO configuration** in `platformio.ini`
3. **Test I2S functionality** - verify audio output works correctly
4. **Test SD card** - ensure SPI communication works (may need to verify SPI pins)
5. **Test analog input** - verify potentiometer reading works correctly

## Potential Issues & Solutions

### Issue 1: SPI Pins for SD Card
The SD card uses SPI, but the code doesn't explicitly define SPI pins. The SdFat library will use default SPI pins.

**Solution:** Verify SPI pin mapping:
- XIAO RP2040 default SPI: MOSI=GPIO3, MISO=GPIO4, SCK=GPIO2
- RP2040-Zero: Should be compatible, but verify if using custom SPI pins

### Issue 2: I2S Library Compatibility
The Arduino I2S library should work identically on both boards since they use the same RP2040 chip.

**Solution:** No changes needed - I2S library is chip-specific, not board-specific.

### Issue 3: Analog Reference
Both boards use 3.3V as analog reference, so `analogRead()` should work identically.

**Solution:** No changes needed.

## Summary

**Minimal Changes Required:**
- ✅ Replace `A3` with `29` in `wav_player.cpp`
- ✅ Replace `A1` with `27` in `main.cpp`
- ✅ Update `platformio.ini` board configuration

**No Changes Needed:**
- ✅ GPIO pin numbers (all identical)
- ✅ I2S functionality
- ✅ ADC functionality
- ✅ Core logic and algorithms

The migration is straightforward because both boards use the same RP2040 chip - only the pin naming conventions differ.
