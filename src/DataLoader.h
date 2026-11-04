#pragma once
#include "Song.h"
#include <cstddef>
#include <string>
#include <vector>
using namespace std;

struct LoadStats {
    int rowsRead = 0;  // data rows in the file
    int loaded = 0;    // rows accepted and loaded
    int skipped = 0;   // rows with missing info or invalid data
    double time = 0.0;       // time to read and parse in ms
};

class DataLoader {
public:
    // reads tracks.csv
    //  name | artists or artist | release_date or year | popularity | tempo
    // `limit` > 0 stops after that many songs (handy for quick tests).
    // Throws std::runtime_error if the file can't be opened or a column is missing.
    static vector<Song> load(const string& path, LoadStats* stats = nullptr, int limit = 0);

    // parses the csv line into a vector of strings, handling quoted commas and escaped quotes
    static vector<string> parseCsvLine(const string& line);

    // turns the multiple artists "['A', \"B's\"]" into {"A", "B's"}
    static vector<string> parseArtistList(const string& raw);
};