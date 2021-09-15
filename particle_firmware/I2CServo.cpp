#include "I2CServo.h"

// from Arduino's Servo.h
#define MIN_PULSE_WIDTH       544     // the shortest pulse sent to a servo  
#define MAX_PULSE_WIDTH      2400     // the longest pulse sent to a servo 

// from Servo.cpp
#define SERVO_MIN() (MIN_PULSE_WIDTH - this->min * 4)  // min value in uS 
#define SERVO_MAX() (MAX_PULSE_WIDTH - this->max * 4)  // max value in uS

I2CServo::I2CServo(Adafruit_PWMServoDriver &driver, uint8_t device) : i2c_driver(&driver), device_num(device), last_useconds(0) {}

int I2CServo::read()
{
	return map(last_useconds, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH, 0, 180);
}

void I2CServo::write(int value)
{
	int useconds = map(value, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
	writeMicroseconds(useconds);
}

void I2CServo::writeMicroseconds(int value)
{
	last_useconds = value;
	i2c_driver->writeMicroseconds(device_num, value);
}
