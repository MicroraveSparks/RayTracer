#ifndef STORAGEHANDLER_H
#define STORAGEHANDLER_H
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>

String listDir(const char *path);
void initStorage(int CS);
JsonDocument *logList();

#endif // STORAGE_HANDLER_H