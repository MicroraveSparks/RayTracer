#include "Analyzer.h"

Analyzer::Analyzer(LTEEvent *events, int numEvents, HardwareSerial &serialMon) : name("Unnamed Analyzer"), description("No description provided"), numEvents(numEvents), SerialMon(serialMon) {
    // For demonstration, we'll just take the first event. In a real implementation, you might want to store all events or process them differently.
    this->events = new LTEEvent[numEvents];
    for (int i = 0; i < numEvents; i++) {
        this->events[i] = events[i];
    }
}

AnalysisResult Analyzer::analyze_events() {
    AnalysisResult finalResult;
    finalResult.error = false;
    finalResult.failed = true;
    finalResult.message = "Events passed analysis.";
    
    for (int i = 0; i < numEvents; i++) {
        AnalysisResult result = analyze_event(events[i]);
        if (result.error) {
            finalResult.error = true;
            finalResult.failed = true;
            finalResult.message += "\nError in event " + String(i) + ": " + result.message;
        } else if (!result.failed) {
            finalResult.failed = true;
            finalResult.message += "\nCheck failed in event " + String(i) + ": " + result.message;
        }
    }
    return finalResult;
}

String Analyzer::get_name() {
    return this->name;
}

String Analyzer::get_description() {
    return this->description;
}