#include <DBHandler.h>

DBHandler::DBHandler(const char *filename, const char *verUrl, const char *syncUrl) {
  filePath = filename;
  verURL = verUrl;
  syncURL = syncUrl;
  
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

  //First, we get the DB version from the server.
  DEBUG_PRINT(F("Connecting to server to get Db Version: "));
  DEBUG_PRINT(verURL);

  httpClient.begin(wifiClient, verURL);

  int result = httpClient.GET();
  if (result != 200) {
    DEBUG_PRINT(F("Bogus HTTP server response  - "));
    DEBUG_PRINT(result);
    return false;
  }

  String dbVersion = httpClient.getString();
  DEBUG_PRINT(F("Got DB version of "));
  DEBUG_PRINT(dbVersion);

  if (dbVersion.length() >32) {
    DEBUG_PRINT("Hash length too long - aborting");
    return false;
  }

  if (!strcmp(database.DBVersion(), dbVersion.c_str())) {
    DEBUG_PRINT(F("Server result - "));
    DEBUG_PRINT(result);
    DEBUG_PRINT(F(" - DB is in sync"));
    return true;
  }
  //End this request.
  httpClient.end();

  DEBUG_PRINT(F("DB update in progress, connecting to "));
  DEBUG_PRINT(syncURL);
  //Get the database itself now, we need to update.
  httpClient.begin(wifiClient, syncURL);
  result = httpClient.GET();

  if (result != 200) {
    DEBUG_PRINT(F("Bogus HTTP server response  - "));
    DEBUG_PRINT(result);
    return false;
  }
  DEBUG_PRINT(F("Beginning update"));


  //Database update starts here.
  unsigned long bytesExpected = httpClient.getSize();;
  DEBUG_PRINT(F("Expected bytes:"));
  DEBUG_PRINT (bytesExpected);

  unsigned long bytesReceived = 0;
  EDB_FS tempDB;
  tempDB.create("/tmpDB", TABLE_SIZE, sizeof(DBRecord));
  tempDB.setDBVersion(dbVersion.c_str());

  WiFiClient *ptr = httpClient.getStreamPtr();

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
      DEBUG_PRINT(F(" got length "));
      DEBUG_PRINT(len);
      DEBUG_PRINT(F(", expected: "));
      DEBUG_PRINT(sizeof(DBRecord)-1);

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
  for (unsigned int i=0;  i<database.count(); ++i) {
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
