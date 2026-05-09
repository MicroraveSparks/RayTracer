#include "Analyzer.h"

class TestAnalyzer : public Analyzer {
public:
    TestAnalyzer(LTEEvent* events, int numEvents, HardwareSerial &serialMon) : Analyzer(events, numEvents, serialMon) {
        this->name = "Test Analyzer";
        this->description = "A simple test analyzer for demonstration purposes.";
    }
    AnalysisResult analyze_event(LTEEvent event) override {
        AnalysisResult result;
        if (event.type == "test_event") {
            this->SerialMon.println("Analyzing test event: " + event.data);
            result.error = false;
            result.failed = true; // Simulate a failed check for demonstration
            result.message = "Test event detected! Check failed.";
        } else if (event.type == "regular_event") {
            this->SerialMon.println("Analyzing regular event: " + event.data);
            result.error = false;
            result.failed = false;
            result.message = "Regular event, check passed.";
        } else {
            result.error = true;
            result.failed = false;
            result.message = "Unknown event type.";
        }
        return result;
    }
};