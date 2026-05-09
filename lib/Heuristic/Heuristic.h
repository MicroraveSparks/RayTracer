#include <Arduino.h>
#include <map>
#include <ArduinoJson.h>

enum class Heuristic { Test };

Heuristic getHeuristic(String input);
Heuristic *getEnabled(JsonDocument heuristics, Heuristic *heuristicList, size_t &enabledCount);