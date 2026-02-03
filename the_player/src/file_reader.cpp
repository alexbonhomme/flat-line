#include "file_reader.h"

#define SD_CS_PIN 0 // Adafruit microSD Card BFF (TX/GPIO0 on XIAO RP2040)
#define SD_FAT_TYPE 3
// #define SD_CS_PIN A2 // Adafruit Audio BFF
#define SPI_CLOCK SD_SCK_MHZ(50)
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)

FileReader::FileReader() : dirOpen_(false), initialized_(false) {}

bool FileReader::begin() {
#if DEBUG
  // debug output at 115200 baud
  Serial.begin(115200);
  while (!Serial)
    delay(100);

  // setup SD-card
  Serial.print("Initializing SD card...");
#endif

  if (!sd_.begin(SD_CONFIG)) {
#if DEBUG
    Serial.println(" failed!");
#endif
    initialized_ = false;
    return false;
  }

#if DEBUG
  Serial.println(" done.");
#endif
  initialized_ = true;
  return true;
}

bool FileReader::nextWav() {
  if (!initialized_) {
    return false;
  }

  if (!dirOpen_) {
    if (!dir_.open("/")) {
      return false;
    }
    dirOpen_ = true;
  }

  while (file_.openNext(&dir_, O_RDONLY)) {
    char filename[EXFAT_MAX_NAME_LENGTH + 1];
    file_.getName(filename, sizeof filename);

#if DEBUG
    Serial.println(filename);
#endif

    size_t nameLen = strlen(filename);
    if (file_.isFile() && (filename[0] != '.') && nameLen >= 4 &&
        !strcasecmp(&filename[nameLen - 4], ".wav")) {
      return true;
    }

    file_.close();
  }

  dir_.close();
  dirOpen_ = false;
  return false;
}

File &FileReader::currentFile() { return file_; }

void FileReader::closeCurrent() { file_.close(); }
