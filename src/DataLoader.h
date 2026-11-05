#pragma once
#include "Song.h"

#include <string>
#include <vector>

struct LoadStats {
    int loaded = 0;     // rows that became songs
    int skipped = 0;    // rows that couldn't be read
    double time = 0.0;  // time to read the file
};

class DataLoader {
public:
    // Reads data/songs_clean.tsv made by scripts/clean_data.py
    static std::vector<Song> load(const std::string& path, LoadStats* stats = nullptr, int limit = 0);
};