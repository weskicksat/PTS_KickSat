#ifndef SpriteMagEx_h
#define SpriteMagEx_h

#include "Flags.h"

typedef int16 magInt12;

typedef struct sMagneticFieldRaw {
  magInt12 x; // Only 12 bits are used. Error is -4096
  magInt12 y; // Only 12 bits are used. Error is -4096
  magInt12 z; // Only 12 bits are used. Error is -4096
} MagneticFieldRaw;

class SpriteMagEx {
public:
  SpriteMagEx();

  void init();

  MagneticFieldRaw read();
};

#endif

