#ifndef DBHANDLER
#define DBHANDLER

#include <Arduino.h>
#include <LittleFS.h>
#include <EDB_FS.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#define DEBUG
#ifdef DEBUG
  #define DEBUG_PRINT(x)  Serial.println (x)
#else
  #define DEBUG_PRINT(x)
#endif


typedef struct DBRecord {
  char hash[33];  //hash is 32 chars + terminating \0
};

#define REMOTE_DELIM_CHAR '\n' //Currently \n terminated lines....

#define MAX_RECORDS 500
#define TABLE_SIZE ( sizeof(DBRecord) * MAX_RECORDS )

class DBHandler {

public:
  DBHandler(const char *, const char *, const char *);
  ~DBHandler();
  bool sync();
  bool contains(const char*);

private:
  const char *filePath;
  const char *verURL;
  const char *syncURL;

  EDB_FS database;
};

#endif
