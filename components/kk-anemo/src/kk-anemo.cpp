/*

  This is a library for the KK-ANEMO Digital Anemometer Project.

  The KK_ANEMO device uses I2C for communication. Two pins are required to
  interface to the device. Configuring the I2C bus is expected to be done
  in user code. The ANEMO library doesn't do this automatically.

  Written by Karol Nowicki, March, 2023.

*/

#include "../../kk-anemo/src/kk-anemo.h"

// Define milliseconds delay for ESP8266 platform
#if defined(ESP8266)

#  include <pgmspace.h>
#  define _delay_ms(ms) delayMicroseconds((ms)*1000)

// Use _delay_ms from utils for AVR-based platforms
#elif defined(__avr__)
#  include <util/delay.h>

// Use Wiring's delay for compability with another platforms
#else
#  define _delay_ms(ms) delay(ms)
#endif

// Legacy Wire.write() function fix
#if (ARDUINO >= 100)
#  define __wire_write(d) I2C->write(d)
#else
#  define __wire_write(d) I2C->send(d)
#endif

// Legacy Wire.read() function fix
#if (ARDUINO >= 100)
#  define __wire_read() I2C->read()
#else
#  define __wire_read() I2C->receive()
#endif

/**
 * Constructor
 * @params addr Sensor address (0x56, see datasheet)
 *
 */
ANEMO::ANEMO(byte addr) {

  ANEMO_I2CADDR = addr;
  // Allows user to change TwoWire instance
  I2C = &Wire;
}

/**
 * Configure sensor
 * @param addr Address of the sensor
 * @param i2c TwoWire instance connected to I2C bus
 */
bool ANEMO::begin(byte addr, TwoWire* i2c) {

  // I2C is expected to be initialized outside this library
  // But, allows a different address and TwoWire instance to be used
  if (i2c) {
    I2C = i2c;
  }
  if (addr) {
    ANEMO_I2CADDR = addr;
  }
  ANEMO_MODE = CONFIGURED;
  return true;
}


/**
 * Read wind velocity from sensor
 *
 * @return Wind velocity in m/s (0.0 ~ 180.000)
 * 	   -1 : no valid return value
 * 	   -2 : sensor not configured
 */
float ANEMO::readWind() {

  if (ANEMO_MODE == UNCONFIGURED) {
    Serial.println(F("[ANEMO] Device is not configured!"));
    return -2.0;
  }

  // Measurement result will be stored here
  float velocity = -1.0;
  uint16_t h_value;
  uint16_t l_value;

  // Read two bytes from the sensor, which are low and high parts of the sensor
  // value
  if (3 == I2C->requestFrom((int)ANEMO_I2CADDR, (int)3)) {
    volatile unsigned int tmp = 0;
    tmp = __wire_read();
    tmp;                    //has effect- no warning of not used variable tmp :)
    h_value = __wire_read();
    l_value = __wire_read();
    velocity = h_value + (float)((l_value*1000)/256)/1000;
  }

  if (velocity != -1.0) {
// Print raw value if debug enabled
#ifdef ANEMO_DEBUG
    Serial.print(F("[ANEMO] Raw value: "));
    Serial.println(velocity);
// Print converted value if debug enabled
    Serial.print(F("[ANEMO] Converted float value: "));
    Serial.println(velocity);
#endif
  }

  return velocity;
}
