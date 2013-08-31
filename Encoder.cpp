#include <string.h>
#include "Encoder.h"

#ifdef USE_SERIAL
void globalPrint(const char *text);
void globalPrintln(const char *text);
void globalPrintln(unsigned int value);
#endif

Encoder::Encoder()
  : encodedDataSize(0) {
  // Fill the buffer with a pattern that can be used by debuggers to spot un
  for(int i=0;i<ENCODED_BUFFER_SIZE;i++) {
    buffer[i] = 255-i;
  }
}

int Encoder::encode(AllData &allData, byte dataSetID, byte reverseBits) {
  encodedDataSize = 0;
  memset(buffer,0,sizeof(buffer)); // all data starts blank
  if(!(dataSetID < 6 || 7 == dataSetID)) {
    return encodedDataSize;
  }
  cursorBit = 0x80;
  cursorByte = 0;
  if(reverseBits > 0 && 7 != dataSetID) {
    append(6 << 5, 3); // send the bit reversal flag
#ifdef USE_DEBUG_TRANSMIT
    //globalPrint("After reverse bits: ");
    //globalPrintln((uint16)buffer[0]);
#endif
  }
  append(dataSetID << 5, 3); // send the lowest 3 bits of dataSetID
#ifdef USE_DEBUG_TRANSMIT
  //globalPrint("Data set: ");
  //globalPrintln((uint16)dataSetID);
  //globalPrint("After data set: ");
  //globalPrintln((uint16)buffer[0]);
#endif

  switch(dataSetID) {
    case 0: // 0(48): gAvgX[16], gAvgY[16], gAvgZ[16]
      if(reverseBits) {
        appendReverse((uint16)allData.gAvgX,16,(uint16)allData.gAvgY,16,(uint16)allData.gAvgZ,16,0,0);
      } else {
        append((uint16)allData.gAvgX,16,(uint16)allData.gAvgY,16,(uint16)allData.gAvgZ,16,0,0);
      }
      break;
    case 1: // 1(36): bAvgX[12], bAvgX[12], bAvgZ[12]
      if(reverseBits) {
        appendReverse((uint16)allData.bAvgX,12,(uint16)allData.bAvgY,12,(uint16)allData.bAvgZ,12,0,0);
      } else {
        append((uint16)(allData.bAvgX << 4),12,(uint16)(allData.bAvgY << 4),12,(uint16)(allData.bAvgZ << 4),12,0,0);
      }
      break;
    case 2: // 2(40): temperature[8], bootCount[16], bootDuration[16]
      if(reverseBits) {
        appendReverse((uint16)allData.temperature,8,(uint16)allData.bootCount[0],16,(uint16)allData.bootDurations[allData.bootCount[0]%NUMBER_OF_BOOTS],16,0,0);
      } else {
        append(((uint16)allData.temperature)<<8,8,(uint16)allData.bootCount[0],16,(uint16)allData.bootDurations[allData.bootCount[0]%NUMBER_OF_BOOTS],16,0,0);
      }
      break;
    case 3: // 3(64): gStdDev[16], bStdDev[16], gStdDevTemp[16], bStdDevTemp[16]
      if(reverseBits) {
        appendReverse((uint16)allData.gStdDev,16,(uint16)allData.bStdDev,16,(uint16)allData.gStdDevTemp,16,(uint16)allData.bStdDevTemp,16);
      } else {
        append((uint16)allData.gStdDev,16,(uint16)allData.bStdDev,16,(uint16)allData.gStdDevTemp,16,(uint16)allData.bStdDevTemp,16);
      }
      break;
    case 4: // 4(64): upMin[16], upMax[16], upAvg[16], radiationCount[16]
      if(reverseBits) {
        appendReverse((uint16)allData.upTimeMin,16,(uint16)allData.upTimeMax,16,(uint16)allData.upTimeAvg,16,(uint16)allData.radiationCount,16);
      } else {
        append((uint16)allData.upTimeMin,16,(uint16)allData.upTimeMax,16,(uint16)allData.upTimeAvg,16,(uint16)allData.radiationCount,16);
      }
      break;
    case 5: // 5(28): gAvgNorm[16], bAvgNorm[12]
      if(reverseBits) {
        appendReverse((uint16)allData.gAvgNorm,16,(uint16)allData.bAvgNorm,12,0,0,0,0);
      } else {
        append((uint16)allData.gAvgNorm,16,(uint16)(allData.bAvgNorm << 4),12,0,0,0,0);
      }
      break;
    case 7: // 7: "UnitLNE\n"
      // Give thanks to Keith Laumer's Bolo series, especially the "The Last Command" story.
      // It was the first one I read as a child, getting it in a great sci-fi short story book
      // from the RIF (Reading is Fundamental) program in 5th grade. Unit LNE of the Line is
      // a self-aware tank that was retired after a great war. It awakes desparate to complete
      // its mission, is always short on power, has almost no sensors, and still keeps trying.
      // Unit LNE, thank you. You stand relieved.
      // http://hell.pl/szymon/Baen/The%20best%20of%20Jim%20Baens%20Universe/The%20World%20Turned%20Upside%20Down/0743498747__14.htm
      append((byte)0,5);
      append((byte)'U',8);
      append((byte)'n',8);
      append((byte)'i',8);
      append((byte)'t',8);
      append((byte)'L',8);
      append((byte)'N',8);
      append((byte)'E',8);
      append((byte)'\n',8);
      break;
    default:
      break;
  }
#ifdef USE_DEBUG_TRANSMIT
  //globalPrint("cursorBit: ");
  //globalPrintln(cursorBit);
  //globalPrint("cursorByte: ");
  //globalPrintln(cursorByte);
#endif
  if(0x80 == cursorBit) {
    encodedDataSize = cursorByte;
  } else {
    encodedDataSize = 1+cursorByte;
  }
  return encodedDataSize;
}

void Encoder::append(byte value, int bitCount) {
  uint16 newValue = value;
  append(newValue<<8,bitCount,0,0,0,0,0,0);
}

void Encoder::append(uint16 value1, int bitCount1, uint16 value2, int bitCount2, uint16 value3, int bitCount3, uint16 value4, int bitCount4) {
  uint16 mask = 0x8000;
  while(bitCount1 > 0 || bitCount2 > 0 || bitCount3 > 0 || bitCount4 > 0) {
    if(bitCount1 > 0) {
      bitCount1--;
      if(value1 & mask) {
        buffer[cursorByte] |= cursorBit;
      }
      if(cursorBit == 1) {
        cursorBit = 0x80;
        cursorByte++;
      } else {
        cursorBit >>= 1;
      }
    }

    if(bitCount2 > 0) {
      bitCount2--;
      if(value2 & mask) {
        buffer[cursorByte] |= cursorBit;
      }
      if(cursorBit == 1) {
        cursorBit = 0x80;
        cursorByte++;
      } else {
        cursorBit >>= 1;
      }
    }

    if(bitCount3 > 0) {
      bitCount3--;
      if(value3 & mask) {
        buffer[cursorByte] |= cursorBit;
      }
      if(cursorBit == 1) {
        cursorBit = 0x80;
        cursorByte++;
      } else {
        cursorBit >>= 1;
      }
    }

    if(bitCount4 > 0) {
      bitCount4--;
      if(value4 & mask) {
        buffer[cursorByte] |= cursorBit;
      }
      if(cursorBit == 1) {
        cursorBit = 0x80;
        cursorByte++;
      } else {
        cursorBit >>= 1;
      }
    }

    // Move to the next overall bit on each value
    if(mask == 0x0001) {
      mask = 0x8000;
    } else {
      mask >>= 1;
    }
  }
}

void Encoder::appendReverse(uint16 value1, int bitCount1, uint16 value2, int bitCount2, uint16 value3, int bitCount3, uint16 value4, int bitCount4) {
  uint16 mask = 0x0001;
  while(bitCount1 > 0 || bitCount2 > 0 || bitCount3 > 0 || bitCount4 > 0) {
    if(bitCount4 > 0) {
      bitCount4--;
      if(value4 & mask) {
        buffer[cursorByte] |= cursorBit;
      }
      if(cursorBit == 1) {
        cursorBit = 0x80;
        cursorByte++;
      } else {
        cursorBit >>= 1;
      }
    }

    if(bitCount3 > 0) {
      bitCount3--;
      if(value3 & mask) {
        buffer[cursorByte] |= cursorBit;
      }
      if(cursorBit == 1) {
        cursorBit = 0x80;
        cursorByte++;
      } else {
        cursorBit >>= 1;
      }
    }

    if(bitCount2 > 0) {
      bitCount2--;
      if(value2 & mask) {
        buffer[cursorByte] |= cursorBit;
      }
      if(cursorBit == 1) {
        cursorBit = 0x80;
        cursorByte++;
      } else {
        cursorBit >>= 1;
      }
    }

    if(bitCount1 > 0) {
      bitCount1--;
      if(value1 & mask) {
        buffer[cursorByte] |= cursorBit;
      }
      if(cursorBit == 1) {
        cursorBit = 0x80;
        cursorByte++;
      } else {
        cursorBit >>= 1;
      }
    }

    // Move to the next overall bit on each value
    if(mask == 0x8000) {
      mask = 0x0001;
    } else {
      mask <<= 1;
    }
  }
}

