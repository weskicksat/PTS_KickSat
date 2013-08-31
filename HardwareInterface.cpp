#include <string.h>
#include <SpriteRadio.h>
#include "CCFlash.h"
#include "HardwareInterface.h"

extern SpriteRadio radio;
extern SpriteMagEx mag;
extern SpriteGyroEx gyro;

void globalDelay(unsigned long milliseconds);
int globalRandom(int minValue, int maxValue);
int globalRandom(int maxValue);

#ifdef USE_DEBUG_TRANSMIT
#ifdef READY_FOR_FLIGHT
#error
#endif
void globalWrite(byte value);
#endif


// Radiation bit pattern is 10101100
#define RADIATION_BIT_PATTERN (0xAC)

// return segment pointer inside variable. Note: return start of NEXT segment
#define SEGPTR(x) ( (unsigned char *) (((unsigned short)(&x)+512) & 0xFE00) )

static const unsigned char flash[3*512] = { 0 };

HardwareInterface::HardwareInterface() {
}

void HardwareInterface::init() {
  radio.txInit();
  i2cDelay();
  mag.init();
  i2cDelay();
  gyro.init();
  i2cDelay();
}

void HardwareInterface::readLiveData(SensorFrame &frame) {
  i2cDelay();
  frame.b = mag.read();
  i2cDelay();
  frame.g = gyro.read();
}

void HardwareInterface::readStoredData(AllData &allData) {
  memset(&allData,0,sizeof(allData));
#ifdef USE_FLASH
  Flash.read(SEGPTR(flash),(unsigned char *)&allData,sizeof(allData));
#endif
}

void HardwareInterface::store(AllData &allData) {
  allData.allDataChecksum = calculateChecksum(allData);
#ifdef USE_FLASH
  Flash.erase(SEGPTR(flash));
  Flash.write(SEGPTR(flash),(unsigned char *)&allData,sizeof(allData));
#endif
}

void HardwareInterface::i2cDelay() {
  globalDelay(2);
  //delay(1);
  //while ( TI_USCI_I2C_notready() ); // wait for bus to be free
}

void HardwareInterface::radioTransmit(const unsigned char *data, int dataLength) {
  i2cDelay();
#ifndef USE_DEBUG_TRANSMIT
#ifndef READY_FOR_FLIGHT
#error
#endif
  radio.transmit((char *)data, dataLength);
#else
#ifdef READY_FOR_FLIGHT
#error
#endif
  // Send the bytes using Serial using the same delays used by the radio.transmit() function.
  globalDelay(1000 + globalRandom(-500,500));
  for(int i = 0; i < dataLength; i++) {
    globalWrite(data[i]);
    globalDelay(1000 + globalRandom(-500,500));
  }
#endif
}

void HardwareInterface::initAllData(AllData &allData) {
  // Zero all integers and memory blocks
  memset(&allData,0,sizeof(allData));

  // Setup radiation check bytes
  memset(allData.radiationCheckBytes,RADIATION_BIT_PATTERN,sizeof(allData.radiationCheckBytes));
}

int HardwareInterface::countErrorsInRadiationPattern(byte radiationCheckBytes[NUMBER_OF_RADIATION_FLASH_BYTES]) {
  int count = 0;
  for(int i=0;i<NUMBER_OF_RADIATION_FLASH_BYTES;i++) {
    if(radiationCheckBytes[i] != RADIATION_BIT_PATTERN) {
      radiationCheckBytes[i] = RADIATION_BIT_PATTERN;
      count++;
    }
  }
  return count;
}

void HardwareInterface::addNoiseData(AllData &allData, NoiseTemperaturePoint &mag, NoiseTemperaturePoint &gyro) {
  int slot = allData.noisePointCount % NUMBER_OF_NOISE_TEMP_POINTS;
  allData.gyroNoiseTemperature[slot] = gyro;
  allData.magNoiseTemperature[slot] = mag;

  // Once the buffer is full, cycle around to the beginning. However, keep track
  // of the fact that the buffer is in fact full. By setting the count to at least
  // the buffer size, the modulus index remains intact but so does the fact that
  // the buffer is full, all without consuming more memory for another flag.
  allData.noisePointCount++;
  if(allData.noisePointCount >= 2*NUMBER_OF_NOISE_TEMP_POINTS) {
    allData.noisePointCount = NUMBER_OF_NOISE_TEMP_POINTS;
  }
}

void HardwareInterface::setDuration(AllData &allData, unsigned long nowMillis) {
  int bootCount = allData.bootCount[0];
  if(bootCount < 0) {
    bootCount = 0;
  }
  int slot = bootCount % NUMBER_OF_BOOTS;
  unsigned long t = nowMillis / 1000;
  if(t > 32767) {
    t = 32767;
  }
  allData.bootDurations[slot] = (uint16)t;
}

uint32 HardwareInterface::calculateChecksum(byte *data, int count) {
  // From http://en.wikipedia.org/wiki/Adler32
  // A basic, but slow version is used to conserve RAM.
#define MOD_ADLER (65521)
  uint32 a = 1, b = 0;
  // Process each byte of the data in order
  for(int index = 0; index < count; index++) {
      a = (a + data[index]) % MOD_ADLER;
      b = (b + a) % MOD_ADLER;
  }
  return (b << 16) | a;
#undef MOD_ADLER
}

uint32 HardwareInterface::calculateChecksum(AllData &allData) {
  return calculateChecksum((byte *)&allData,((unsigned char *)&allData.allDataChecksum) - (unsigned char *)&allData);
}

int16 HardwareInterface::calculateCovariance(NoiseTemperaturePoint points[NUMBER_OF_NOISE_TEMP_POINTS], int count) {
  if(count <= 0) {
    return 0;
  }
  if(count > NUMBER_OF_NOISE_TEMP_POINTS) {
    count = NUMBER_OF_NOISE_TEMP_POINTS;
  }
  float meanT = 0, meanS = 0;
  for(int i=0;i<count;i++) {
    meanT += points[i].temperature;
    meanS += points[i].stddev;
  }
  meanT /= count;
  meanS /= count;
  
  float covariance = 0, a, b;
  for(int i=0;i<count;i++) {
    a = points[i].temperature - meanT;
    b = points[i].stddev - meanS;
    covariance += a*b/count;
  }
  return (int16)(covariance*100);
}

void HardwareInterface::calculateUptimeStatistics(AllData &allData) {
  int bootCount = allData.bootCount[0];
  if(bootCount < 0) {
    bootCount = 0;
  }
  uint16 upTimeSum = 0, n = 0, t;
  for(int i=0;i<bootCount && i<NUMBER_OF_BOOTS;i++,n++) {
    t = allData.bootDurations[i % NUMBER_OF_BOOTS];
    if(i == 0) {
      allData.upTimeMin = allData.upTimeMax = allData.upTimeAvg = upTimeSum = t;
    } else {
      upTimeSum += t;
      if(allData.upTimeMin > t) {
        t = allData.upTimeMin;
      }
      if(allData.upTimeMax < t) {
        t = allData.upTimeMax;
      }
    }
  }
  if(n > 0) {
    allData.upTimeAvg = upTimeSum / n;
  }
}


