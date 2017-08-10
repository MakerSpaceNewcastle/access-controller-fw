#include <commshandler.h>

CommsHandler::CommsHandler() {
  led = new Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800); //status LED
  led->begin();
  led->setBrightness(100);

  #ifdef MQTT_SUPPORT
  espClient = new WiFiClient();
  client = new PubSubClient(*espClient);
  //Initialise MQTT server
  client->setServer(MQTT_SERVER, MQTT_PORT);
  #endif
}

CommsHandler::~CommsHandler() {
  delete led;
#ifdef MQTT_SUPPORT
  delete client;
  delete espClient;
#endif
}

void CommsHandler::ledReadyState() {
  if (WiFi.status() == WL_CONNECTED ) setLEDColor( LED_READY_COLOUR );
  else setLEDColor (LED_READY_COLOUR_NO_WIFI);
}

bool CommsHandler::connectToWifi(const char *ssid, const char *pw) {
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

void CommsHandler::setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  led->setPixelColor(0, r, g, b);
  led->show();
}

bool CommsHandler::logAccess(const char *hash, const char *event) {
  HTTPClient client;
  String fullURL(LOGURL);
  client.begin(fullURL + "&card=" + hash + "&event=" + event);
  int result = client.GET();
  client.end();
  if (result == 200) return true;
  return false;
}

bool CommsHandler::sendMQTT(const char *topic, const char *message) {
#ifdef MQTT_SUPPORT
  //Send MQTT status update.
  if (!client->connected()) {
	client->connect(DEVICENAME);
	delay(50);
  }
  if (client->connected()) {
      // Once connected, publish an announcement...
      client->publish(topic, message);
  }
#endif
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
  int result = ESPhttpUpdate.update(OTA_SERVER, OTA_PORT, OTA_URL);
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
  sendMQTT(MQTT_ACTIVATE_TOPIC, hash);
}

void CommsHandler::deviceDeactivated(const char *hash) {
#if LATCH_MODE == 1 //If it's a momentary device (ie non-latching), don't log/send
  logAccess(hash, "Deactivated");
  sendMQTT(MQTT_DEACTIVATE_TOPIC, hash);
#endif
  ledReadyState();
}

void CommsHandler::loginFail(const char *hash) {
  setLEDColor(LED_DENIED_COLOUR); //go red.
  logAccess(hash, "LoginFail");
  sendMQTT(MQTT_LOGINFAIL_TOPIC, hash);
  delay(2000);
  ledReadyState();
}
