#include <DBHandler.h>

DBHandler::DBHandler(const char *filename, const char *url) {
  filePath = filename;
  syncURL = url;
  if ( ! SPIFFS.begin()) {
    Serial.println("Unable to init SPIFFS - trying format");
    //Oh dear.  Try formatting it, might be a fresh board.
    if ( SPIFFS.format()) {
      Serial.println("Format successful");
    }
    else {
      Serial.println("Format also failed - aborting.");
      return;
    }
  }

  int result = database.open(filePath);
  if (result == EDB_ERROR) {
    Serial.print("Unable to open database, attempting to create new one - ");
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
    Serial.println("Database opened, version ");
    Serial.print(database.DBVersion());
    Serial.print(", containing ");
    Serial.print(database.count());
    Serial.println(" records");
  }
}

DBHandler::~DBHandler() {}

bool DBHandler::sync() {
  Serial.print("DB Sync: ");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Abandoned (Wifi not connected)");
	  return false;
  }
  
  Serial.print("Connecting to server: ");
  Serial.println(syncURL);
  HTTPClient client;
  client.setTimeout(10);
  client.begin(syncURL);
  //The Etag needs to be placed in quotes.
  client.addHeader("If-None-Match", String("\"") + String(database.DBVersion()) + String("\""));
  //Use 2 headers from the server  - etag for versioning, and content-length to make sure whole DB is received.
  const char *headerKeys[] = {"ETag", "Content-Length", };
  client.collectHeaders(headerKeys, 2);

  int result = client.GET();

  if (result != 304 && result != 200) {
    Serial.println("Bogus HTTP server response  - ");
    Serial.println(result);
    return false;
  }

  //ETag mandatory, as used for database versioning
  if (!client.hasHeader("ETag")) {
    Serial.println("Error - ETag header missing. Update failed");
    return false;
  }
  String id = client.header("ETag");
  //Handle both weak etags and quoted etags, and quoted weak etags...
  if (id.startsWith("W/")) id.remove(0,2); //If it's a weak etag, trim the leading W/
  if (id.length() == 34 && id.startsWith("\"")) id = id.substring(1,33); //If it's quoted, strip the quotes...
  //Now check if the id is the right length
  if (id.length() != 32) {
    Serial.print("Invalid DB ETag received from remote server - ");
    Serial.println(id);
    return false;
  }
  const char *newDBID = id.c_str();

  if (result == 304 || !strcmp(database.DBVersion(), newDBID)) {
    Serial.print("Server result - ");
    Serial.print(result);
    Serial.println(" - DB is in sync");
    return true;
  }

  //Database update starts here.
  unsigned long bytesExpected = client.header("Content-Length").toInt();
  unsigned long bytesReceived = 0;
  Serial.print("Remote version of database is ");
  Serial.println(newDBID);
  EDB_FS tempDB;
  tempDB.create("/tmpDB", TABLE_SIZE, sizeof(DBRecord));
  tempDB.setDBVersion(newDBID);

  WiFiClient *ptr = client.getStreamPtr();

  String buf;
  int errorsDetected = 0;

  while (ptr->available()) {
    buf = ptr->readStringUntil(REMOTE_DELIM_CHAR);
    const char *p = buf.c_str();
    unsigned int len = buf.length();
    bytesReceived += len + 1; //the +1 is because the delim char is otherwise not counted.
    if (len == sizeof(DBRecord)-1) {
      tempDB.appendRec((unsigned char*)p);
    }
    else {
      Serial.print("Error: Garbled data, rejected row ");
      Serial.println(p);
      errorsDetected = 1;
    }
  }

  if (bytesReceived != bytesExpected) {
    Serial.print("Error: Incomplete update - bytes expected: ");
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
    Serial.print("Received database of ");
    Serial.print(bytesReceived);
    Serial.println(" bytes");
    Serial.println("Substituting new database");
    //Delete the 'real DB', move the temp DB into its' place.
    SPIFFS.remove(filePath);
    SPIFFS.rename("/tmpDB", filePath);
  }
  else {
    Serial.println("Rejecting DB update due to errors");
    //delete temporary database
    SPIFFS.remove("/tmpDB");
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
