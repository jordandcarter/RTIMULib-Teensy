////////////////////////////////////////////////////////////////////////////
//
//  This file is part of RTIMULib-Teensy
//
//  Copyright (c) 2014-2015, richards-tech
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of 
//  this software and associated documentation files (the "Software"), to deal in 
//  the Software without restriction, including without limitation the rights to use, 
//  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
//  Software, and to permit persons to whom the Software is furnished to do so, 
//  subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all 
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <EEPROM.h>
#include "I2Cdev.h"
#include "RTIMULib.h"


RTIMU *imu;                                           // the IMU object
RTIMUSettings *settings;                              // the settings object

//  DISPLAY_INTERVAL sets the rate at which results are displayed

#define DISPLAY_INTERVAL  100                         // interval between pose displays

//  SERIAL_PORT_SPEED defines the speed to use for the debug serial port

#define  SERIAL_PORT_SPEED  115200

unsigned long lastDisplay;
unsigned long lastRate;
int sampleCount;
RTQuaternion gravity;

void setup()
{
    int errcode;
  
    Serial.begin(SERIAL_PORT_SPEED);
    while (!Serial) {
        ; // wait for serial port to connect. 
    }
    Wire.begin();
    settings = new RTIMUSettings();
    imu = RTIMU::createIMU(settings);                        // create the imu object
  
    Serial.print("TeensyAccel starting using device "); Serial.println(imu->IMUName());
    if ((errcode = imu->IMUInit()) < 0) {
        Serial.print("Failed to init IMU: "); Serial.println(errcode);
    }
  
    if (imu->getCompassCalibrationValid())
        Serial.println("Using compass calibration");
    else
        Serial.println("No valid compass calibration data");

    lastDisplay = lastRate = millis();
    sampleCount = 0;
  
    gravity.setScalar(0);
    gravity.setX(0);
    gravity.setY(0);
    gravity.setZ(1);
}

void loop()
{  
    unsigned long now = millis();
    unsigned long delta;
    RTVector3 realAccel;
    RTQuaternion rotatedGravity;
    RTQuaternion fusedConjugate;
    RTQuaternion qTemp;
    RTIMU_DATA imuData;
    if (imu->IMURead()) {                                // get the latest data if ready yet
        imuData = imu->getIMUData();
    
        //  do gravity rotation and subtraction
    
        // create the conjugate of the pose
    
        fusedConjugate = imuData.fusionQPose.conjugate();
    
        // now do the rotation - takes two steps with qTemp as the intermediate variable
    
        qTemp = gravity * imuData.fusionQPose;
        rotatedGravity = fusedConjugate * qTemp;
    
        // now adjust the measured accel and change the signs to make sense
    
        realAccel.setX(-(imuData.accel.x() - rotatedGravity.x()));
        realAccel.setY(-(imuData.accel.y() - rotatedGravity.y()));
        realAccel.setZ(-(imuData.accel.z() - rotatedGravity.z()));
    
        sampleCount++;
        if ((delta = now - lastRate) >= 1000) {
            Serial.print("Sample rate: "); Serial.print(sampleCount);
            if (!imu->IMUGyroBiasValid())
                Serial.println(", calculating gyro bias");
            else
                Serial.println();
        
            sampleCount = 0;
            lastRate = now;
        }
        if ((now - lastDisplay) >= DISPLAY_INTERVAL) {
            lastDisplay = now;
      
            Serial.print(RTMath::displayRadians("Accel:", realAccel));
        }
    }
}

