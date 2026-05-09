#include "SIMHandler.h"

SIMHandler::SIMHandler(HardwareSerial &serialMon, SoftwareSerial &serialAT, bool debugEnabled) : SerialMon(serialMon), SerialAT(serialAT), debugEnabled(debugEnabled) {
    this->debugEnabled = debugEnabled;
    initSIM();
    int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
    float timezone = 0;
    try {
        this->modem->getNetworkTime(&year, &month, &day, &hour, &min, &sec, &timezone);
        this->SerialMon.printf("Current time: %04d-%02d-%02d %02d:%02d:%02d (UTC%+.1f)\n", year, month, day, hour, min, sec, timezone);
    } catch (const std::exception& e) {
        DBG("Error retrieving time: " + String(e.what()));
    }
}

bool SIMHandler::isInitialized() {
    return modem != nullptr;
}

String commandCleaner(String input) {
    String commandPrefix = "AT";
    String commandsSeparator = "  ";
    int start = input.indexOf(commandPrefix); // Find the start of the AT command
    while (start != -1) {
        DBG("Cleaning command: %s\n", input.c_str());
        int len = input.indexOf(commandsSeparator, start) + commandsSeparator.length(); // Find the end of the command
        if (len != -1) {
            input.remove(start, len); // Remove the AT command
        } else {
            break; // Skip if we cant find the end of the command
        }
        DBG("Cleaned command: %s\n", input.c_str());
        start = input.indexOf(commandPrefix); // Find the next AT command
    }
    input.trim(); // Remove leading and trailing whitespace
    return input;
}

void SIMHandler::initSIM(){
    this->SerialMon.println("Initializing SIM800L...");
    
    TinyGsm *modem;
    if (debugEnabled) {
        this->SerialMon.println("Debug mode enabled: SIM800L communication will be logged to Serial Monitor.");
        StreamDebugger debugger(this->SerialAT, this->SerialMon);
        modem = new TinyGsm(debugger);
    } else {
        this->SerialMon.println("Debug mode disabled: SIM800L communication will not be logged.");
        modem = new TinyGsm(this->SerialAT);
    }
    
    #ifdef GSM_BAUD_RATE
        modem->setBaud(GSM_BAUD_RATE);
    #else
        uint32_t baud = TinyGsmAutoBaud(this->SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX); // Auto-baud to match SIM800L baud rate
    #endif

    
    this->SerialMon.println("SIM800L initialized successfully with baud rate: " + String(baud) + " bps");

    if (this->debugEnabled) {    
        String modemInfo = commandCleaner(modem->getModemModel());
        this->SerialMon.println("Modem Model: " + modemInfo);
        
        String name = commandCleaner(modem->getModemName());
        this->SerialMon.println("Modem Name: " + name);
        
        String manufacturer = commandCleaner(modem->getModemManufacturer());
        this->SerialMon.println("Modem Manufacturer: " + manufacturer);
        
        String hw_ver = commandCleaner(modem->getModemModel());
        this->SerialMon.println("Modem Hardware Version: " + hw_ver);
        
        String fv_ver = commandCleaner(modem->getModemRevision());
        this->SerialMon.println("Modem Firmware Version: " + fv_ver);
    }
    
    this->modem = modem;
}

JsonDocument SIMHandler::getModemInfo() {
    JsonDocument *doc = new JsonDocument();
    JsonObject modemInfo = doc->to<JsonObject>();
    modemInfo["manufacturer"] = commandCleaner(this->modem->getModemManufacturer());
    modemInfo["model"] = commandCleaner(this->modem->getModemModel());
    modemInfo["firmware_version"] = commandCleaner(this->modem->getModemRevision());
    modemInfo["imei"] = commandCleaner(this->modem->getIMEI());
    modemInfo["imsi"] = commandCleaner(this->modem->getIMSI());
    return *doc;
}

bool SIMHandler::hasNewData() {
    return this->SerialAT.available() > 0;
}