#ifndef ANALYZERS_H
#define ANALYZERS_H
#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>

# define ANALYZER_MAX_EVENTS 999

struct LTEEvent {
    String type;
    String data;
    JsonDocument toJson() {
        JsonDocument *doc = new JsonDocument();
        JsonObject obj = doc->to<JsonObject>();
        obj["type"] = type;
        obj["data"] = data;
        return *doc;
    }
};

struct AnalysisResult {
    bool error;
    bool failed;
    String message;
};

class Analyzer
{
protected:
    String name;
    String description;
    LTEEvent *events;
    int numEvents;
    HardwareSerial &SerialMon;
    
    virtual AnalysisResult analyze_event(LTEEvent event) = 0; // Pure virtual function to be implemented by derived classes
public:
    Analyzer(LTEEvent *events, int numEvents, HardwareSerial &serialMon = Serial);
    
    AnalysisResult analyze_events();

    String get_name();
    String get_description();

};


#endif // ANALYZERS_H