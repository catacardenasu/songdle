#pragma once
#include "Game.h"

#include <string>
#include <vector>

struct BenchmarkConfig {
    int games = 1000;
    unsigned seed = 42;
    int maxGuesses = 8;
    bool verify = true;
    std::string csvPath = "benchmark_results.csv";
};

struct StructureStats {
    std::string name;
    double buildMillis = 0.0;
    int memoryBytes = 0;
    double sumTreeMicros = 0.0;
    double slowestTreeMicros = 0.0;
    int filters = 0;

    double avgTreeMicros() const { return filters ? sumTreeMicros / filters : 0.0; }
};

struct BenchmarkReport {
    int games = 0;
    int solved = 0;
    int totalGuesses = 0;
    int mismatches = 0;  // guesses where the structures disagreed (should be 0)
    double wallSeconds = 0.0;
    std::vector<StructureStats> structures;
};

class Benchmark {
public:
    static BenchmarkReport run(const std::vector<Song>& songs, const std::vector<CategorySets>& sets, const BenchmarkConfig& cfg);
};