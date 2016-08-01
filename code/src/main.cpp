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
#define DOOR_UNLOCK_TIME 3000 //How long the door lock is left open (in milliseconds)

MFRC522 mfrc522(SS_PIN, RST_PIN);	// Create the MFRC522 instance
Adafruit_NeoPixel led = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800); //status LED

//Add in your card RFID hashes here, separated by a \n
//eg String allowedCards="blahblah\nfeedbeef"

String allowedCards="e4c4e25f7c68060b6684e4fbf0ed0487\ndfbbcfe0920817d6d463110ab0d5fd7c\n";

//Helper function to hash the card ID
String hashID() {
  return md5_string((char*)mfrc522.uid.uidByte, mfrc522.uid.size);
}

//Activate relay
void openLatch() {
  Serial.println("Unlocking Door");
  digitalWrite(RELAY_PIN, HIGH);
  Serial.print("Waiting for ");
  Serial.print(DOOR_UNLOCK_TIME/1000);
  Serial.println(" seconds");
  delay(DOOR_UNLOCK_TIME);
  Serial.println("Securing door...");
  digitalWrite(RELAY_PIN, LOW);
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
  //Now ready to accept cards.
  Serial.println("Ready, listening for card");

}

void loop() {

  //Keep trying to sense if a card is present.
  while (! mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50);
  }

  //Found a card.

  //Calculate the MD5 hash of the Card UID field.
  String cardHash = hashID();

  Serial.println("Read valid card.  Card UID Hash (MD5): ");
  Serial.println(cardHash);

  //LED to yellow while we check local database
  led.setPixelColor(0,255,255,0);
  led.show();

  // Is the card in the cache database
  if (allowedCards.indexOf(cardHash)>=0) {
    Serial.println("Card OK - opening door");
    //Green LED
    led.setPixelColor(0,0,255,0);
    led.show();
    openLatch();
  }
  else {
    //Access DENIED....
    Serial.println("Card not allowed - NOT opening door");
    //Go red for 1 second
    led.setPixelColor(0,255,0,0);
    led.show();
    delay(1000);
  }

  //Back to blue
  led.setPixelColor(0,0,0,250);
  led.show();

  Serial.println("Ready, listening for card");
}
