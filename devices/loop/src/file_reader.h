#pragma once

#include "SdFat.h"

#if defined(ARDUINO_ARCH_RP2040)
#define File FatFile
#endif

class FileReader
{
public:
  FileReader();

  bool begin();
  bool nextWav();
  File &currentFile();
  void closeCurrent();

private:
  SdFat sd_;
  File dir_;
  File file_;
  bool dirOpen_;
  bool initialized_;
};
