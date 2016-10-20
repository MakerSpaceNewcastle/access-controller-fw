/*
 * Basic 'standard' RFID access controller for ESP8266/MFRC522 controller boards
 * https://www.github.com/davidmpye/RFID_access_controller
 * Version 1.2 of board h/w.
 * Alistair MacDonald 2015
 * David Pye 2016 - davidmpye@gmail.com
 * GNU GPL v2.0 or later
 * Needs: https://github.com/alistairuk/MD5_String library
 *        https://github.com/davidmpye/EDB_FS library
 *        Adafruit neopixel library
 *        ESP8266 Core libraries
 */
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include <MD5_String.h>  //  https://github.com/alistairuk/MD5_String
#include <../lib/DBHandler/DBHandler.h> //Included as part of project tree.

#include "user_config.h"

#ifdef MQTT_SUPPORT
#include <PubSubClient.h> //for MQTT announcements...
#endif

#ifdef OTA_UPDATE_SUPPORT
#include <ESP8266httpUpdate.h>
#endif

//Pin defintions for MFRC522/SPI wiring
#define RST_PIN	15
#define SS_PIN	2

//Pin definition for other peripherals
#define RELAY_PIN 5
#define LED_PIN 15
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 0

#if !(LATCH_MODE == 0 || LATCH_MODE == 1)
#error "LATCH_MODE must be defined as 0 or 1"
#endif

#ifdef MQTT_SUPPORT
WiFiClient espClient;
PubSubClient client(espClient);
#endif

//Folder to use on SPIFFS
const char *filePath = "/cardDB";

MFRC522 mfrc522(SS_PIN, RST_PIN);	// Create the MFRC522 instance
Adafruit_NeoPixel led = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800); //status LED
DBHandler database(filePath, syncURL);

// ****************** FUNCTIONS ******************

void sendMQTT(const char *topic, const char *message) {

#ifdef MQTT_SUPPORT
  //Send MQTT broadcast to space's server.
  //Connect as backdoor to the server.
  if (!client.connected() ) client.connect(deviceName);
  if (client.connected()) {
      // Once connected, publish an announcement...
      client.publish(topic, message);
    }
#endif

}

//Hit the log URL.
bool logAccess(String hash, const char *event) {
  HTTPClient client;
  String fullURL(logURL);
  client.begin(fullURL + "?card=" + hash + "&event=" + event);
  int result = client.GET();
  client.end();
  if (result == 200) return true;
  return false;
}

//Produces a blue LED if we're online, and a magenta one if we're running from the local cache
void ledToRestingColor() {
  if (WiFi.status() == WL_CONNECTED ) led.setPixelColor(0, 0, 0, 255);
  else led.setPixelColor(0,255,0,255);
  led.show();
}

//Activate device
void activateDevice() {
  Serial.print("Activating device");
  //LED to green
  led.setPixelColor(0,0,255,0);
  led.show();
  //Trigger the relay via FET
  digitalWrite(RELAY_PIN, HIGH);
}

void deactivateDevice() {
    Serial.println("Deactivating device");
    digitalWrite(RELAY_PIN, LOW);

    //LED back to resting colour
    ledToRestingColor();
    //Wait for 2 seconds otherwise we can end up reactivating immediately if card still present....
    delay(2000);
}


bool connectToWifi(const char *SSID, const char *pw) {
    if (WiFi.status() == WL_CONNECTED) return true;

    Serial.print("Connecting to wifi ");
    WiFi.begin(ssid, pw);
    //Try for 10 seconds to get on to wifi.  If not, we'll have to run unconnected.
    for (int i=0; i<20; ++i) {
      delay(500);
      Serial.print(".");
      if (WiFi.status() == WL_CONNECTED) return true;
    }
    return false;
}

void OTAUpdate() {
  //Four yellow flashes to indicate start of update.
  for (int i=0; i<4; ++i) {
    led.setPixelColor(0,255,255,0);
    led.show();
    delay(100);
    led.clear();
    led.show();
    delay(100);
  }
  ESPhttpUpdate.rebootOnUpdate(false);
  int result = ESPhttpUpdate.update(OTAServer, OTAPort, OTAURL);

  for (int i=0; i<4; ++i) {
    //Four green flashes if success, four red flashes if failed.
    if (result == HTTP_UPDATE_OK) led.setPixelColor(0, 0,255,0);
    else led.setPixelColor(0,255,0,0);
    led.show();
    delay(100);
    led.clear();
    led.show();
    delay(100);
  }
  //Restart.
  ESP.restart();
}

void setup() {
  Serial.begin(115200);

  Serial.println("Starting...");

  // Set up a yellow LED while we are getting ready
  led.begin();
  led.setBrightness(100);
  led.setPixelColor(0,255,255,0);
  led.show();

  connectToWifi(ssid, pw);

  //Relay control pin for the FET
  pinMode(RELAY_PIN, OUTPUT);

  // Init the SPI bus + RFID reader
  SPI.begin();
  mfrc522.PCD_Init();

  //Initialise and if necessary sync the user database.
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Syncing database");
    if (database.sync()) Serial.println(" - done");
    else Serial.println(" - failed");
  }
  else {
    Serial.println("Running in offline mode, using local database");
  }
  ledToRestingColor();
#ifdef MQTT_SUPPORT
  //Initialise MQTT server
  client.setServer(MQTT_Server,1883);
#endif
}

bool checkCard(String hash) {
  //if it's in the local cache, then it's valid.
  if (database.contains(hash.c_str())) return true;

  if (WiFi.status() == WL_CONNECTED) {
    //Check our database is up to date, and if it isn't,
    //sync it, then check again.
    //LED to yellow while we do this.
    led.setPixelColor(0,255,255,0);
    led.show();
    Serial.println("Unrecognised card, attempting database sync.");
    database.sync();
    //If it's in the cache now, it's valid.
    if (database.contains(hash.c_str())) return true;
  }
  //Wasn't in the local cache, nor the updated one.
  return false;
}

void loop() {
  Serial.println("Ready, listening for card - ");
  Serial.print("free heap: ");
  Serial.println(ESP.getFreeHeap());
  //Keep trying to sense if a card is present.
  while (! mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50);
  }
  //Found a card - generate the MD5 hash of the card UID.
  String cardHash = md5_string((char*)mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println("Read card.  Card UID Hash (MD5): ");
  Serial.println(cardHash);
#ifdef OTA_UPDATE_SUPPORT
  //See if this card is the OTA update trigger card, and if so, do the update.
  if (!strcmp(cardHash.c_str(), OTATriggerCardHash)) OTAUpdate();
#endif

  bool validUser = checkCard(cardHash);
  if (validUser) {
    Serial.println("Card allowed - activating device");
    activateDevice();
    logAccess(cardHash, "Activated");
    sendMQTT(activateMQTTTopic, cardHash.c_str());

    #if LATCH_MODE == 0  //Non-latching, ie for door controller

      Serial.print(" - Waiting for ");
      Serial.print(ACTIVATE_TIME/1000);
      Serial.println(" seconds");
      delay(ACTIVATE_TIME);
      deactivateDevice();

    #elif LATCH_MODE == 1 //Latching, remains triggered ie for milling machine controller etc

      Serial.print(" - Will remain active until card presented");
      //Wait for 2 seconds for the user to remove their card, otherwise we just keep activating/deactivating
      delay(2000);
      //Wait for a card to be presented to end session
      while (! mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        delay(50);
      }
      deactivateDevice();
      logAccess(cardHash, "Deactivated");
      sendMQTT(deactivateMQTTTopic, cardHash.c_str());
    #endif
  }
  else {
    //This card is NOT allowed access.
    Serial.println("Card not allowed - NOT activating device");
    //Go red for 1 second
    led.setPixelColor(0,255,0,0);
    led.show();
    delay(1000);
    //Log the denied access.
    logAccess(cardHash, "Denied");
    //Back to normal colour.
    ledToRestingColor();
  }
}
