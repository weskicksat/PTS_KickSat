// Wesley Faler KickSat Sprite code
// v.2013-08-30 final
// by Wesley Faler, Don Smith, and The Part-Time Scientists GLXP Team

#include <stdlib.h>
#include <string.h>
#include "Flags.h"
#include <SpriteRadio.h>
#include <SpriteMag.h>
#include <SpriteGyro.h>
#include "SpriteGyroEx.h"
#include "SpriteMagEx.h"
#include "HardwareInterface.h"
#include "I2CUser.h"
#include "Encoder.h"

unsigned char prn0[80] = {
  0b00111010, 0b00010010, 0b01111101, 0b10011010, 0b01010000, 0b10111011, 0b10101101, 0b10100111,
  0b01100110, 0b00100011, 0b01010011, 0b01001101, 0b10011110, 0b01110100, 0b00010100, 0b11101110,
  0b11010101, 0b00111110, 0b10000111, 0b00111101, 0b11101010, 0b01111111, 0b11101001, 0b01100001,
  0b00010001, 0b01100111, 0b10000000, 0b11100011, 0b11101101, 0b00101110, 0b10010000, 0b11100001,
  0b11000101, 0b11111101, 0b10010010, 0b10000001, 0b00100000, 0b11010100, 0b01001000, 0b11000001,
  0b00000110, 0b00100100, 0b01010110, 0b00001001, 0b00000010, 0b10010011, 0b01111111, 0b01000111,
  0b00001110, 0b00010010, 0b11101001, 0b01101111, 0b10001110, 0b00000011, 0b11001101, 0b00010001,
  0b00001101, 0b00101111, 0b11111100, 0b10101111, 0b01111001, 0b11000010, 0b11111001, 0b01010110,
  0b11101110, 0b01010000, 0b01011100, 0b11110011, 0b01100101, 0b10010101, 0b10001000, 0b11001101,
  0b11001011, 0b01101011, 0b10111010, 0b00010100, 0b10110011, 0b01111100, 0b10010000, 0b10111001
};

unsigned char prn1[80] = {
  0b01100010, 0b00101010, 0b11010000, 0b01000010, 0b10010001, 0b00011110, 0b00111111, 0b11010011,
  0b11101110, 0b01011000, 0b01101000, 0b01011111, 0b10110110, 0b11000100, 0b00100101, 0b10000111,
  0b11100110, 0b10010111, 0b01110011, 0b01101111, 0b01110010, 0b11010101, 0b01110101, 0b11100010,
  0b11010010, 0b00010010, 0b01111110, 0b01100110, 0b10000001, 0b01000111, 0b01010001, 0b10011100,
  0b11001000, 0b10101111, 0b10101011, 0b01111101, 0b01011110, 0b00011011, 0b01010110, 0b00111101,
  0b00001110, 0b01010100, 0b10011110, 0b00010101, 0b00000100, 0b10101000, 0b00101011, 0b10110011,
  0b00011001, 0b11010100, 0b01110101, 0b11111010, 0b01100110, 0b00000110, 0b11011110, 0b11010010,
  0b11100001, 0b01000101, 0b01010010, 0b11000100, 0b00100100, 0b11000100, 0b01011010, 0b01100000,
  0b01111001, 0b01101111, 0b01110010, 0b01001000, 0b00010111, 0b10100111, 0b10010110, 0b00100000,
  0b11010000, 0b00001110, 0b00011101, 0b11011010, 0b11110111, 0b11010010, 0b10101110, 0b11100101
};

//Initialize the radio class, supplying the Gold Codes that correspond to 0 and 1
SpriteRadio radio = SpriteRadio(prn0, prn1);

I2CUser i2c = I2CUser();
SpriteGyroEx gyro = SpriteGyroEx();
SpriteMagEx mag = SpriteMagEx();
HardwareInterface hardware = HardwareInterface();
AllData allData;

unsigned long nextSystemCheckTime = 0;
unsigned long nextTransmitTime = 0;

#define MS_BETWEEN_SYSTEM_CHECKS (30000L)
#define MS_BETWEEN_PACKETS (5000L)

/*
System state is stored in Flash. However, given that Flash is subject to radiation effects, critical
flags for the system state (such as the boot count) are redundantly stored.

Upon boot in setup():
  Read system status from Flash into allData.
  Vote to get true boot count.
  If voting is deadlocked, jump to "First boot(isCorrupt=true)".
  If there is a disagreement but the majority rules
    If boot count is 0, jump to "First boot(isCorrupt=true)" (which will overwrite the corrupt boot counts).
    Else, jump to "Normal boot(isCorrupt=true)"
  Else
    If boot count is 0, jump to "First boot(isCorrupt=false)"
    Else, jump to "Normal boot(isCorrupt=false)"
  
First boot(isCorrupt):
  Set boot count to one.
  Initialize all counts, durations, and radiation patterns.
  If isCorrupt, set radiationCounter to one.
  Generate a checksum of allData and store it to allData.
  Store allData into Flash.
  Enter normal loop.

Normal boot(isCorrupt):
  If isCorrupt, set increment-with-limit radiationCounter.
  Else
    Generate a checksum of allData.
    Compare the checksum with that stored in allData.
    If there was a change,
      increment-with-limit radiationCounter.
  Increment-with-limit boot count.
  Generate a checksum of allData and store it to allData.
  Store allData into Flash.
  Enter normal loop.

At end of Setup:
  Set the random seed using the radio PRNs, the boot count, and the radiation counter

Normal loop:
  If it is time for a system check:
    Determine the next time for a system check.
    Scan for radiation effects and bad checksums, increment-with-limit radiationCounter if an error is found.
    Update the current uptime.
    Checksum and store allData to Flash.

  If it is not time to transmit another packet:
    Pause briefly.
    Jump back to the top of Normal loop.

  If there are any data sets that are ready to send:
    Clear the transmit timer.
    Pick a data set at random. Continue choosing until one of the unsent data sets is selected.
    Mark the data set as transmitted. This is done before transmission so there are no infinite retries on data.
    Checksum and store allData to Flash.
    If the data set's definition is active
      Encode the data set, randomly selecting the reversal flag.
      Transmit the encoded data.
      Set the transmit timer to send the next packet in the future.
    Jump back to the top of Normal loop.

  There are no data sets ready to send, so collect all data points.
  Mark all data sets as ready to be sent.
    (This is done before collecting data so that if data collection always fails, the prior data will be sent.)
  Read sensors, filling allData.
*/


void setup() {
#ifdef USE_SERIAL
  Serial.begin(9600);
  //Serial.println("setup");
#endif
  // Read system status from Flash into allData.
  hardware.init();
  memset(&allData,0,sizeof(allData));
  hardware.readStoredData(allData);
  // Vote to get true boot count.
  int bootCount = allData.bootCount[0];
  int bootCountCount = 1;
  if(allData.bootCount[0] == allData.bootCount[1] && allData.bootCount[0] == allData.bootCount[2]) {
    bootCountCount = 3;
  } else if(allData.bootCount[0] == allData.bootCount[1] || allData.bootCount[0] == allData.bootCount[2]) {
    bootCountCount = 2;
    bootCount = allData.bootCount[0];
  } else if(allData.bootCount[1] == allData.bootCount[2]) {
    bootCountCount = 2;
    bootCount = allData.bootCount[1];
  }
  // If voting is deadlocked, jump to "First boot(isCorrupt=true)".
  // If there is a disagreement but the majority rules
  //   If boot count is 0, jump to "First boot(isCorrupt=true)" (which will overwrite the corrupt boot counts).
  //   Else, jump to "Normal boot(isCorrupt=true)"
  // Else
  //   If boot count is 0, jump to "First boot(isCorrupt=false)"
  //   Else, jump to "Normal boot(isCorrupt=false)"
  boolean isCorrupt = false;
  boolean isFirstBoot = false;
  if(1 == bootCountCount) { // Voting is deadlocked
    isCorrupt = true;
    isFirstBoot = true;
  } else if(2 == bootCountCount) { // There is disagreement but majority rules
    isCorrupt = true;
    isFirstBoot = 0 == bootCount;
  } else {
    isCorrupt = false;
    isFirstBoot = 0 == bootCount;
  }

  // First boot(isCorrupt):
  if(isFirstBoot) {
    // Initialize all counts, durations, and radiation patterns.
    hardware.initAllData(allData);
    // Set boot count to one.
    allData.bootCount[0] = allData.bootCount[1] = allData.bootCount[2] = bootCount = 1;
    // If isCorrupt, set radiationCounter to one.
    if(isCorrupt) {
      allData.radiationCount = 1;
    }
    // Store allData into Flash, generating a checksum in the process.
    hardware.store(allData);
    // Enter normal loop.
  } else { // Normal boot (isCorrupt)
    //  If isCorrupt, set increment-with-limit radiationCounter.
    if(isCorrupt) {
      if(allData.radiationCount < 0x7ffe) {
        allData.radiationCount++;
      }
    } else { // Boot count is not corrupt
      // Generate a checksum of allData.
      uint32 checksum = hardware.calculateChecksum(allData);
      // Compare the checksum with that stored in allData.
      if(checksum != allData.allDataChecksum) { // If there was a change,
        // increment-with-limit radiationCounter.
        if(allData.radiationCount < 0x7fff) {
          allData.radiationCount++;
        } else {
          allData.radiationCount = 0;
        }
      }
    }
    // Increment-with-limit the boot count
    if(bootCount < 0x7fff) {
      bootCount++;
    } else {
      bootCount = 1;
    }
    allData.bootCount[0] = allData.bootCount[1] = allData.bootCount[2] = bootCount;
    // Store allData into Flash, generating a checksum in the process.
    hardware.store(allData);
    // Enter normal loop.
  }

  // Set the random seed using the radio PRNs, the boot count, and the radiation counter.
  // The radio's init() function also sets the random seed, but it should be different with
  // each boot so we don't send the same sequence of packets.
  randomSeed(((int)prn0[0]) + ((int)prn1[0]) + ((int)prn0[1]) + ((int)prn1[1]) + allData.bootCount[0] + allData.radiationCount);
}

void loop() {
#ifdef USE_SERIAL
  //Serial.println("loop");
#endif
  if(nextSystemCheckTime <= millis()) { // If it is time for a system check
#ifdef USE_SERIAL
    //Serial.println("time for a system check");
#endif
    // Determine the next time for a system check.
    nextSystemCheckTime = MS_BETWEEN_SYSTEM_CHECKS + millis();
    // Scan for radiation effects and bad checksums, increment-with-limit radiationCounter if an error is found.
    boolean foundError = false;
    uint32 checksum = hardware.calculateChecksum(allData);
    if(checksum != allData.allDataChecksum) {
      foundError = true;
    }
    // Check for radiation changes, even if the checksum failed since checking also fixes the errors
    if(hardware.countErrorsInRadiationPattern(allData.radiationCheckBytes)) {
      foundError = true;
    }
    // increment-with-limit radiationCounter.
    if(foundError) {
      if(allData.radiationCount < 0x7fff) {
        allData.radiationCount++;
      } else {
        allData.radiationCount = 0;
      }
    }
    // Update the current uptime.
    hardware.setDuration(allData,millis());
    hardware.calculateUptimeStatistics(allData);
    // Checksum and store allData to Flash.
    hardware.store(allData);
    // Continue on
  }

  if(nextTransmitTime > millis()) { // If it is not time to transmit another packet:
#ifdef USE_SERIAL
    //Serial.println("not time to transmit");
#endif
    // Pause briefly.
    globalDelay(250);
    // Jump back to the top of Normal loop.
    return;
  }

  // Check for data sets that are ready to send
  int dataSetsReady = 0;
  for(int i=0;i<NUMBER_OF_DATA_SETS;i++) {
    if(allData.readyToSend[i]) {
      dataSetsReady++;
    }
  }

  if(dataSetsReady > 0) { // If there are any data sets that are ready to send:
#ifdef USE_SERIAL
    //Serial.println("dataset is ready");
#endif
    // Clear the transmit timer.
    nextTransmitTime = 0;
    // Pick a data set at random. Continue choosing until one of the unsent data sets is selected.
    int dataSetToUse = random(dataSetsReady);
    for(int i=0;i<NUMBER_OF_DATA_SETS;i++) {
      if(allData.readyToSend[i]) {
        if(dataSetToUse == 0) {
          dataSetToUse = i; // convert to an actual index
          break;
        } else {
          dataSetToUse--;
        }
      }
    }
    // Mark the data set as transmitted. This is done before transmission so there are no infinite retries on data.
    allData.readyToSend[dataSetToUse] = false;
    // Checksum and store allData to Flash.
    hardware.store(allData);
    if(dataSetToUse < 6 || 7 == dataSetToUse) { // If the data set's definition is active
      // Encode the data set, randomly selecting the reversal flag.
#ifdef USE_SERIAL
      //Serial.println("encoding");
#endif
      Encoder encoder;
      if(encoder.encode(allData,dataSetToUse,random(100)>=40) > 0? 1 : 0) {
#ifdef USE_SERIAL
        /*
        Serial.println("transmitting");
        Serial.print("bc:"); Serial.print(allData.bootCount[0]); 
        Serial.print(" bx:"); Serial.print(allData.bAvgX);
        Serial.print(" by:"); Serial.print(allData.bAvgY);
        Serial.print(" bz:"); Serial.print(allData.bAvgZ);
        Serial.print(" gx:"); Serial.print(allData.gAvgX);
        Serial.print(" gy:"); Serial.print(allData.gAvgY);
        Serial.print(" gz:"); Serial.print(allData.gAvgZ);
        Serial.print(" t:"); Serial.print(allData.temperature);
        Serial.print(" bN:"); Serial.print(allData.bAvgNorm);
        Serial.print(" gN:"); Serial.print(allData.gAvgNorm);
        Serial.print(" bS:"); Serial.print(allData.bStdDev);
        Serial.print(" gS:"); Serial.print(allData.gStdDev);
        Serial.print(" bST:"); Serial.print(allData.bStdDevTemp);
        Serial.print(" gST:"); Serial.print(allData.gStdDevTemp);
        Serial.println(":");
        */
#endif
        // Transmit the encoded data.
        hardware.radioTransmit(encoder.buffer,encoder.encodedDataSize);
#ifdef USE_SERIAL
        //Serial.println("transmitted");
        /*
        // Clear all data flags so the sensors will be read again immediately during testing
        for(int i=0;i<NUMBER_OF_DATA_SETS;i++) {
          allData.readyToSend[i] = false;
        }
        */
#endif
        // Set the transmit timer to send the next packet in the future.
        nextTransmitTime = MS_BETWEEN_PACKETS + millis();
      }
    }
    // Jump back to the top of Normal loop.
    return;
  }

  // There are no data sets ready to send, so collect all data points.
#ifdef USE_SERIAL
  //Serial.println("No data sets are ready");
#endif
  // Mark all data sets as ready to be sent.
  //   (This is done before collecting data so that if data collection always fails, the prior data will be sent.)
  for(int i=0;i<NUMBER_OF_DATA_SETS;i++) {
    allData.readyToSend[i] = true;
  }
  // Read sensors, filling allData.
  readAllSensors();
}

void globalDelay(unsigned long milliseconds) {
  delay(milliseconds);
}

int globalRandom(int minValue, int maxValue) {
  return (int)random(minValue,maxValue);
}

int globalRandom(int maxValue) {
  return (int)random(maxValue);
}

#ifdef USE_DEBUG_TRANSMIT
#ifdef READY_FOR_FLIGHT
#error
#endif
void globalWrite(byte value) {
  Serial.write(value);
  //Serial.print("transmit ");
  //Serial.println(value,HEX);
}

void globalPrint(const char *text) {
  Serial.print(text);
}

void globalPrintln(const char *text) {
  Serial.println(text);
}

void globalPrintln(unsigned int value) {
  Serial.println(value,HEX);
}
#endif

#define NUMBER_OF_READINGS (10)

void readAllSensors() {
  // Incremental computation of mean and variance. Covariance as well.
  // http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
  float mx = 0, my = 0, mz = 0, gx = 0, gy = 0, gz = 0, temp = 0, mn = 0, gn = 0;
  float mn2 = 0, gn2 = 0;
  float n = 0, delta, t;
  SensorFrame frame;
  for(int i=0;i<NUMBER_OF_READINGS;i++) {
    hardware.readLiveData(frame);
    n = i+1;
    // Track the magnetic field components
    delta = frame.b.x - mx; mx += delta/n;
    delta = frame.b.y - my; my += delta/n;
    delta = frame.b.z - mz; mz += delta/n;
    // Track the magnitude of the magnetic field and its noise
    t = sqrt((float)frame.b.x*(float)frame.b.x + (float)frame.b.y*(float)frame.b.y + (float)frame.b.z*(float)frame.b.z);
    delta = t - mn; mn += delta/n;
    mn2 += delta * (t - mn);

    // Track the gyro components
    delta = frame.g.x - gx; gx += delta/n;
    delta = frame.g.y - gy; gy += delta/n;
    delta = frame.g.z - gz; gz += delta/n;
    // Track the magnitude of the gyro rate and its noise
    t = sqrt((float)frame.g.x*(float)frame.g.x + (float)frame.g.y*(float)frame.g.y + (float)frame.g.z*(float)frame.g.z);
    delta = t - gn; gn += delta/n;
    gn2 += delta * (t - gn);

    // Track the temperature
    frame.g.temperature = (frame.g.temperature >> 8) & 0xff;
    delta = frame.g.temperature - temp; temp += delta/n;
  }
  // Store the readings
  allData.bAvgX = (magInt12)mx;
  allData.bAvgY = (magInt12)my;
  allData.bAvgZ = (magInt12)mz;

  allData.gAvgX = (int16)gx;
  allData.gAvgY = (int16)gy;
  allData.gAvgZ = (int16)gz;

  allData.temperature = (byte)temp;

  if(n <= 1) {
    n = 1;
  } else {
    n -= 1;
  }
  allData.bAvgNorm = (magInt12)mn;
  allData.bStdDev = sqrt(mn2/n); // "n-1" is performed above to handle low signal count case

  allData.gAvgNorm = (int16)gn;
  allData.gStdDev = sqrt(gn2/n); // "n-1" is performed above to handle low signal count case

  NoiseTemperaturePoint mnt, gnt;
  mnt.temperature = allData.temperature;
  mnt.stddev = allData.bStdDev;
  gnt.temperature = allData.temperature;
  gnt.stddev = allData.gStdDev;
  hardware.addNoiseData(allData,mnt,gnt);
  
  allData.bStdDevTemp = hardware.calculateCovariance(allData.magNoiseTemperature,allData.noisePointCount);
  allData.gStdDevTemp = hardware.calculateCovariance(allData.gyroNoiseTemperature,allData.noisePointCount);
}

