#include "ecg_display.h"
#include <Wire.h>

namespace {
uint8_t sampleInterpolated(const uint8_t* wave, uint16_t newest, float offset) {
  if (offset < 0.0f) {
    offset = 0.0f;
  }
  const float maxOffset = static_cast<float>(Config::WAVE_BUFFER_SIZE - 1);
  if (offset > maxOffset) {
    offset = maxOffset;
  }

  const uint16_t i0 = static_cast<uint16_t>(offset);
  const uint16_t i1 = (i0 < Config::WAVE_BUFFER_SIZE - 1) ? static_cast<uint16_t>(i0 + 1) : i0;
  const float frac = offset - static_cast<float>(i0);

  const uint16_t idx0 = (newest + Config::WAVE_BUFFER_SIZE - i0) % Config::WAVE_BUFFER_SIZE;
  const uint16_t idx1 = (newest + Config::WAVE_BUFFER_SIZE - i1) % Config::WAVE_BUFFER_SIZE;

  const float y0 = static_cast<float>(wave[idx0]);
  const float y1 = static_cast<float>(wave[idx1]);
  return static_cast<uint8_t>(y0 + (y1 - y0) * frac + 0.5f);
}

uint8_t sampleSmoothed(const uint8_t* wave, uint16_t newest, float offset, float compress) {
  // Keep anti-aliasing mild so sharp QRS peaks remain visible at high compression.
  const float delta = fminf(compress * 0.2f, 1.0f);
  const uint16_t yA = sampleInterpolated(wave, newest, offset - delta);
  const uint16_t yB = sampleInterpolated(wave, newest, offset);
  const uint16_t yC = sampleInterpolated(wave, newest, offset + delta);
  return static_cast<uint8_t>((yA + (2U * yB) + yC) / 4U);
}

void sampleBinMinMax(
    const uint8_t* wave,
    uint16_t newest,
    float binStartOffset,
    float compress,
    uint8_t* outMinY,
    uint8_t* outMaxY,
    uint8_t* outCenterY) {
  // Scan each compressed bin and keep extrema to prevent R-peak amplitude loss.
  constexpr uint8_t kSubsamples = 6;
  uint8_t minY = Config::GRAPH_BOTTOM;
  uint8_t maxY = Config::GRAPH_TOP;

  for (uint8_t i = 0; i < kSubsamples; i++) {
    float frac = static_cast<float>(i) / static_cast<float>(kSubsamples - 1);
    float offset = binStartOffset - frac * compress;
    uint8_t y = sampleInterpolated(wave, newest, offset);
    if (y < minY) minY = y;
    if (y > maxY) maxY = y;
  }

  *outMinY = minY;
  *outMaxY = maxY;
  *outCenterY = static_cast<uint8_t>((static_cast<uint16_t>(minY) + static_cast<uint16_t>(maxY)) / 2U);
}
}  // namespace

void EcgDisplay::begin() {
  Wire.setClock(Config::I2C_CLOCK_HZ);
  display_.begin();
  display_.clearBuffer();
  display_.setFont(u8g2_font_5x8_tf);
  display_.drawStr(0, 12, "AD8232 ECG Monitor");
  display_.drawStr(0, 24, "Init...");
  display_.sendBuffer();
}

void EcgDisplay::render(bool leadOff, uint16_t bpm, bool beatTriggered, const uint8_t* wave, uint16_t waveHead) {
  display_.clearBuffer();

  display_.setFont(u8g2_font_5x8_tf);
  display_.setCursor(0, 7);
  if (leadOff) {
    display_.print("LEADS OFF");
  } else {
    display_.print("BPM:");
    display_.print(bpm);
  }

  const uint16_t newest = (waveHead + Config::WAVE_BUFFER_SIZE - 1) % Config::WAVE_BUFFER_SIZE;
  const float compress = static_cast<float>(Config::HORIZONTAL_COMPRESS);
  const float totalSpan = static_cast<float>(Config::SCREEN_W - 1) * compress;
  const uint16_t maxSpan = Config::WAVE_BUFFER_SIZE - 1;
  const float span = totalSpan > static_cast<float>(maxSpan) ? static_cast<float>(maxSpan) : totalSpan;

  uint8_t centerY[Config::SCREEN_W];
  uint8_t minY[Config::SCREEN_W];
  uint8_t maxY[Config::SCREEN_W];

  for (uint8_t x = 0; x < Config::SCREEN_W; x++) {
    float binStart = span - static_cast<float>(x) * compress;
    sampleBinMinMax(wave, newest, binStart, compress, &minY[x], &maxY[x], &centerY[x]);
  }

  for (uint8_t x = 0; x < Config::SCREEN_W - 1; x++) {
    display_.drawLine(x, centerY[x], x + 1, centerY[x + 1]);
    uint8_t yTop = minY[x];
    uint8_t yBottom = maxY[x];
    if (yBottom > yTop) {
      display_.drawVLine(x, yTop, static_cast<uint8_t>(yBottom - yTop + 1));
    }
  }

  if (beatTriggered) {
    display_.drawBox(Config::SCREEN_W - 6, 0, 6, 6);
  }

  display_.sendBuffer();
}
