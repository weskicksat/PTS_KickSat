#include <TI_USCI_I2C_master.h>
#include <utility/HMC5883L_Config.h>
#include <string.h>
#include "SpriteMagEx.h"
#include "I2CUser.h"

SpriteMagEx::SpriteMagEx() {
}

void SpriteMagEx::init() {
  // Ensure the I2C buffer is large enough
  #if I2C_SEND_BUFFER_SIZE < 2
  #error
  #endif
  i2c.sendBuffer[0] = 0x00;
  i2c.sendBuffer[1] = (MAG_SAMPLES_AVE|MAG_DATA_RATE|MAG_MEAS_MODE);

  TI_USCI_I2C_transmitinit(MAG_ADDRESS, I2C_PRESCALE);
  TI_USCI_I2C_transmit(i2c.sendBuffer, 2);

  i2c.sendBuffer[0] = 0x01; i2c.sendBuffer[1] = MAG_GAIN;
  TI_USCI_I2C_transmit(i2c.sendBuffer, 2);

  i2c.sendBuffer[0] = 0x02; i2c.sendBuffer[1] = MAG_OPER_MODE;
  TI_USCI_I2C_transmit(i2c.sendBuffer, 2);
}

MagneticFieldRaw SpriteMagEx::read() {
  // Ensure the I2C buffer is large enough
  #if I2C_RECEIVE_BUFFER_SIZE < 6
  #error
  #endif

  i2c.sendBuffer[0] = 0x03;

  TI_USCI_I2C_transmitinit(MAG_ADDRESS, I2C_PRESCALE);
  TI_USCI_I2C_transmit(i2c.sendBuffer, 1);

  memset(i2c.receiveBuffer,0,6);
  TI_USCI_I2C_receiveinit(MAG_ADDRESS, I2C_PRESCALE);
  TI_USCI_I2C_receive(i2c.receiveBuffer, 6);

  MagneticFieldRaw b;
  // Note: Register sequence is X, Z, Y.
  b.x = (magInt12)(i2c.receiveBuffer[0] << 8) | i2c.receiveBuffer[1];
  b.z = (magInt12)(i2c.receiveBuffer[2] << 8) | i2c.receiveBuffer[3];
  b.y = (magInt12)(i2c.receiveBuffer[4] << 8) | i2c.receiveBuffer[5];

  return b;
}

