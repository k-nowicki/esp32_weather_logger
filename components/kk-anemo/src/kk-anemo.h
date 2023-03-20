/*

  This is a library for the KK-ANEMO Digital Anemometer Project.

  The ANEMO board uses I2C for communication. Two pins are required to
  interface to the device. Configuring the I2C bus is expected to be done
  in user code. The ANEMO library doesn't do this automatically.

  Written by Karol Nowicki, March, 2023.

*/

#ifndef KK_ANEMO_h
#define KK_ANEMO_h

#if (ARDUINO >= 100)
#  include <Arduino.h>
#else
#  include <WProgram.h>
#endif

#include "Wire.h"

// Uncomment, to enable debug messages
// #define ANEMO_DEBUG

// No active state
#define ANEMO_POWER_DOWN 0x00

// Waiting for measurement command
#define ANEMO_POWER_ON 0x01

// Reset data register value - not accepted in POWER_DOWN mode
#define ANEMO_RESET 0x07

class ANEMO {

public:
  enum Mode {
    // same as Power Down
    UNCONFIGURED = 0,
    // Configured
    CONFIGURED = 0x10,
  };

  ANEMO(byte addr = 0x23);
  bool begin(byte addr = 0x23, TwoWire* i2c = nullptr);
  float readWind();

private:
  byte ANEMO_I2CADDR;
  Mode ANEMO_MODE = UNCONFIGURED;
  TwoWire* I2C;
};

#endif
