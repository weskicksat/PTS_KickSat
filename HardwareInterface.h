#ifndef HardwareInterface_h
#define HardwareInterface_h

#include "Flags.h"
#include "SpriteGyroEx.h"
#include "SpriteMagEx.h"

typedef struct sSensorFrame {
  MagneticFieldRaw b;
  GyroData g;
} SensorFrame;

typedef struct sNoiseTempPoint {
  byte temperature;
  int16 stddev;
} NoiseTemperaturePoint;

#define NUMBER_OF_BOOTS (8)
#define NUMBER_OF_RADIATION_FLASH_BYTES (128)
#define NUMBER_OF_NOISE_TEMP_POINTS (20)
#define NUMBER_OF_DATA_SETS (8)

typedef struct sAllData {
  int16 bootCount[3];
  magInt12 bAvgX, bAvgY, bAvgZ;
  int16 gAvgX, gAvgY, gAvgZ;
  byte temperature;
  int16 bStdDev, gStdDev;
  int16 bAvgNorm, gAvgNorm;
  int16 bStdDevTemp, gStdDevTemp; // relate noise to temperature
  int16 radiationCount;
  uint16 bootDurations[NUMBER_OF_BOOTS]; // in seconds.
  byte radiationCheckBytes[NUMBER_OF_RADIATION_FLASH_BYTES];
  NoiseTemperaturePoint gyroNoiseTemperature[NUMBER_OF_NOISE_TEMP_POINTS];
  NoiseTemperaturePoint magNoiseTemperature[NUMBER_OF_NOISE_TEMP_POINTS];
  byte noisePointCount;

  uint32 allDataChecksum;
  byte readyToSend[NUMBER_OF_DATA_SETS];
  uint16 upTimeMin, upTimeMax, upTimeAvg;
} AllData;

class HardwareInterface {
public:
  HardwareInterface();

  void init();
  void readLiveData(SensorFrame &frame);
  void readStoredData(AllData &allData);
  void store(AllData &allData);
  void radioTransmit(const unsigned char *data, int dataLength);
  void i2cDelay();

  void initAllData(AllData &allData);
  int countErrorsInRadiationPattern(byte radiationCheckBytes[NUMBER_OF_RADIATION_FLASH_BYTES]);
  
  void addNoiseData(AllData &allData, NoiseTemperaturePoint &mag, NoiseTemperaturePoint &gyro);
  void setDuration(AllData &allData, unsigned long nowMillis);

  uint32 calculateChecksum(byte *data, int count);
  uint32 calculateChecksum(AllData &allData);
  
  int16 calculateCovariance(NoiseTemperaturePoint points[NUMBER_OF_NOISE_TEMP_POINTS], int count);

  void calculateUptimeStatistics(AllData &allData);
protected:
};

#endif

