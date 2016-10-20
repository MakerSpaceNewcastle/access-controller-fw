#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#define MQTT_SUPPORT //Uncomment to enable MQTT publishing for access.
#define OTA_UPDATE_SUPPORT //Uncomment to enable OTA update triggered by rfid as specified below.


//#define LATCH_MODE 0    //The relay will be triggered for ACTIVATE_TIME ms, then automatically reset.
#define LATCH_MODE 1    //The relay will be triggered and remain 'on' until another RFID is presented to end session
#define ACTIVATE_TIME 3000 //LATCH_MODE 0 only - How long the device remains activated (in milliseconds).

//Config
const char *ssid = "";
const char *pw = "";

const char *deviceName = "";

const char *syncURL = ""; //URL for synchronising database
const char *logURL = ""; //The log API needs to be improved for multidevices FIXME
const char *MQTT_Server="";

const char *activateMQTTTopic = "";
const char *deactivateMQTTTopic = "";

//OTA Config
const char *OTAServer = "";
uint16_t OTAPort = 80;
const char *OTAURL = "";
const char *OTATriggerCardHash = "";
#endif
