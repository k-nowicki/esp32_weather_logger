# KK-ANEMO


## Overview

The KK-Anemometer is a DIY cup-anemometer based on popular A/Y3144 Hall effect 
switch. The Switch is connected to LPC1114 uC, which counts interrupts and measures
wind speed based on that. Communication with that uC is done by I2C bus.
Protocol is extremely simple - send address of the device (0x56) and receive
2-bytes of data. Higher byte is the integer part and lower byte is a fractional 
part. Lower byte needs to be divided by 256 and then added to the integer part to
make whole float value (example below).

<code>
    h_value = __wire_read(); <br/>
    l_value = __wire_read(); <br/>
    velocity = h_value + (float)((l_value*1000)/256)/1000; <br/>
</code>