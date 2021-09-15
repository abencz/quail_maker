#include <stdint.h>

#include "Adafruit_PWMServoDriver.h"

class I2CFan
{
public:
	I2CFan(Adafruit_PWMServoDriver &driver, uint8_t device_num_);

	void setPower(uint8_t percent);

private:
	Adafruit_PWMServoDriver *i2c_driver;
	uint8_t device_num;
};