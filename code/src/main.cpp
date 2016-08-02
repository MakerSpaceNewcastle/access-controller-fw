/*
 * Basic 'standard' RFID access controller for ESP8266/MFRC522 controller boards
 * https://www.github.com/davidmpye/RFID_access_controller
 * Version 1.2 of board h/w.
 * Alistair MacDonald 2015
 * David Pye 2016
 *
 */

#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
//Uses Alistair MacDonald's MD5 library, available here: https://github.com/alistairuk/MD5_String
#include <MD5_String.h>
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>


//Pin defintions for MFRC522 wiring
#define RST_PIN	16
#define SS_PIN	2

//Pin definition for other peripherals
#define RELAY_PIN 5
#define LED_PIN 15
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 0

//Config
//#define LATCH_MODE 0    //The relay will be triggered for ACTIVATE_TIME ms, then automatically reset.
#define LATCH_MODE 1    //The relay will be triggered and remain 'on' until another RFID is presented to end session

#define ACTIVATE_TIME 3000 //How long the device remains activated (in milliseconds). No effect if LATCH_MODE is 1...


MFRC522 mfrc522(SS_PIN, RST_PIN);	// Create the MFRC522 instance
Adafruit_NeoPixel led = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800); //status LED

//Add in your card RFID hashes here, separated by a \n
//eg String allowedCards="blahblah\nfeedbeef"

String allowedCards="e4c4e25f7c68060b6684e4fbf0ed0487\ndfbbcfe0920817d6d463110ab0d5fd7c\n";


#if !(LATCH_MODE == 0 || LATCH_MODE == 1)
#error "LATCH_MODE must be 0 or 1"
#endif

//Helper function to hash the card ID
String hashID() {
  return md5_string((char*)mfrc522.uid.uidByte, mfrc522.uid.size);
}

//Activate relay
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
  //Wait at 2 seconds to see if a card is present, otherwise we just keep activating/deactivating
  delay(2000);
  //Wait for a card to be presented to end session
  while (! mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50);
  }
#endif
  Serial.println("Deactivating device");
  digitalWrite(RELAY_PIN, LOW);

 //LED back to blue
 led.setPixelColor(0,0,0,250);
 led.show();

  //Wait for 1 second otherwise we can end up reactivating immediately if card still present....
  delay(2000);
}

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("Init....");

  //Relay control pin for the FET
  pinMode(RELAY_PIN, OUTPUT);

  // Init the SPI bus + RFID reader
  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max); //try gain to max

  // Set up a yellow LED while we are getting ready
  led.begin();
  led.setBrightness(100);
  led.setPixelColor(0,255,255,0);
  led.show();

  //LED to blue.
  led.setPixelColor(0,0,0,250);
  led.show();
}

void loop() {

  Serial.println("Ready, listening for card");
  //Keep trying to sense if a card is present.
  while (! mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50);
  }
  //Found a card.
  //Calculate the MD5 hash of the Card UID field.
  String cardHash = hashID();

  Serial.println("Read card.  Card UID Hash (MD5): ");
  Serial.println(cardHash);

  //LED to yellow while we check local database
  led.setPixelColor(0,255,255,0);
  led.show();

  // Is the card in the cache database
  if (allowedCards.indexOf(cardHash)>=0) {
    Serial.println("Card allowed - activating device");
    activateDevice();
  }
  else {
    //Access DENIED....
    Serial.println("Card not allowed - NOT activating device");
    //Go red for 1 second
    led.setPixelColor(0,255,0,0);
    led.show();
    delay(1000);
  }
}
