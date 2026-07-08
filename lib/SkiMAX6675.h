/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <Arduino.h>

class SkiMAX6675 {
public:
    SkiMAX6675(uint8_t sckPin, uint8_t csPin, uint8_t soPin)
        : sckPin_(sckPin), csPin_(csPin), soPin_(soPin)
    {
    }

    void begin()
    {
        pinMode(sckPin_, OUTPUT);
        pinMode(csPin_, OUTPUT);
        pinMode(soPin_, INPUT);

        digitalWrite(csPin_, HIGH);
        digitalWrite(sckPin_, LOW);
    }

    bool readCelsius(double& celsius)
    {
        uint16_t raw = readRaw();

        if (raw & 0x0004) {
            return false;
        }

        celsius = ((raw >> 3) & 0x0FFF) * 0.25;
        return true;
    }

private:
    uint16_t readRaw()
    {
        uint16_t value = 0;

        digitalWrite(csPin_, LOW);
        delayMicroseconds(1);

        for (uint8_t i = 0; i < 16; i++) {
            digitalWrite(sckPin_, HIGH);
            delayMicroseconds(1);

            value <<= 1;
            if (digitalRead(soPin_) == HIGH) {
                value |= 1;
            }

            digitalWrite(sckPin_, LOW);
            delayMicroseconds(1);
        }

        digitalWrite(csPin_, HIGH);
        return value;
    }

    uint8_t sckPin_;
    uint8_t csPin_;
    uint8_t soPin_;
};
