#include "I2CFan.h"

I2CFan::I2CFan(Adafruit_PWMServoDriver &driver, uint8_t device) : i2c_driver(&driver), device_num(device) {}

void I2CFan::setPower(uint8_t percent)
{
	if (percent > 100)
		percent = 100;
		
	uint16_t pwm = 4095 * percent / 100;
	i2c_driver->setPin(device_num, pwm);
}
