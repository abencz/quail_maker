#include <stdint.h>

#include "Adafruit_PWMServoDriver.h"

class I2CServo
{
public:
	I2CServo(Adafruit_PWMServoDriver &driver, uint8_t device_num_);
	int read();
	void write(int value);
	void writeMicroseconds(int value);

private:
	Adafruit_PWMServoDriver *i2c_driver;
	uint8_t device_num;
	int last_useconds;
};