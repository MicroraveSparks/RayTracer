#include <Arduino.h>
#include <map>

#include <ServerHandler.h>
#include <StorageHandler.h>
#include <SIMHandler.h>
#include <Analysis.h>

int led = 15;

#define SerialMon Serial
#define GSM_BAUD_RATE 9600 // Set to disable auto-baud and use this fixed baud rate
SoftwareSerial SerialAT(16, 17); // RX, TX pins for SIM800L communication
SIMHandler *sim;

const int CS = 5;

JsonDocument setupHeuristics();
void runHeuristics(JsonDocument heuristics);
Heuristic getHeuristic(String input);

Analysis *analyzer;

Status RayTracerStatus(false, SafetyStatus::Paused);

const char* ssid = "raydar";
const char* password = "raytracer";
const char* admin = "admin";
const char* adminPassword = "raytracer";
String encryptionKey = "my_secret_key";
int port = 8080;
RayTracerWebServer* portal = new RayTracerWebServer(RayTracerStatus, port, SerialMon, led);

enum class Command { ListDir, WiFi, Help, Unknown };
std::map<String, Command> commandMap = {
  {String("ls"), Command::ListDir},
  {String("wifi"), Command::WiFi},
  {String("Help"), Command::Help}
};  

Command getCommand(String input);

void printWiFiInfo();

void setup() {
  SerialMon.begin(115200); // Start the serial communication at 115200 baud rate
  delay(500);
  while (!SerialMon) { ; /* Wait for the serial port to connect */ }
  sim = new SIMHandler(SerialMon, SerialAT);
  
  // Initialize status LED pin as an output
  SerialMon.println("Initializing LED...");
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  
  if (!sim->isInitialized()) {
    SerialMon.println("Failed to initialize SIM module");
    while (true) { delay(1000); } // Halt execution if SIM module fails to initialize
  }
  
  SerialMon.println("SIM module initialized successfully");
  JsonDocument modemInfo = sim->getModemInfo();
  SerialMon.println("Modem Info:"); 
  for (JsonPair pair : modemInfo.as<JsonObject>()) {
    SerialMon.printf("%s: %s\n", pair.key().c_str(), pair.value().as<String>().c_str());
  }
  
  // Initialize SD card with specified parameters
  initStorage(CS);
  SerialMon.print(listDir("/"));
  
  SerialMon.println("Setting up Heuristics...");
  if (!SD.open("/settings")) {
    SD.mkdir("/settings");
  }
  
  File heuristicsFile = SD.open("/settings/heuristics.json", FILE_READ);
  JsonDocument Heuristics;
  if (!heuristicsFile) {
    SerialMon.println("Heuristics configuration file not found on SD card. Creating default heuristics configuration.");
    JsonDocument defaultHeuristics = setupHeuristics();
    
    File newFile = SD.open("/settings/heuristics.json", FILE_WRITE);
    if (!newFile) {
      SerialMon.println("Failed to create heuristics configuration file on SD card.");
      while (true) { delay(1000); } // Halt execution if heuristics configuration file cannot be created
    }
    serializeJson(defaultHeuristics, newFile);
    newFile.close();
    Heuristics = defaultHeuristics;
  } else {
    DeserializationError error = deserializeJson(Heuristics, heuristicsFile);
    heuristicsFile.close();
    if (error) {
      SerialMon.println("Failed to parse heuristics configuration file. Using empty heuristics configuration.");
      Heuristics.clear();
    } else {
      if (Heuristics.isNull() || !Heuristics.is<JsonObject>()) {
        SerialMon.println("Heuristics configuration file is not a valid JSON object. Rebuilding heuristics configuration.");
        Heuristics = setupHeuristics();
      }
      for (JsonPair pair : Heuristics.as<JsonObject>()) {
        JsonObject heuristic = pair.value().as<JsonObject>();
        SerialMon.printf("Loaded heuristic '%s' with enabled state: %s\n", heuristic["name"].as<String>().c_str(), heuristic["enabled"].as<bool>() ? "true" : "false");
      }
      SerialMon.println("Heuristics configuration loaded successfully:");
    }
    heuristicsFile = SD.open("/settings/heuristics.json", FILE_WRITE);
    if (!heuristicsFile) {
      SerialMon.println("Failed to open heuristics configuration file for writing. Changes to heuristics will not be saved.");
    } else {
      serializeJson(Heuristics, heuristicsFile);
      heuristicsFile.close();
    }
  }

  if(!SD.open("/logs")) {
    SD.mkdir("/logs"); // Make logging directory if it does not yet exist
  } else {
    SerialMon.println("Logging directory found on SD card.");
    SerialMon.println(listDir("/logs"));
  }

  analyzer = new Analysis(sim, SerialMon);

  File www = SD.open("/www");
  if (!www || !www.isDirectory()) {
    SerialMon.println("Failed to open WebServer Source directory on SD card. please ensure to copy the constents of raydar/data/www to the sd card in the /www/ directory before running");
    while (true) { delay(1000); } // Halt execution if WebServer Source directory cannot be opened
  }
  SerialMon.print(listDir("/www"));
  SerialMon.print(listDir("/certs"));
  
  // Start the web server
  SerialMon.println("Starting web server...");
  User users[MAX_ACCOUNTS] = {
    {admin, hash_password(adminPassword), true},
  };
  portal->setup_server(ssid, password, users, encryptionKey);
  portal->begin();
  if (portal == nullptr) {
    SerialMon.println("Failed to start server");
    while (true) { delay(1000); } // Halt execution if server fails to start
  }
  SerialMon.printf("HTTP server started at http://%s:%d\n", WiFi.softAPIP().toString().c_str(), port);
  
  portal->setModemInfo(modemInfo);
  //Serial.print("Enter a Command > ");
}

JsonDocument setupHeuristics() {
  JsonDocument defaultHeuristics;
  JsonObject testHeuristic = defaultHeuristics["test_heuristic"].to<JsonObject>();
  testHeuristic["name"] = "Test Heuristic";
  testHeuristic["enabled"] = false;
  return defaultHeuristics;
}

Command getCommand(String input) {
    auto it = commandMap.find(input);
    return (it != commandMap.end()) ? it->second : Command::Unknown;
}

void loop() {
  if (RayTracerStatus.isRunning()) {
    if (RayTracerStatus.getSafetyStatus() == SafetyStatus::Paused) {
      SerialMon.println("Raytracer is running. Starting Analyzer...");
      analyzer->start();
    }
    SafetyStatus currentStatus = analyzer->checkStatus();
    RayTracerStatus.setSafetyStatus(currentStatus);
  } else {
    if (RayTracerStatus.getSafetyStatus() != SafetyStatus::Paused) {
      SerialMon.println("Raytracer is paused. Stopping Analyzer...");
      analyzer->stop();
      RayTracerStatus.setSafetyStatus(SafetyStatus::Paused);
    }
  }
  delay(5000); // Wait for 5 seconds before checking status again
}
/*
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    Serial.println("\nReceived command: " + command);
    // Here you can add code to handle different commands
    digitalWrite(led, HIGH); // Turn LED on
    switch (getCommand(command))
    {
    case Command::ListDir: listDir("/"); break;           // List files in the root directory
    case Command::WiFi: printWiFiInfo(); break;
    default:
        Serial.println("Unknown command");
      break;
    }
    delay(1000);  // Wait 1 second
    digitalWrite(led, LOW);  // Turn LED off
    delay(1000);
    Serial.print("Enter a Command > ");
  }
*/

void printWiFiInfo(){
  Serial.printf("SSID: %s\tIP Address: %s\tPassword: %s\n", ssid, portal->getIP().toString().c_str(), password);
}