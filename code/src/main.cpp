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
#include <../lib/DBHandler/DBHandler.h> //Included as part of project tree.
#include <commshandler.h>

//Folder to use on SPIFFS

MFRC522 mfrc522(SS_PIN, RST_PIN);	// Create the MFRC522 instance

CommsHandler comms;
DBHandler database( SPIFFS_FILEPATH, DB_VER_URL, DB_SYNC_URL);

// ****************** FUNCTIONS ******************

//Activate device
void activateDevice() {
  Serial.print(F("Activating device"));
  //Trigger the relay via FET
  digitalWrite(RELAY_PIN, HIGH);
}

void deactivateDevice() {
    Serial.println(F("Deactivating device"));
    digitalWrite(RELAY_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting..."));

  comms.ledWaitState();
  comms.connectToWifi(WIFI_SSID, WIFI_PW);

  //Relay control pin for the FET
  pinMode(RELAY_PIN, OUTPUT);

  // Init the SPI bus + RFID reader
  SPI.begin();
  mfrc522.PCD_Init();

  //Initialise and if necessary sync the user database.
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("Syncing database"));
    if (database.sync()) Serial.println(" - done");
    else Serial.println(" - failed");
  }
  else {
    Serial.println(F("Running in offline mode, using local database"));
  }
  comms.ledReadyState();
}

bool checkCard(char *hash) {
  //if it's in the local cache, then it's valid.
  if (database.contains(hash)) return true;

  if (WiFi.status() == WL_CONNECTED) {
    //Check our database is up to date, and if it isn't,
    //sync it, then check again.
    //LED to yellow while we do this.
    comms.ledWaitState();
    Serial.println(F("Unrecognised card, attempting database sync."));
    database.sync();
    //If it's in the cache now, it's valid.
    if (database.contains(hash)) return true;
  }
  //Wasn't in the local cache, nor the updated one.
  return false;
}

void loop() {

  unsigned long freeHeap = ESP.getFreeHeap();
  Serial.print(F("Free heap: "));
  Serial.println(freeHeap);
  if (freeHeap < 5000) {
    Serial.println(F("Free heap < 5k - rebooting"));
    ESP.reset();
  }

  Serial.println(F("Ready, listening for card - "));

  uint8_t reset_count = 0;
  //Keep trying to sense if a card is present.
  while (! mfrc522.PICC_IsNewCardPresent() ) {
    //This is in the polling loop so that the led colour can be flipped if the wifi status changes 
    //ie we connect or disconnect.
    comms.ledReadyState();
    delay(50);
    
    //This is an attempt to stop the front door reader crashing....
    //Will reset every 10 seconds ish (255*50mS)
    reset_count++;
    if (reset_count == 255) {
      mfrc522.PCD_Init();
    }
  }

  //Read the card - if it's unreadable, return, and loop will run again.
  if (!mfrc522.PICC_ReadCardSerial()) 
    return;

  //Generate the MD5 hash of the card UID.
  MD5Builder hashBuilder;
  hashBuilder.begin();
  hashBuilder.add(mfrc522.uid.uidByte, mfrc522.uid.size);
  hashBuilder.calculate();
  char hash[17];
  hashBuilder.getChars(&hash[0]);

  Serial.println(F("Read card.  Card UID Hash (MD5): "));
  Serial.println(hash);
#ifdef OTA_UPDATE_SUPPORT
  //See if this card is the OTA update trigger card, and if so, do the update.
  if (!strcmp(hash, OTA_TRIGGER_CARDHASH)) comms.OTAUpdate();
#endif

  bool validUser = checkCard(hash);
  if (validUser) {
    Serial.println(F("Card allowed - activating device"));
    activateDevice();
    comms.deviceActivated(hash);
#if LATCH_MODE == 0  //Non-latching, ie for door controller

      Serial.print(" - Waiting for ");
      Serial.print(ACTIVATE_TIME/1000);
      Serial.println(" seconds");
      delay(ACTIVATE_TIME);
      deactivateDevice();
      comms.deviceDeactivated(hash);

#elif LATCH_MODE == 1 //Latching, remains triggered ie for milling machine controller etc

      Serial.print(F(" - Will remain active until card presented"));
      //Wait for 2 seconds for the user to remove their card, otherwise we just keep activating/deactivating
      delay(2000);
      //Wait for a card to be presented to end session
      while (! mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        delay(50);
      }
      deactivateDevice();
      comms.deviceDeactivated(hash);

      delay(2000); //otherwise we will reactivate again as the card is still readable!
  #endif
  }
  else {
    //This card is NOT allowed access.
    Serial.println(F("Card not allowed - NOT activating device"));
    comms.loginFail(hash);
  }
}
