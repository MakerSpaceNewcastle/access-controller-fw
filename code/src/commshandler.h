#ifndef COMMSHANDLER_H
#define COMMSHANDLER_H

#include "user_config.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

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

   Adafruit_NeoPixel *led;
  WiFiClient  *espClient;


};


#endif
