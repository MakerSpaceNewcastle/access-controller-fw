#ifndef DBHANDLER
#define DBHANDLER

#include <Arduino.h>
#include <FS.h>
#include <EDB_FS.h>
#include <ESP8266HTTPClient.h>

typedef struct DBRecord {
  char hash[33];  //hash is 32 chars + terminating \0
};

#define REMOTE_DELIM_CHAR '\n' //Currently \n terminated lines....

#define MAX_RECORDS 500
#define TABLE_SIZE ( sizeof(DBRecord) * MAX_RECORDS )

class DBHandler {

public:
  DBHandler(const char *, const char *);
  ~DBHandler();
  bool sync();
  bool contains(const char*);

private:
  const char *filePath;
  const char *syncURL;

  EDB_FS database;
};

#endif
