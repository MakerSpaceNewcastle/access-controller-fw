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
#include <DBHandler.h> //Included as part of project tree.

//Pin defintions for MFRC522/SPI wiring
#define RST_PIN	16
#define SS_PIN	2

//Pin definition for other peripherals
#define RELAY_PIN 5
#define LED_PIN 15
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 0

//#define LATCH_MODE 0    //The relay will be triggered for ACTIVATE_TIME ms, then automatically reset.
#define LATCH_MODE 1    //The relay will be triggered and remain 'on' until another RFID is presented to end session

#if !(LATCH_MODE == 0 || LATCH_MODE == 1)
#error "LATCH_MODE must be 0 or 1"
#endif

#define ACTIVATE_TIME 3000 //How long the device remains activated (in milliseconds). No effect if LATCH_MODE is 1...

//Config
const char *ssid = "";
const char *pw = "";
const char *syncURL = ""; //URL for synchronising database

const char *filePath = "/cardDB";

MFRC522 mfrc522(SS_PIN, RST_PIN);	// Create the MFRC522 instance
Adafruit_NeoPixel led = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800); //status LED
DBHandler database(filePath, syncURL);

//Produces a blue LED if we're online, and a magenta one if we're running from the local cache
void ledToRestingColor() {
  if (WiFi.status() == WL_CONNECTED ) led.setPixelColor(0, 0, 0, 255);
  else led.setPixelColor(0,255,0,255);
  led.show();
}

//Activate device
void activateDevice() {
  Serial.println("Activating device");
  //LED to green
  led.setPixelColor(0,0,255,0);
  led.show();
  //Trigger the relay via FET
  digitalWrite(RELAY_PIN, HIGH);

#if LATCH_MODE == 0  //Non-latching, ie for door controller

  Serial.print("Waiting for ");
  Serial.print(ACTIVATE_TIME/1000);
  Serial.println(" seconds");
  delay(ACTIVATE_TIME);

#elif LATCH_MODE == 1 //Latching, remains triggered ie for milling machine controller etc

  Serial.println("Device activated.  Will remain active until card presented");
  //Wait for 2 seconds for the user to remove their card, otherwise we just keep activating/deactivating
  delay(2000);
  //Wait for a card to be presented to end session
  while (! mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50);
  }

#endif

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
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max); //Gain to maximum

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
  Serial.println("Ready, listening for card");
  //Keep trying to sense if a card is present.
  while (! mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50);
  }
  //Found a card - generate the MD5 hash of the card UID.
  String cardHash = md5_string((char*)mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println("Read card.  Card UID Hash (MD5): ");
  Serial.println(cardHash);

  bool validUser = checkCard(cardHash);
  if (validUser) {
    Serial.println("Card allowed - activating device");
    activateDevice();
  }
  else {
    //This card is NOT allowed access.
    Serial.println("Card not allowed - NOT activating device");
    //Go red for 1 second
    led.setPixelColor(0,255,0,0);
    led.show();
    delay(1000);
    //Back to normal colour.
    ledToRestingColor();
  }
}
