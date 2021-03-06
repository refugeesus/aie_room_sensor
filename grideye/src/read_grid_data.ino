/*
 * Copyright 2017, Helium Systems, Inc.
 * All Rights Reserved. See LICENCE.txt for license information
 */


#include "Arduino.h"
#include "Board.h"
#include "Helium.h"
#include "HeliumUtil.h"
#include "ArduinoJson.h"
#include "SparkFunCCS811.h"
#include "Adafruit_AMG88xx.h"

// NOTE: Please ensure you've created a channel with the above
// CHANNEL_NAME as it's name.
#define CHANNEL_NAME "Helium MQTT"
#define CONFIG_INTERVAL_KEY "config.interval_ms"
#define CONFIG_TEMP_KEY "config.temp_c"
#define CCS811_ADDR 0x5B

Helium  helium(&atom_serial);
Channel channel(&helium);
Config config(&channel);
int32_t temp_c = 30;
unsigned long cur_time;
unsigned long start_time;
int32_t send_interval = 10000;
int reading_1 = 0;
int reading_2 = 0;
int reading_3 = 0;

CCS811 ccs(CCS811_ADDR);
Adafruit_AMG88xx amg;

//INT pin from the sensor board goes to this pin on your microcontroller board
#define INT_PIN 2

#define TEMP_INT_HIGH 30
#define TEMP_INT_LOW 15

volatile bool intReceived = false;
uint8_t pixelInts[8];
/******* 
         bit 0  bit 1  bit 2  bit 3  bit 4  bit 5  bit 6  bit 7
byte 0 |  0      1      0      0      0      0      0      1
byte 1 |  0      0      0      0      0      0      0      0
byte 2 |  0      0      0      0      0      0      0      0
byte 3 |  0      0      0      1      0      0      0      0
byte 4 |  0      0      0      0      0      0      0      0
byte 5 |  0      0      0      0      0      0      0      0
byte 6 |  0      0      0      0      0      1      0      0
byte 7 |  0      0      0      0      0      0      0      0
*****/

void
setup()
{
    Serial.begin(9600);
 
    pinMode(INT_PIN, INPUT);
  
    helium.begin(HELIUM_BAUD_RATE);
    
    // Connect the Atom to the Helium Network
    helium_connect(&helium);
    
    CCS811Core::status returnCode = ccs.begin();
    if (returnCode != CCS811Core::SENSOR_SUCCESS)
    {
        Serial.println(".begin() returned with an error.");
        while (1); //Hang if there was a problem.
    }    
    
    bool status;
  
    // default settings
    status = amg.begin();
    if (!status) {
        Serial.println("Could not find a valid AMG88xx sensor, check wiring!");
        while (1);
    }
  
    amg.setInterruptLevels(TEMP_INT_HIGH, TEMP_INT_LOW);

    //set to absolue value mode
    amg.setInterruptMode(AMG88xx_ABSOLUTE_VALUE);

    //enable interrupts
    amg.enableInterrupt();

    //attach to our Interrupt Service Routine (ISR)
    attachInterrupt(digitalPinToInterrupt(INT_PIN), AMG88xx_ISR, FALLING);

    channel_create(&channel, CHANNEL_NAME);
    start_time = millis();
}

void
loop()
{
    cur_time = millis();
    if (ccs.dataAvailable())
    {
        ccs.readAlgorithmResults();

        reading_1 = ccs.getCO2();
        reading_2 = ccs.getTVOC();
    }
    
    if(intReceived) {
        
        amg.getInterrupt(pixelInts);
        
        for(int i=0; i<8; i++){
            Serial.println(pixelInts[i], BIN);
        }
        Serial.println();
        amg.clearInterrupt();
        intReceived = false;
    }

    if ((cur_time - start_time) > send_interval) {
        StaticJsonBuffer<JSON_OBJECT_SIZE(2) + 100> jsonBuffer;
        JsonObject & root = jsonBuffer.createObject();
        uint8_t used; 
        char buffer[HELIUM_MAX_DATA_SIZE];
        root[F("co2")] = reading_1;
        root[F("tvoc")] = reading_2;
        root[F("used")] = reading_3;

        used = root.printTo(buffer, HELIUM_MAX_DATA_SIZE);
        // Send data to channel
        channel_send(&channel, CHANNEL_NAME, buffer, used);
        // Print status and result
        update_config(true);
        // Wait a while till the next time
        start_time = cur_time;
        Serial.println(reading_3);
        reading_3 = 0;
    }
}

void
update_config(bool stale)
{
    if (stale)
    {
        DBG_PRINT(F("Fetching Config - "));
        int status = config.get(CONFIG_INTERVAL_KEY, &send_interval, 5000);
        report_status(status);
        
        if (status == helium_status_OK)
        {
            DBG_PRINT(F("Updating Config - "));
            status = config.set(CONFIG_INTERVAL_KEY, send_interval);
            report_status(status);
        }
        
        status = config.get(CONFIG_TEMP_KEY, &temp_c, 5);
        report_status(status);
        
        if (status == helium_status_OK)
        {
            DBG_PRINT(F("Updating Config - "));
            status = config.set(CONFIG_TEMP_KEY, temp_c);
            report_status(status);

            amg.setInterruptLevels(temp_c, TEMP_INT_LOW);
        }
        
    }
}

void AMG88xx_ISR() {
  //keep your ISR short!
  //we don't really want to be reading from or writing to the sensor from inside here.
  intReceived = true;
  reading_3 = 1;
}
