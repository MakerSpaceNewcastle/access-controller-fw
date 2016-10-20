#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include <Arduino.h>

#define MQTT_SUPPORT //Uncomment to enable MQTT publishing for access.
#define OTA_UPDATE_SUPPORT //Uncomment to enable OTA update triggered by rfid as specified below.

//#define LATCH_MODE 0    //The relay will be triggered for ACTIVATE_TIME ms, then automatically reset.
#define LATCH_MODE 1    //The relay will be triggered and remain 'on' until another RFID is presented to end session
#define ACTIVATE_TIME 3000 //LATCH_MODE 0 only - How long the device remains activated (in milliseconds).

//Config
#define WIFI_SSID ""
#define WIFI_PW ""

#define DEVICENAME ""

#define SYNCURL "http://www.blah.com?dev=" DEVICENAME //URL for synchronising database
#define LOGURL  "http://www.blah.com?dev=" DEVICENAME //The log API needs to be improved for multidevices FIXME
#define MQTT_SERVER "192.168.0.1"
#define MQTT_PORT 1833

#define MQTT_ACTIVATE_TOPIC "status/" DEVICENAME "/activated"
#define MQTT_DEACTIVATE_TOPIC "status/" DEVICENAME "/deactivated"
#define MQTT_LOGINFAIL_TOPIC "status/" DEVICENAME "/loginfail"

//OTA Config
#define OTA_SERVER "192.168.0.1"
#define OTA_PORT 80
#define OTA_URL "/RFID/" DEVICENAME ".bin"
#define OTA_TRIGGER_CARDHASH ""

//Board structure/pinouts
//Pin defintions for MFRC522/SPI wiring
#define RST_PIN	15 //ACTUALLY it's wired to 16, but seems to prefer being somewhere else!
#define SS_PIN	2

//Pin definition for other peripherals
#define RELAY_PIN 5
#define LED_PIN 15
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 0

#define SPIFFS_FILEPATH "/cardDB"

#if !(LATCH_MODE == 0 || LATCH_MODE == 1)
#error "LATCH_MODE must be defined as 0 or 1"
#endif

#endif
