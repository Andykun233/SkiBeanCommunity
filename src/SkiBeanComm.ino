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

/***************************************************
 * HiBean ESP32 BLE Roaster Control
 ***************************************************/

#include <Arduino.h>
#include <PID_v1.h>
#include "../lib/SerialDebug.h"
#include "../lib/SkiBLE.h"
#include "../lib/SkiLED.h"
#include "../lib/SkiCMD.h"
#include "../lib/SkiPIDConfig.h"
#include "../lib/SkiMAX6675.h"

#ifndef MAX6675_SCK_PIN
#define MAX6675_SCK_PIN 4
#endif

#ifndef MAX6675_CS_PIN
#define MAX6675_CS_PIN 5
#endif

#ifndef MAX6675_SO_PIN
#define MAX6675_SO_PIN 6
#endif

#ifndef MAX6675_2_SCK_PIN
#define MAX6675_2_SCK_PIN MAX6675_SCK_PIN
#endif

#ifndef MAX6675_2_CS_PIN
#define MAX6675_2_CS_PIN 10
#endif

#ifndef MAX6675_2_SO_PIN
#define MAX6675_2_SO_PIN MAX6675_SO_PIN
#endif

// -----------------------------------------------------------------------------
// Current Sketch and Release Version (for BLE device info)
// -----------------------------------------------------------------------------
#define FW_VERSION "Andy Fork Version 1.0.0"
String firmWareVersion = String(FW_VERSION);
String sketchName = String(__FILE__).substring(String(__FILE__).lastIndexOf('/')+1);

// -----------------------------------------------------------------------------
// Global Bean Temperature Variable
// -----------------------------------------------------------------------------
double temp          = 0.0; // temperature
double temp2         = 0.0; // second temperature
char CorF = 'C';            // default units

// -----------------------------------------------------------------------------
// MAX6675 thermocouple reader for bean temperature
// -----------------------------------------------------------------------------
SkiMAX6675 thermocouple1(MAX6675_SCK_PIN, MAX6675_CS_PIN, MAX6675_SO_PIN);
SkiMAX6675 thermocouple2(MAX6675_2_SCK_PIN, MAX6675_2_CS_PIN, MAX6675_2_SO_PIN);
unsigned long lastTempReadMs = 0;
const unsigned long TEMP_READ_INTERVAL_MS = 500;

// -----------------------------------------------------------------------------
// Track BLE writes from HiBean
// -----------------------------------------------------------------------------
std::queue<String> messageQueue;  // Holds commands written by Hibean to us

// -----------------------------------------------------------------------------
// Setup PID and Config interface
// -----------------------------------------------------------------------------
double pInput, pOutput;
double pSetpoint = 0.0; // Desired temperature (adjustable on the fly)
int manualHeatLevel = 50;

PIDConfig myPIDConfig;
PID myPID(&pInput, &pOutput, &pSetpoint,
        myPIDConfig.getKp(), myPIDConfig.getKi(), myPIDConfig.getKd(),
        myPIDConfig.getPMode(), DIRECT);  //pid instance with our default values

double convertTemperature(double celsius) {
    return (CorF == 'F') ? (1.8 * celsius + 32.0) : celsius;
}

void setup() {
    Serial.begin(115200);
    D_println("Starting HiBean ESP32 BLE Roaster Control.");
    delay(3000); //let fw upload finish before we take over hwcdc serial tx/rx

    D_println("Serial SERIAL_DEBUG ON!");

    // set pinmode on tx for commands to roaster, take it high
    pinMode(TX_PIN, OUTPUT);
    digitalWrite(TX_PIN, HIGH);

    // Start thermocouple readers for bean temperatures
    thermocouple1.begin();
    thermocouple2.begin();

    // Start BLE
    initBLE();

    // Set PID to start in MANUAL mode
    myPID.SetMode(MANUAL);

    // clamp output limits to 0-100(% heat), set sample interval 
    myPID.SetOutputLimits(0.0,myPIDConfig.getMaxPower());
    myPID.SetSampleTime(myPIDConfig.getSampleTime());

    // Ensure heat starts at 0% for safety
    manualHeatLevel = 0;
    handleHEAT(manualHeatLevel);

    shutdown();
}

void loop() {
    // roaster shut down, clear our buffers   
    if (itsbeentoolong()) { shutdown(); }

    // Read bean temperatures from MAX6675 thermocouples.
    if ((millis() - lastTempReadMs) >= TEMP_READ_INTERVAL_MS) {
        lastTempReadMs = millis();

        double celsius1 = 0.0;
        if (thermocouple1.readCelsius(celsius1)) {
            temp = convertTemperature(celsius1);
        } else {
            D_println("MAX6675 thermocouple 1 read failed.");
        }

        double celsius2 = 0.0;
        if (thermocouple2.readCelsius(celsius2)) {
            temp2 = convertTemperature(celsius2);
        } else {
            D_println("MAX6675 thermocouple 2 read failed.");
        }
    }

    // process incoming ble commands from HiBean, could be read or write
    while (!messageQueue.empty()) {
        String msg = messageQueue.front(); //grab the first one
        messageQueue.pop(); //remove it from the queue
        parseAndExecuteCommands(msg);  // process the command it
    }

    // Ensure PID or manual heat control is handled
    handlePIDControl();
    
    // update the led so user knows we're running
    handleLED();
}
