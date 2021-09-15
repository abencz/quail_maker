// This #include statement was automatically added by the Particle IDE.
#include "Adafruit_PWMServoDriver.h"

// This #include statement was automatically added by the Particle IDE.
#include "ServoEaser.h"

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_AM2315.h>

#include "I2CFan.h"
#include "I2CServo.h"

STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

// RH Sensor
Adafruit_AM2315 sensor;
double temp, rh;
const int polling_interval = 10000;  // ms
Timer sensor_timer(polling_interval, poll_sensor);

// read RH sensor values into Particle variables
void poll_sensor()
{
    float _temp, _rh;
    sensor.readTemperatureAndHumidity(_temp, _rh);
    temp = _temp;
    rh = _rh;
}

// Egg Turner
#define EEPROM_PARAM_ADDRESS 0x00ul
// 2 bytes major + 2 bytes minor + 4 bytes patch
// for the sake of having something complex enough that garbage won't be picked up as "valid"
#define EEPROM_PARAM_VERSION 0x01010001ul

// boundaries
#define EGG_SERVO_MIN_ANGLE 0
#define EGG_SERVO_MAX_ANGLE 180

class EggParams 
{
public:
    EggParams() : version(EEPROM_PARAM_VERSION), load_angle(90), center_angle(90), sweep_angle(45), egg_turn_time(15000) {
    }
    
    void load() {
        EggParams new_params;
        EEPROM.get(EEPROM_PARAM_ADDRESS, new_params);
        
        // only use params if version matches
        if (new_params.version == EEPROM_PARAM_VERSION) {
            *this = new_params;
        }
    }
    
    void setup() {
        load();
        Particle.function("set_param_center_angle", &EggParams::set_center_angle, this);
        //Particle.function("set_param_egg_turn_time", &EggParams::set_egg_turn_time, this);
        Particle.function("set_param_load_angle", &EggParams::set_load_angle, this);
        Particle.function("set_param_sweep_angle", &EggParams::set_sweep_angle, this);
    }
    
    void save() {
        EEPROM.put(EEPROM_PARAM_ADDRESS, *this);
    }
    
    int min_angle() {
        return (int)center_angle - sweep_angle;
    }
    
    int max_angle() {
        return (int)center_angle + sweep_angle;
    }
    
    // functions for setting parameters
    int set_center_angle(String data) {
        long new_angle = data.toInt();
        if (new_angle == 0)
            return 1;
            
        if (new_angle < EGG_SERVO_MIN_ANGLE || new_angle > EGG_SERVO_MAX_ANGLE)
            return 1;
            
        center_angle = new_angle;
        save();
        return 0;
    }

    int set_load_angle(String data) {
        long new_angle = data.toInt();
        if (new_angle == 0)
            return 1;
            
        if (new_angle < EGG_SERVO_MIN_ANGLE || new_angle > EGG_SERVO_MAX_ANGLE)
            return 1;
        
        load_angle = new_angle;
        save();
        return 0;
    }

    int set_sweep_angle(String data) {
        long new_sweep = data.toInt();
        if (new_sweep == 0)
            return 1;
        
        uint8_t old_sweep = sweep_angle;
        sweep_angle = new_sweep;
        
        if (min_angle() < EGG_SERVO_MIN_ANGLE || max_angle() > EGG_SERVO_MAX_ANGLE) {
            sweep_angle = old_sweep;  // put back the old value
            return 1;
        }
        
        save();
        return 0;
    }

    int set_egg_turn_time(String data) {
        long new_time = data.toInt();
        if (new_time < 5 || new_time > 864000)  // egg turn time must be at least 5s, at most 10 days
            return 1;

        // user provided time will be in seconds, we convert to ms
        egg_turn_time = new_time * 1000;
        save();
        return 0;
    }
    
public:
    uint32_t version;         // guard value when loading from EEPROM - should be EEPROM_PARAM_VERSION
    uint32_t egg_turn_time;  // time between turning eggs [ms]
    uint8_t load_angle;      // angle used for loading eggs
    uint8_t center_angle;    // angle where egg turner is centered
    uint8_t sweep_angle;     // angle between center_angle and min/max angles
};

EggParams params;

const int servo_pin = D3;
const int servo_frame_millis = 20;  // minimum time between servo updates
const int egg_ease_time = 2000; // time to move eggs between positions [ms]

retained int current_angle = 0;
String operating_mode = "initializing";

Adafruit_PWMServoDriver pwm;
I2CFan inside_fan(pwm, 0);

I2CServo egg_turner(pwm, 15);
ServoEaser<I2CServo> egg_easer;
Timer egg_timer(params.egg_turn_time, turn_eggs);
Timer load_eggs_timer(30000, load_eggs);
Timer start_turning_eggs_timer(60000, start_turning_eggs);

// set egg turner to neutral position, stop periodic turning
int load_eggs() {
    load_eggs_timer.stop();
    egg_timer.stop();
    operating_mode = "loading_eggs";
    egg_easer.easeTo(params.load_angle, egg_ease_time);
    current_angle = params.load_angle;
    return 0;
}

int load_eggs_function(String extras) {
    start_turning_eggs_timer.stop();
    return load_eggs();
}

// start turning eggs ever egg_turn_time ms
int start_turning_eggs() {
    start_turning_eggs_timer.stop();
    turn_eggs();
    egg_timer.start();
    operating_mode = "turning_eggs";
    return 0;
}

int start_turning_eggs_function(String extra) {
    load_eggs_timer.stop();
    return start_turning_eggs();
}

int go_to_angle(String data) {
    long angle = data.toInt();

    if (angle < EGG_SERVO_MIN_ANGLE || angle > EGG_SERVO_MAX_ANGLE)
        return 1;

    egg_easer.easeTo(angle, egg_ease_time);
}

int set_param_egg_turn_time(String data) {
    int result = params.set_egg_turn_time(data);
    if (result == 0) {
        int active = egg_timer.isActive();
        egg_timer.changePeriod(params.egg_turn_time);
        if (!active)
            egg_timer.stop();
    }
    
    return result;
}

void turn_eggs()
{
    int next_angle;
    if (current_angle < params.center_angle)
        next_angle = params.max_angle();
    else
        next_angle = params.min_angle();
        
    egg_easer.easeTo(next_angle, egg_ease_time);
}

void setup() {
    pwm.begin();
    pwm.setPWMFreq(250);  // Analog servos run at ~60 Hz updates

    sensor.begin();
    
    // initialize egg_turner
    params.setup();
    
    // set current angle to center angle if it was initialized (not retained)
    if (current_angle == 0)
        current_angle = params.center_angle;
    egg_timer.changePeriod(params.egg_turn_time);
    egg_timer.stop();
    // egg_turner.attach(servo_pin);
    egg_easer.begin(egg_turner, servo_frame_millis);
    egg_easer.setCurrPos(current_angle);
    
    Particle.variable("temperature", temp);
    Particle.variable("rh", rh);
    Particle.variable("current_angle", current_angle);
    Particle.variable("operating_mode", operating_mode);
    Particle.function("load_eggs", load_eggs_function);
    Particle.function("set_param_egg_turn_time", set_param_egg_turn_time);
    Particle.function("start_turning_eggs", start_turning_eggs_function);
    Particle.function("go_to_angle", go_to_angle);
    
    // start polling sensor
    sensor_timer.start();
    
    // start timer to do auto load
    load_eggs_timer.start();
    start_turning_eggs_timer.start();
    
    // start Particle connection
    Particle.connect();
}

void loop() {
    static unsigned long last_millis = 0;
    
    // handle time rollover
    if(millis() < last_millis) {
        egg_easer.reset();
    }
    
    egg_easer.update();
    current_angle = egg_easer.getCurrPos();
    last_millis = millis();
    
    // inside_fan.setPower(0);
    // pwm.writeMicroseconds(15, 1000);
    // delay(10000);
    // inside_fan.setPower(33);
    // pwm.writeMicroseconds(15, 1250);
    // delay(10000);
    // inside_fan.setPower(66);
    // pwm.writeMicroseconds(15, 1500);
    // delay(10000);
    // inside_fan.setPower(100);
    // pwm.writeMicroseconds(15, 2000);
    // delay(10000);
}