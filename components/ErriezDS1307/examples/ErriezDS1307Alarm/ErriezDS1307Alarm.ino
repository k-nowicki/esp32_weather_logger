/*
 * MIT License
 *
 * Copyright (c) 2020 Erriez
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*!
 * \brief DS1307 RTC alarm example for Arduino
 * \details
 *    Source:         https://github.com/Erriez/ErriezDS1307
 *    Documentation:  https://erriez.github.io/ErriezDS1307
 */

#include <Wire.h>
#include <ErriezDS1307.h>
#include "ErriezDS1307_Alarm.h"

// Create DS1307 RTC object
ErriezDS1307 rtc;


// Alarm on handler
void alarmOn()
{
    Serial.println("Alarm ON");
}

// Alarm off handler
void alarmOff()
{
    Serial.println("Alarm OFF");
}

// Define at least one alarm (hour, minute, second, handler)
Alarm alarms[] = {
    Alarm(12, 0, 5, &alarmOn),
    Alarm(12, 0, 15, &alarmOff),
    Alarm(12, 0, 30, &alarmOn),
    Alarm(12, 1, 0, &alarmOff)
};

void printTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    char buf[10];

    // Print time
    snprintf(buf, sizeof(buf), "%d:%02d:%02d", hour, minute, second);
    Serial.println(buf);
}

void setup()
{
    // Initialize serial port
    delay(500);
    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println(F("\nErriez DS1307 software alarm example\n"));

    // Initialize TWI
    Wire.begin();
    Wire.setClock(400000);

    // Initialize RTC
    while (!rtc.begin()) {
        Serial.println(F("RTC not found"));
        delay(3000);
    }

    // Enable RTC clock
    rtc.clockEnable(true);

    // Set initial time
    rtc.setTime(12, 0, 0);
}

void loop()
{
    static uint8_t secondLast = 0xff;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    // Read RTC time
    if (!rtc.getTime(&hour, &minute, &second)) {
        Serial.println(F("Error: DS1307 read failed"));
    } else {
        // Print RTC time every second
        if (second != secondLast) {
            secondLast = second;

            // Print RTC time
            printTime(hour, minute, second);

            // Handle alarms
            for (uint8_t i = 0; i < sizeof(alarms) / sizeof(Alarm); i++) {
                alarms[i].tick(hour, minute, second);
            }
        }
    }

    // Wait 100ms
    delay(100);
}