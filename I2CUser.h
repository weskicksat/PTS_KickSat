#ifndef I2CUser_h
#define I2CUser_h

// Shared data buffer for all I2C users.
// Since there is no multithreading, all send and receive buffers can be combined
// into a single lump of memory, saving precious RAM.

// Start with a single byte buffer.
#define I2C_SEND_BUFFER_SIZE (1)
#define I2C_RECEIVE_BUFFER_SIZE (1)

// Expand the buffer size for each user.

// Magnetometer sends 2 bytes and receives 6 bytes.
#define MAG_SEND_BUFFER_SIZE (2)
#if MAG_SEND_BUFFER_SIZE > I2C_SEND_BUFFER_SIZE
#undef I2C_SEND_BUFFER_SIZE
#define I2C_SEND_BUFFER_SIZE MAG_SEND_BUFFER_SIZE
#endif

#define MAG_RECEIVE_BUFFER_SIZE (6)
#if MAG_RECEIVE_BUFFER_SIZE > I2C_RECEIVE_BUFFER_SIZE
#undef I2C_RECEIVE_BUFFER_SIZE
#define I2C_RECEIVE_BUFFER_SIZE MAG_RECEIVE_BUFFER_SIZE
#endif

// Gyro sends 2 bytes and receives 8 bytes.
#define GYRO_SEND_BUFFER_SIZE (2)
#if GYRO_SEND_BUFFER_SIZE > I2C_SEND_BUFFER_SIZE
#undef I2C_SEND_BUFFER_SIZE
#define I2C_SEND_BUFFER_SIZE GYRO_SEND_BUFFER_SIZE
#endif

#define GYRO_RECEIVE_BUFFER_SIZE (8)
#if GYRO_RECEIVE_BUFFER_SIZE > I2C_RECEIVE_BUFFER_SIZE
#undef I2C_RECEIVE_BUFFER_SIZE
#define I2C_RECEIVE_BUFFER_SIZE GYRO_RECEIVE_BUFFER_SIZE
#endif

class I2CUser {
public:
  byte sendBuffer[I2C_SEND_BUFFER_SIZE];
  byte receiveBuffer[I2C_RECEIVE_BUFFER_SIZE];
};

extern I2CUser i2c;

#endif

