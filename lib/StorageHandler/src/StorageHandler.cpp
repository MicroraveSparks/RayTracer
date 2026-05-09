#include "StorageHandler.h"

void initStorage(int CS) {
  if(!SD.begin(CS)){
    Serial.println("Card Mount Failed");
    while (true) { delay(1000); } // Halt execution if SD card fails to initialize
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    while (true) { delay(1000); } // Halt execution if no SD card is detected
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
}

String listDir(const char *path) {
  String result = "";
   // List all files in the root directory of the SD card
  result += "Listing directory: " + String(path) + "\n";

  File root = SD.open(path);
  if(!root){
    result += "Failed to open directory\n";
    return result;
  }
  if(!root.isDirectory()){
    result += "Not a directory\n";
    return result;
  }
  
  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      result += "  DIR : " + String(file.name()) + "\n";
    } else {
      result += "  FILE: " + String(file.name()) + "  SIZE: " + String(file.size()) + "\n";
    }
    file = root.openNextFile();
  }
  return result;
}

JsonDocument *logList() {
  JsonDocument *doc = new JsonDocument();
  JsonArray files = doc->to<JsonArray>();
  File root = SD.open("/logs");
  if(!root || !root.isDirectory()){
    return doc; // Return empty document if directory cannot be opened or is not a directory
  }
  else {
    File file = root.openNextFile();
    while(file){
      JsonDocument *logObj = new JsonDocument();
      JsonObject fileObj = logObj->to<JsonObject>();
      if(!file.isDirectory()){
        String fname = file.name();
        int id = fname.substring(0, fname.lastIndexOf('.')).toInt(); // Use filename without extension as ID
        time_t timestamp = file.getLastWrite();
        fileObj["id"] = id;
        fileObj["name"] = file.name();
        fileObj["size"] = file.size();
        fileObj["timestamp"] = timestamp;
        files.add(fileObj);
      }
      serializeJson(*logObj, Serial);
      file = root.openNextFile();
    }
  }
  return doc;
}