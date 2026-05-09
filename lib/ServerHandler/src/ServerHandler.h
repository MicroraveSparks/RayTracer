#ifndef SERVER_HANDLER_H
#define SERVER_HANDLER_H

#include <WiFi.h>
#include <SD.h>
#include <ESPAsyncWebServer.h>
#include <User.h>
#include <mbedtls/base64.h>
#include <mbedtls/pk.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <ArduinoJson.h>
#include <Status.h>
#include <StorageHandler.h>

#define SESSION_TIMEOUT 3600 // in seconds
#define MAX_ACCOUNTS 10

class RayTracerWebServer : public AsyncWebServer {
public:
    RayTracerWebServer(Status &currentStatus, int port, HardwareSerial &serial = Serial, int outputPin = 15);
    void setup_server(const char* ssid, const char* password, User *users, String encryptionKey);
    void setModemInfo(JsonDocument info);
    time_t now;
    
    struct Session {
        User* user;
        String token;
        time_t expiry;
    };
    Session sessions[MAX_ACCOUNTS];
    
    IPAddress getIP();
    
private:
    HardwareSerial &SerialMon;
    int output = 15;

    Status &raytracerStatus;
    
    JsonDocument fetchHeurtistics();
    JsonDocument modemInfo;

    User users[MAX_ACCOUNTS];
    mbedtls_pk_context pk_context;
    String HeuristicToggle(String heuristicsName, String id, bool enabled);
    String alertJS(String message, String redirect, int timeout);
    String alertHTML(String message, String redirect, int timeout);

    AsyncMiddlewareFunction *authMiddleware;
    AsyncMiddlewareFunction *adminMiddleware;

    bool generate_certs(String encryptionKey);
    void setupRoutes();
    
    String generate_token(int userIndex);
    Session check_auth(AsyncWebServerRequest *request);
    
    // Replaces placeholder with button section in your web page
    String processor(const String& var);
};




#endif // SERVER_HANDLER_H