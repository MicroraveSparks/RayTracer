#ifndef SIMHANDLER_H
#define SIMHANDLER_H

#include <SoftwareSerial.h>
#include <StreamDebugger.h>

#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 57600
//#define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1Kb


#include <ArduinoJSON.h>

String commandCleaner(String input);

class SIMHandler {
public:
    SIMHandler(HardwareSerial &serialMon, SoftwareSerial &serialAT, bool debugEnabled = false);
    bool isInitialized();
    JsonDocument getModemInfo();
    bool hasNewData();
private:
    TinyGsm *modem;
    bool debugEnabled = false;
    HardwareSerial &SerialMon;
    SoftwareSerial &SerialAT;
    void initSIM();
};

#endif // SIMHANDLER_H