#ifndef SpriteGyroEx_h
#define SpriteGyroEx_h

#include "Flags.h"

typedef struct sGyroData {
  int16 temperature; // Temp(Celcius) = (temperature+13200)/280.0+35.0
  int16 x;
  int16 y;
  int16 z;
} GyroData;

class SpriteGyroEx {
public:
  SpriteGyroEx();
  SpriteGyroEx(GyroData biasToUse);
	
  void init();

  // Read angular rate and temperature from gyro
  GyroData read();

private:
  GyroData bias;
};

#endif

