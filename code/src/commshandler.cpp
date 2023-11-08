#include <commshandler.h>

CommsHandler::CommsHandler() {
  led = new Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800); //status LED
  led->begin();
  led->setBrightness(100);
}

CommsHandler::~CommsHandler() {
  delete led;
}

void CommsHandler::ledReadyState() {
  if (WiFi.status() == WL_CONNECTED ) {
    setLEDColor( LED_READY_COLOUR );
  }
  else {
    setLEDColor (LED_READY_COLOUR_NO_WIFI);
  }
}

bool CommsHandler::connectToWifi(const char *ssid, const char *pw) {
    if (WiFi.status() == WL_CONNECTED) return true;

    Serial.print(F("Connecting to wifi "));
    WiFi.begin(ssid, pw);
    WiFi.mode(WIFI_STA); //Hopefully stop unwanted SSID broadcasts!
    
    //Try for 10 seconds to get on to wifi.  If not, we'll have to run unconnected.
    for (int i=0; i<20; ++i) {
      delay(500);
      Serial.print(".");
      if (WiFi.status() == WL_CONNECTED) return true;
    }
    return false;
}

void CommsHandler::setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  led->setPixelColor(0, r, g, b);
  led->show();
}

bool CommsHandler::logAccess(const char *hash, const char *event) {
  if (WiFi.status() != WL_CONNECTED) {
	  return false;
  }
  
  WiFiClient wifiClient;
  HTTPClient httpClient;
  httpClient.setTimeout(5000);

  httpClient.begin(wifiClient, LOGURL);
  httpClient.addHeader("Content-Type", "application/json");
  String json = "{ \"type\": \"" + String(event) + "\", \"hash\": \"" + String(hash) + "\"}";

  int result = httpClient.POST(json);
  httpClient.end();

  if (result == 200) return true;
  return false;
}

void CommsHandler::OTAUpdate() {
  //Four yellow flashes to indicate start of update.
  for (int i=0; i<4; ++i) {
    setLEDColor(255,255,0);
    delay(100);
    setLEDColor(0,0,0);
    delay(100);
  }

  ESPhttpUpdate.rebootOnUpdate(false);
  WiFiClient espClient; 
  int result = ESPhttpUpdate.update(espClient, OTA_SERVER, OTA_PORT, OTA_URL);
  for (int i=0; i<4; ++i) {
    //Four green flashes if success, four red flashes if failed.
    if (result == HTTP_UPDATE_OK) setLEDColor(0,255,0);
    else setLEDColor(255,0,0);
    delay(100);
    setLEDColor(0,0,0);
    delay(100);
  }
  //Restart.
  ESP.restart();
}

void CommsHandler::ledWaitState() {
  //yellow
  setLEDColor(LED_WAIT_COLOUR);
}

void CommsHandler::deviceActivated(const char *hash) {
  setLEDColor(LED_GRANTED_COLOUR); //green
  logAccess(hash, "Activated");
}

void CommsHandler::deviceDeactivated(const char *hash) {
#if LATCH_MODE == 1 //If it's a momentary device (ie non-latching), don't log/send
  logAccess(hash, "Deactivated");
#endif
  ledReadyState();
}

void CommsHandler::loginFail(const char *hash) {
  setLEDColor(LED_DENIED_COLOUR); //go red.
  logAccess(hash, "LoginFail");
  delay(2000);
  ledReadyState();
}
