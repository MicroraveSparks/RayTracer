#include "Analysis.h"

Analysis::Analysis(SIMHandler *sim, HardwareSerial &serialMon) : SerialMon(serialMon) {
    this->sim = sim;
    this->refreshHeuristics();
    for (size_t i = 0; i < NUM_HEURISTICS; i++) {
        this->analysisResults[this->heuristicList[i]] = {false, false, "Not analyzed yet"};
    }
    this->logMutex = xSemaphoreCreateMutex();
    File logDir = SD.open("/logs");
    if (!logDir) {
        SerialMon.println("Failed to open logs directory");
    } else {
        File qmdlDir = SD.open("logs/qmdl");
        File pcapDir = SD.open("logs/pcap");
        if (!qmdlDir) {
            SD.mkdir("logs/qmdl");
        }        if (!pcapDir) {
            SD.mkdir("logs/pcap");
        }
        int max_log = 0;
        File file = qmdlDir.openNextFile();
        for (file; file; file = qmdlDir.openNextFile()) {
            String filename = String(file.name());
            int log_num = filename.substring(0, filename.length() - 5).toInt(); // Assuming log files are named as "X.pcap"
            max_log = max(log_num, max_log);
        }
        this->current_logfile = max_log;
        qmdlDir.close();
    }
}

void Analysis::refreshHeuristics() {
    File heuristicsFile = SD.open("/settings/heuristics.json", FILE_READ);
    if (heuristicsFile) {
      JsonDocument heuristics;
      DeserializationError error = deserializeJson(heuristics, heuristicsFile);
      SerialMon.println("Heuristics configuration loaded successfully:");
      SerialMon.println(heuristics.as<JsonObject>());
    if (!error) {
        getEnabled(heuristics, this->heuristicList, this->numEnabled); // Get enabled heuristics
        for (size_t i = 0; i < this->numEnabled; i++) {
          SerialMon.printf("Enabled heuristic: %d\n", this->heuristicList[i]);
        }
      } else {
        SerialMon.println("Failed to parse heuristics configuration file. Keeping existing configuration.");
      }
      heuristicsFile.close();
    } else {
      SerialMon.println("Failed to open heuristics configuration file. Keeping existing configuration.");
    }
}

void Analysis::start() {
    current_logfile++;
    File qmdlFile = SD.open("/logs/qmdl/" + String(current_logfile) + ".qmdl", FILE_WRITE); // Create new qmdl file for logging
    File pcapFile = SD.open("/logs/pcap/" + String(current_logfile) + ".pcap", FILE_WRITE); // Create new pcap file for logging
    qmdlFile.close();
    pcapFile.close();
    xTaskCreatePinnedToCore(
        [](void* param) {
            static_cast<Analysis*>(param)->loggingLoop();
        },
        "LoggingTask",
        8192,
        this,
        1,
        &this->loggingTaskHandle,
        0
    );
    xTaskCreatePinnedToCore(
        [](void* param) {
            static_cast<Analysis*>(param)->analysisLoop();
        },
        "AnalysisTask",
        8192,
        this,
        1,
        &this->analysisTaskHandle,
        0
    );
}

void Analysis::stop() {
    if (this->loggingTaskHandle) {
        vTaskDelete(this->loggingTaskHandle);
        this->loggingTaskHandle = nullptr;
    }
    if (this->analysisTaskHandle) {
        vTaskDelete(this->analysisTaskHandle);
        this->analysisTaskHandle = nullptr;
    }
}

void Analysis::loggingLoop() {
    SerialMon.println("Logging loop started...");
    while (true) {
        
        uint8_t data[256]; // Placeholder data buffer
        size_t length = 0; // Placeholder data length
        bool error = false; // Simulate error condition for demonstration
        if (!error) {
            int numEvents = 5;
            LTEEvent lteEvents[numEvents] = {
                LTEEvent{"regular_event", "Test data 1"},
                LTEEvent{"regular_event", "Test data 2"},
                LTEEvent{"test_event", "Malicious data"},
                LTEEvent{"regular_event", "Test data 4"},
                LTEEvent{"regular_event", "Test data 5"}
            }; // Placeholder events, replace with actual event data
            JsonDocument jsonEvents;
            for (size_t i = 0; i < numEvents; i++) {
                JsonDocument doc = lteEvents[i].toJson(); // Convert event to JSON format
                this->SerialMon.println("Generated JSON for event " + String(i) + ":");
                serializeJson(doc, this->SerialMon); // For debugging, print the JSON to Serial
                jsonEvents[i] = doc.as<JsonObject>(); // Add JSON object to array
            }
            uint8_t *data;
            size_t length;
            serializeJson(jsonEvents, this->SerialMon); // For debugging, print the serialized JSON to Serial
            
        }
        

        // Simulate receiving GSMTap data and writing to pcap
        xSemaphoreTake(this->logMutex, portMAX_DELAY);
        write_gsmtap_to_pcap(this->current_logfile, data, length);
        xSemaphoreGive(this->logMutex);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay to simulate processing time
    }
}

void Analysis::analysisLoop() {
    while (true) {
        // Placeholder for GSMTap data retrieval logic
        xSemaphoreTake(this->logMutex, portMAX_DELAY);
        // Simulate retrieving GSMTap data from pcap
        // File logfile = SD.open("/logs/" + String(current_logfile) + ".pcap", FILE_READ);
        int numEvents = 5;
        LTEEvent events[numEvents] = {
          LTEEvent{"regular_event", "Test data 1"},
          LTEEvent{"regular_event", "Test data 2"},
          LTEEvent{"test_event", "Malicious data"},
          LTEEvent{"regular_event", "Test data 4"},
          LTEEvent{"regular_event", "Test data 5"}
        }; // Placeholder events, replace with actual event data
        xSemaphoreGive(this->logMutex);

        // Simulate analysis of retrieved GSMTap data
        for (size_t i = 0; i < this->numEnabled; i++) {
            Heuristic heuristic = this->heuristicList[i];
            runAnalysis(heuristic, events, numEvents);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay to simulate processing time
    }
}

void Analysis::write_gsmtap_to_pcap(int id, uint8_t *data, size_t length) {
    File pcapFile = SD.open("/logs/" + String(current_logfile) + ".pcap", FILE_APPEND);
    if (pcapFile) {
        // Write GSMTap data to pcap file
        pcapFile.write(data, length);
        pcapFile.close();
    } else {
        SerialMon.println("Failed to open pcap file for writing");
    }
}

void Analysis::runAnalysis(Heuristic heuristic, LTEEvent* events, int numEvents) {
    switch (heuristic) {
        case Heuristic::Test:
            this->analysisResults[heuristic] = TestAnalyzer(events, numEvents, this->SerialMon).analyze_events();
            break;
        default:
            // Handle unknown heuristic case if necessary
            this->SerialMon.println("Unknown heuristic. Skipping analysis.");
            this->analysisResults[heuristic] = {true, false, "Unknown heuristic"};
            break;
    }
}

void Analysis::getFailed(AnalysisResult* result, size_t &numFailed) {
    numFailed = 0;
    for (const auto& pair : this->analysisResults) {
        if (pair.second.failed) {
            result[numFailed++] = pair.second;
        }
    }
}

SafetyStatus Analysis::checkStatus() {
    if (this->analysisTaskHandle == nullptr && this->loggingTaskHandle == nullptr) {
        return SafetyStatus::Paused; // If the analysis and logging tasks are not running, return paused
    }
    else if (this->analysisTaskHandle == nullptr || this->loggingTaskHandle == nullptr) {
        return SafetyStatus::Error; // If one of the tasks is not running, return error
    }
    for (const auto& pair : this->analysisResults) {
        if (pair.second.failed) {
            return SafetyStatus::Unsafe; // If any heuristic has a failed check, return unsafe
        }
        else if (pair.second.error) {
            return SafetyStatus::Error; // If any heuristic has an error, return error
        }
    }
    return SafetyStatus::Safe; // All heuristics passed without errors
}
