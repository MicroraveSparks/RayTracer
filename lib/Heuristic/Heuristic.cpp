#include "Heuristic.h"

std::map<String, Heuristic> heuristicMap = {
  {String("test_heuristic"), Heuristic::Test}
};

Heuristic getHeuristic(String input) {
    auto it = heuristicMap.find(input);
    return (it != heuristicMap.end()) ? it->second : Heuristic::Test; // Default to Test heuristic if not found
}

Heuristic *getEnabled(JsonDocument heuristics, Heuristic *heuristicList, size_t &enabledCount) {
  enabledCount = 0;
  for (JsonPair pair : heuristics.as<JsonObject>()) {
    JsonObject heuristic = pair.value().as<JsonObject>();
    if (heuristic["enabled"] && heuristic["enabled"].as<bool>()) {
      Heuristic h = getHeuristic(pair.key().c_str());
      heuristicList[enabledCount++] = h;
    }
  }
  return heuristicList;
}