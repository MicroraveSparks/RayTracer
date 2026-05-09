#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "Analyzer.h"
#include "testAnalyzer.cpp"
#include <SIMHandler.h>
#include <StorageHandler.h>
#include <Heuristic.h>
#include <Status.h>

#define NUM_HEURISTICS 1

class Analysis {
public:
    Analysis(SIMHandler *sim, HardwareSerial &serialMon = Serial);
    void start();
    void stop();
    
    void getFailed(AnalysisResult* result, size_t &numFailed);
    SafetyStatus checkStatus();
    AnalysisResult get_result(Heuristic heuristic);
private:
    TaskHandle_t loggingTaskHandle;
    int current_logfile = 0;
    SemaphoreHandle_t logMutex;

    void loggingLoop();
    void write_gsmtap_to_pcap(int id, uint8_t *data, size_t length);

    Heuristic heuristicList[NUM_HEURISTICS];
    void refreshHeuristics();
    size_t numEnabled = 0;
    
    TaskHandle_t analysisTaskHandle;
    std::map<Heuristic, AnalysisResult> analysisResults;
    void analysisLoop();
    void runAnalysis(Heuristic heuristic, LTEEvent* events, int numEvents);

    HardwareSerial &SerialMon;
    SIMHandler *sim;
};
#endif // ANALYSIS_H