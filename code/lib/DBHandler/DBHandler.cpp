#include <DBHandler.h>


DBHandler::DBHandler(const char *filename, const char *url) {
  filePath = filename;
  syncURL = url;
  if ( ! LittleFS.begin()) {
    DEBUG_PRINT(F("Unable to init LittleFS - trying format"));
    //Oh dear.  Try formatting it, might be a fresh board.
    if ( LittleFS.format()) {
      DEBUG_PRINT(F("Format successful"));
    }
    else {
      DEBUG_PRINT(F("Format also failed - aborting."));
      return;
    }
  }

  int result = database.open(filePath);
  if (result == EDB_ERROR) {
    DEBUG_PRINT(F("Unable to open database, attempting to create new one - "));
    //Might be a new device without a database, try creating instead.
    result = database.create(filePath, TABLE_SIZE, sizeof(DBRecord) );
    if (result  == EDB_OK) {
      DEBUG_PRINT("OK");
      database.setDBVersion("DEADBEEF"); //this will force a sync.
    }
    else {
      DEBUG_PRINT("FAIL");
    }
  }
  else {
    DEBUG_PRINT(F("Database opened, version "));
    DEBUG_PRINT(database.DBVersion());
    DEBUG_PRINT(F(", containing "));
    DEBUG_PRINT(database.count());
    DEBUG_PRINT(F(" records"));
  }
}

DBHandler::~DBHandler() {}

//These must be declared here - if you put them in a function, the stack overflows.
WiFiClient wifiClient;
HTTPClient httpClient;

bool DBHandler::sync() {

  DEBUG_PRINT(F("DB Sync: "));
  
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT(F("Abandoned (Wifi not connected)"));
	  return false;
  }

  DEBUG_PRINT(F("Connecting to server: "));
  DEBUG_PRINT(syncURL);
  httpClient.begin(wifiClient, syncURL);
  //The Etag needs to be placed in quotes.
  httpClient.addHeader("If-None-Match", String("\"") + String(database.DBVersion()) + String("\""));
  //Use 2 headers from the server  - etag for versioning, and content-length to make sure whole DB is received.
  const char *headerKeys[] = {"ETag", "Content-Length", };
  httpClient.collectHeaders(headerKeys, 2);

  int result = httpClient.GET();

  if (result != 304 && result != 200) {
    DEBUG_PRINT(F("Bogus HTTP server response  - "));
    DEBUG_PRINT(result);
    return false;
  }

  //ETag mandatory, as used for database versioning
  if (!httpClient.hasHeader("ETag")) {
    DEBUG_PRINT(F("Error - ETag header missing. Update failed"));
    return false;
  }
  String id = httpClient.header("ETag");
  //Handle both weak etags and quoted etags, and quoted weak etags...
  if (id.startsWith("W/")) id.remove(0,2); //If it's a weak etag, trim the leading W/
  if (id.length() == 34 && id.startsWith("\"")) id = id.substring(1,33); //If it's quoted, strip the quotes...
  //Now check if the id is the right length
  if (id.length() != 32) {
    DEBUG_PRINT(F("Invalid DB ETag received from remote server - "));
    DEBUG_PRINT(id);
    return false;
  }
  const char *newDBID = id.c_str();

  if (result == 304 || !strcmp(database.DBVersion(), newDBID)) {
    DEBUG_PRINT(F("Server result - "));
    DEBUG_PRINT(result);
    DEBUG_PRINT(F(" - DB is in sync"));
    return true;
  }

  //Database update starts here.
  unsigned long bytesExpected = httpClient.header("Content-Length").toInt();
  unsigned long bytesReceived = 0;
  DEBUG_PRINT(F("Remote version of database is "));
  DEBUG_PRINT(newDBID);
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
      DEBUG_PRINT(F("Error: Garbled data, rejected row "));
      DEBUG_PRINT(p);
      errorsDetected = 1;
    }
  }

  if (bytesReceived != bytesExpected) {
    DEBUG_PRINT(F("Error: Incomplete update - bytes expected: "));
    DEBUG_PRINT(bytesExpected);
    DEBUG_PRINT(", got ");
    DEBUG_PRINT(bytesReceived);
    errorsDetected = 1;
  }


  //Do the delete/rename if no errors.
  tempDB.close();
  database.close();
  if (!errorsDetected) {
    //No garbled rows, received all the data.
    DEBUG_PRINT(F("Received database of "));
    DEBUG_PRINT(bytesReceived);
    DEBUG_PRINT(F(" bytes"));
    DEBUG_PRINT(F("Substituting new database"));
    //Delete the 'real DB', move the temp DB into its' place.
    LittleFS.remove(filePath);
    LittleFS.rename("/tmpDB", filePath);
  }
  else {
    DEBUG_PRINT(F("Rejecting DB update due to errors"));
    //delete temporary database
    LittleFS.remove("/tmpDB");
  }

  database.open(filePath);

  DEBUG_PRINT("Database now contains ");
  DEBUG_PRINT(database.count());
  DEBUG_PRINT(" records - version ");
  DEBUG_PRINT(database.DBVersion());
  return true;
}

bool DBHandler::contains(const char *hash) {
  DBRecord rec;
  for (int i=0;  i<database.count(); ++i) {
    EDB_Status result =  database.readRec(i, EDB_REC rec);
    if (result == EDB_OK) {
      if (!strcmp(rec.hash, hash)) {
        DEBUG_PRINT("Hash ");
        DEBUG_PRINT(hash);
        DEBUG_PRINT(" found in database, record no ");
        DEBUG_PRINT(i);
        return true;
      }
    }
    else {
      //Error
      DEBUG_PRINT("Error reading record ");
    }
  }
  return false;
}
