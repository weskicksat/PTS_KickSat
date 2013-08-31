#include <TI_USCI_I2C_master.h>
#include <utility/ITG3200_Config.h>
#include <string.h>
#include "SpriteGyroEx.h"
#include "I2CUser.h"

SpriteGyroEx::SpriteGyroEx() {
  bias.temperature = bias.x = bias.y = bias.z = 0;
}

SpriteGyroEx::SpriteGyroEx(GyroData biasToUse) {
  bias = biasToUse;
}

void SpriteGyroEx::init() {
  // Ensure the I2C buffer is large enough
  #if I2C_SEND_BUFFER_SIZE < 2
  #error
  #endif
  //Sample rate divider registers
  i2c.sendBuffer[0] = SMPL_RATE_REG_ADDR;
  i2c.sendBuffer[1] = GYRO_SAMPLE_RATE;				

  TI_USCI_I2C_transmitinit(GYRO_ADDRESS, I2C_PRESCALE);
  TI_USCI_I2C_transmit(i2c.sendBuffer, 2); //write to register

  i2c.sendBuffer[0] = DLPF_RANGE_REG_ADDR; //Sample rate divider registers
  i2c.sendBuffer[1] = GYRO_RANGE|GYRO_FILTER_SMPL_RATE; //sample rate divider relative to internal sampling

  TI_USCI_I2C_transmit(i2c.sendBuffer, 2); //write to DLPF and range register
}

GyroData SpriteGyroEx::read() {
  // Ensure the I2C buffer is large enough
  #if I2C_RECEIVE_BUFFER_SIZE < 8
  #error
  #endif

  i2c.sendBuffer[0] = 0x1B; //point to first gyro data register
  TI_USCI_I2C_transmitinit(GYRO_ADDRESS, I2C_PRESCALE);
  TI_USCI_I2C_transmit(i2c.sendBuffer, 1);

  memset(i2c.receiveBuffer,0,8);
  TI_USCI_I2C_receiveinit(GYRO_ADDRESS, I2C_PRESCALE); //receiving 8 bytes
  TI_USCI_I2C_receive(i2c.receiveBuffer, 8);

  GyroData output;
  output.temperature = ((int16)i2c.receiveBuffer[0]<<8) + (int16)i2c.receiveBuffer[1]; // + bias.temperature;
  output.x = ((int16)i2c.receiveBuffer[2]<<8) + (int16)i2c.receiveBuffer[3]; // + bias.x;
  output.y = ((int16)i2c.receiveBuffer[4]<<8) + (int16)i2c.receiveBuffer[5]; // + bias.y;
  output.z = ((int16)i2c.receiveBuffer[6]<<8) + (int16)i2c.receiveBuffer[7]; // + bias.z;

  return output;
}

