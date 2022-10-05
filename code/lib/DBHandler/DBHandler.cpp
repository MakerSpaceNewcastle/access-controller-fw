#include <DBHandler.h>

DBHandler::DBHandler(const char *filename, const char *url) {
  filePath = filename;
  syncURL = url;
  if ( ! LittleFS.begin()) {
    Serial.println(F("Unable to init LittleFS - trying format"));
    //Oh dear.  Try formatting it, might be a fresh board.
    if ( LittleFS.format()) {
      Serial.println(F("Format successful"));
    }
    else {
      Serial.println(F("Format also failed - aborting."));
      return;
    }
  }

  int result = database.open(filePath);
  if (result == EDB_ERROR) {
    Serial.print(F("Unable to open database, attempting to create new one - "));
    //Might be a new device without a database, try creating instead.
    result = database.create(filePath, TABLE_SIZE, sizeof(DBRecord) );
    if (result  == EDB_OK) {
      Serial.println("OK");
      database.setDBVersion("DEADBEEF"); //this will force a sync.
    }
    else {
      Serial.println("FAIL");
    }
  }
  else {
    Serial.println(F("Database opened, version "));
    Serial.print(database.DBVersion());
    Serial.print(F(", containing "));
    Serial.print(database.count());
    Serial.println(F(" records"));
  }
}

DBHandler::~DBHandler() {}

//These must be declared here - if you put them in a function, the stack overflows.
WiFiClient wifiClient;
HTTPClient httpClient;

bool DBHandler::sync() {

  Serial.print(F("DB Sync: "));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Abandoned (Wifi not connected)"));
	  return false;
  }

  Serial.print(F("Connecting to server: "));
  Serial.println(syncURL);
  httpClient.begin(wifiClient, syncURL);
  //The Etag needs to be placed in quotes.
  httpClient.addHeader("If-None-Match", String("\"") + String(database.DBVersion()) + String("\""));
  //Use 2 headers from the server  - etag for versioning, and content-length to make sure whole DB is received.
  const char *headerKeys[] = {"ETag", "Content-Length", };
  httpClient.collectHeaders(headerKeys, 2);

  int result = httpClient.GET();

  if (result != 304 && result != 200) {
    Serial.println(F("Bogus HTTP server response  - "));
    Serial.println(result);
    return false;
  }

  //ETag mandatory, as used for database versioning
  if (!httpClient.hasHeader("ETag")) {
    Serial.println(F("Error - ETag header missing. Update failed"));
    return false;
  }
  String id = httpClient.header("ETag");
  //Handle both weak etags and quoted etags, and quoted weak etags...
  if (id.startsWith("W/")) id.remove(0,2); //If it's a weak etag, trim the leading W/
  if (id.length() == 34 && id.startsWith("\"")) id = id.substring(1,33); //If it's quoted, strip the quotes...
  //Now check if the id is the right length
  if (id.length() != 32) {
    Serial.print(F("Invalid DB ETag received from remote server - "));
    Serial.println(id);
    return false;
  }
  const char *newDBID = id.c_str();

  if (result == 304 || !strcmp(database.DBVersion(), newDBID)) {
    Serial.print(F("Server result - "));
    Serial.print(result);
    Serial.println(F(" - DB is in sync"));
    return true;
  }

  //Database update starts here.
  unsigned long bytesExpected = httpClient.header("Content-Length").toInt();
  unsigned long bytesReceived = 0;
  Serial.print(F("Remote version of database is "));
  Serial.println(newDBID);
  EDB_FS tempDB;
  tempDB.create("/tmpDB", TABLE_SIZE, sizeof(DBRecord));
  tempDB.setDBVersion(newDBID);

  WiFiClient *ptr = &wifiClient;

  int errorsDetected = 0;

  while (ptr->available()) {
    String buf = ptr->readStringUntil(REMOTE_DELIM_CHAR);
    const char *p = buf.c_str();
    unsigned int len = buf.length();
    bytesReceived += len + 1; //the +1 is because the delim char is otherwise not counted.
    if (len == sizeof(DBRecord)-1) {
      tempDB.appendRec((unsigned char*)p);
    }
    else {
      Serial.print(F("Error: Garbled data, rejected row "));
      Serial.println(p);
      errorsDetected = 1;
    }
  }

  if (bytesReceived != bytesExpected) {
    Serial.print(F("Error: Incomplete update - bytes expected: "));
    Serial.print(bytesExpected);
    Serial.print(", got ");
    Serial.println(bytesReceived);
    errorsDetected = 1;
  }


  //Do the delete/rename if no errors.
  tempDB.close();
  database.close();
  if (!errorsDetected) {
    //No garbled rows, received all the data.
    Serial.print(F("Received database of "));
    Serial.print(bytesReceived);
    Serial.println(F(" bytes"));
    Serial.println(F("Substituting new database"));
    //Delete the 'real DB', move the temp DB into its' place.
    LittleFS.remove(filePath);
    LittleFS.rename("/tmpDB", filePath);
  }
  else {
    Serial.println(F("Rejecting DB update due to errors"));
    //delete temporary database
    LittleFS.remove("/tmpDB");
  }

  database.open(filePath);

  Serial.print("Database now contains ");
  Serial.print(database.count());
  Serial.print(" records - version ");
  Serial.println(database.DBVersion());
  return true;
}

bool DBHandler::contains(const char *hash) {
  DBRecord rec;
  for (int i=0;  i<database.count(); ++i) {
    EDB_Status result =  database.readRec(i, EDB_REC rec);
    if (result == EDB_OK) {
      if (!strcmp(rec.hash, hash)) {
        Serial.print("Hash ");
        Serial.print(hash);
        Serial.print(" found in database, record no ");
        Serial.println(i);
        return true;
      }
    }
    else {
      //Error
      Serial.print("Error reading record ");
    }
  }
  return false;
}
