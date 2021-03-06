#ifndef _GYRO_H_
#define _GYRO_H_
#include "main.h"

#define ADDRESS 0x68
#define WHO_AM_I 0x75

#define PWR_MGMT_1 0x6B
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C

#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E

#define GYRO_ZOUT_H 0x47
#define GYRO_ZOUT_L 0x48

#define GYRO_FACTOR 16.4

class Gyro
{
private:
  float gz_offset;
  float gz_y_pre[4], gz_x_pre[4];

  // IIR filter coefficient 
  const float a1 = 0.94280904158206336;
  const float a2 = -0.33333333333333343;
  const float b0 = 0.09763107293781749;
  const float b1 = 0.19526214587563498;
  const float b2 = 0.09763107293781749;

  bool flag_gyro = false;

public:
  float yaw, gz;

  uint8_t read_byte(uint8_t reg);
  void write_byte(uint8_t reg, uint8_t data);
  void IIRInit();
  void GyroInit();
  void GyroOffsetCalc();
  void GetGyroData();
};

#endif // _GYRO_H_
