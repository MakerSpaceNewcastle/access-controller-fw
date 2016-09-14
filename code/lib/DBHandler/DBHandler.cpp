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

DBHandler::~DBHandler() {

}

bool DBHandler::sync() {
  Serial.println("Checking server database version");
  HTTPClient client;
  client.begin(syncURL);
  client.addHeader("If-None-Match", database.DBVersion());

  const char *headerKeys[] = {"ETag",};
  client.collectHeaders(headerKeys, 1);

  int result = client.GET();

  String id = client.header("ETag");
  if (id.length() == 34) id = id.substring(1,33);
  const char *newDBID = id.c_str();


  if (result != 304 && result != 200) {
    Serial.println("Bogus server response (not 200/304)");
    return false;
  }

  if (result == 304 || !strcmp(database.DBVersion(), newDBID)) {
    Serial.println("DB is in sync");
    return true;
  }

  if (strlen(newDBID) != 32) {
    Serial.print("Invalid DB ETag received from remote server - ");
    Serial.println(newDBID);
    return false;
  }

  Serial.print("Server version of database is ");
  Serial.println(newDBID);
  EDB_FS tempDB;
  tempDB.create("/tmpDB", TABLE_SIZE, sizeof(DBRecord));
  tempDB.setDBVersion(newDBID);

  WiFiClient *ptr = client.getStreamPtr();

  String buf;

  while (ptr->available()) {
    buf = ptr->readStringUntil(REMOTE_DELIM_CHAR);
    const char *p = buf.c_str();

    if (strlen(p) == sizeof(DBRecord)-1) {
      tempDB.appendRec((unsigned char*)p);
    }
    else {
      Serial.print("Rejected row ");
      Serial.println(p);
    }
  }

  //Do the delete/rename
  Serial.println("Substituting databases..");
  tempDB.close();
  database.close();
  SPIFFS.remove(filePath);
  SPIFFS.rename("/tmpDB", filePath);

  database.open(filePath);

  Serial.print("Database now contains ");
  Serial.print(database.count());
  Serial.print(" records - version ");
  Serial.println(database.DBVersion());
  return true;
}

bool DBHandler::inSync() {
    Serial.print("Local DB Version - ");
    Serial.println(database.DBVersion());
    HTTPClient client;
    client.begin(syncURL);
    client.addHeader("If-None-Match", database.DBVersion());

    const char *headerKeys[] = {"Etag",};
    client.collectHeaders(headerKeys, 1);

    int result = client.GET();

    String id = client.header("ETag");
    if (id.length() == 34 ) id = id.substring(1,32); //the etag might be quoted..
    const char *newDBID = id.c_str();

    if (result == 304 || !strcmp(database.DBVersion(), newDBID)) {
      Serial.print("Database is up to date - version ");
      Serial.println(database.DBVersion());
      return true;
    }
    Serial.println("Not up to date (or failed...!");
    return false;
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
