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
#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>

#include <MD5_String.h>  //  https://github.com/alistairuk/MD5_String
#include <../lib/DBHandler/DBHandler.h> //Included as part of project tree.
#include <commshandler.h>

//Folder to use on SPIFFS

MFRC522 mfrc522(SS_PIN, RST_PIN);	// Create the MFRC522 instance

CommsHandler comms;
DBHandler database( SPIFFS_FILEPATH, SYNCURL);

// ****************** FUNCTIONS ******************

//Activate device
void activateDevice() {
  Serial.print("Activating device");
  //Trigger the relay via FET
  digitalWrite(RELAY_PIN, HIGH);
}

void deactivateDevice() {
    Serial.println("Deactivating device");
    digitalWrite(RELAY_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  comms.ledWaitState();
  comms.connectToWifi(WIFI_SSID, WIFI_PW);

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
  comms.ledReadyState();
}

bool checkCard(String hash) {
  //if it's in the local cache, then it's valid.
  if (database.contains(hash.c_str())) return true;

  if (WiFi.status() == WL_CONNECTED) {
    //Check our database is up to date, and if it isn't,
    //sync it, then check again.
    //LED to yellow while we do this.
    comms.ledWaitState();
    Serial.println("Unrecognised card, attempting database sync.");
    database.sync();
    //If it's in the cache now, it's valid.
    if (database.contains(hash.c_str())) return true;
  }
  //Wasn't in the local cache, nor the updated one.
  return false;
}

void loop() {

  unsigned long freeHeap = ESP.getFreeHeap();
  Serial.print("Free heap: ");
  Serial.println(freeHeap);
  if (freeHeap < 5000) {
    Serial.println("Free heap < 5k - rebooting");
    ESP.reset();
  }

  Serial.println("Ready, listening for card - ");

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
  if (!strcmp(cardHash.c_str(), OTA_TRIGGER_CARDHASH)) comms.OTAUpdate();
#endif

  bool validUser = checkCard(cardHash);
  if (validUser) {
    Serial.println("Card allowed - activating device");
    activateDevice();
    comms.deviceActivated(cardHash.c_str());
#if LATCH_MODE == 0  //Non-latching, ie for door controller

      Serial.print(" - Waiting for ");
      Serial.print(ACTIVATE_TIME/1000);
      Serial.println(" seconds");
      delay(ACTIVATE_TIME);
      deactivateDevice();
      comms.deviceDeactivated(cardHash.c_str());

#elif LATCH_MODE == 1 //Latching, remains triggered ie for milling machine controller etc

      Serial.print(" - Will remain active until card presented");
      //Wait for 2 seconds for the user to remove their card, otherwise we just keep activating/deactivating
      delay(2000);
      //Wait for a card to be presented to end session
      while (! mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        delay(50);
      }
      deactivateDevice();
      comms.deviceDeactivated(cardHash.c_str());

      delay(2000); //otherwise we will reactivate again as the card is still readable!
  #endif
  }
  else {
    //This card is NOT allowed access.
    Serial.println("Card not allowed - NOT activating device");
    comms.loginFail(cardHash.c_str());
  }
}
