#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "app_config.h"

class EcgDisplay {
 public:
  void begin();
  void render(bool leadOff, uint16_t bpm, bool beatTriggered, const uint8_t* wave, uint16_t waveHead);

 private:
  U8G2_SH1106_128X64_NONAME_F_HW_I2C display_{U8G2_R0, U8X8_PIN_NONE};
};
