#ifndef COMMSHANDLER_H
#define COMMSHANDLER_H

#include "user_config.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#ifdef MQTT_SUPPORT
//These set the timeouts for MQTT but need to be redefined above the PubSubClient include.
#define MQTT_KEEPALIVE 5
#define MQTT_SOCKET_TIMEOUT 5
#include <PubSubClient.h> //for MQTT announcements...
#endif

#ifdef OTA_UPDATE_SUPPORT
#include <ESP8266httpUpdate.h>
#endif

class CommsHandler {
public:

 CommsHandler();
 ~CommsHandler();

 //Produces a blue LED if we're online, and a magenta one if we're running from the local cache

 bool connectToWifi(const char *SSID, const char *pw);
 void OTAUpdate();

 void deviceActivated(const char *hash);
 void deviceDeactivated(const char *hash);
 void loginFail(const char *hash);

 void ledWaitState(); //yellow
 void ledReadyState();

private:

  void setLEDColor(uint8_t, uint8_t, uint8_t);
  bool logAccess(const char *hash, const char *event);
  bool sendMQTT(const char *topic, const char *message);

   Adafruit_NeoPixel *led;
  WiFiClient  *espClient;
#ifdef MQTT_SUPPORT
  PubSubClient *client;
#endif

};


#endif
