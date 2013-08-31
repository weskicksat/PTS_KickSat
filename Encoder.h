#ifndef Encoder_h
#define Encoder_h

#include "Flags.h"
#include "HardwareInterface.h"
#define ENCODED_BUFFER_SIZE (10)

/* Data sets:
0(48): gAvgX[16], gAvgY[16], gAvgZ[16]
1(36): bAvgX[12], bAvgX[12], bAvgZ[12]
2(40): temperature[8], bootCount[16], bootDuration[16]
3(64): gStdDev[16], bStdDev[16], gStdDevTemp[16], bStdDevTemp[16]
4(64): upMin[16], upMax[16], upAvg[16], radiationCount[16]
5(28): gAvgNorm[16], bAvgNorm[12]
7: "UnitLNE\n"

Max bits: 64 + overhead (2x 3-bit data sets at worst case) = 70 bits = 9 bytes
*/

class Encoder {
public:
  byte buffer[ENCODED_BUFFER_SIZE];
  byte encodedDataSize;

public:
  Encoder();
  int encode(AllData &allData, byte dataSetID, byte reverseBits);

protected:
  byte cursorByte, cursorBit;
  
  void append(byte value, int bitCount);

  void append(uint16 value1, int bitCount1, uint16 value2, int bitCount2, uint16 value3, int bitCount3, uint16 value4, int bitCount4);
  void appendReverse(uint16 value1, int bitCount1, uint16 value2, int bitCount2, uint16 value3, int bitCount3, uint16 value4, int bitCount4);
};

#endif

