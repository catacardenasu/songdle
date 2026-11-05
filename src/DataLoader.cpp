#include "DataLoader.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>

std::vector<Song> DataLoader::load(const std::string& path, LoadStats* stats, int limit) {
    auto start = std::chrono::steady_clock::now();

    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("Could not open " + path);

    LoadStats local;
    std::vector<Song> songs;
    songs.reserve(600000);

    std::string line;
    std::getline(in, line);  // skip the header row
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();  // Windows line endings

        std::stringstream row(line);
        Song s;
        std::string year, popularity, tempo;
        std::getline(row, s.name, '\t');
        std::getline(row, s.artist, '\t');
        std::getline(row, year, '\t');
        std::getline(row, popularity, '\t');
        std::getline(row, tempo, '\t');

        try {
            s.year = std::stoi(year);
            s.popularity = std::stoi(popularity);
            s.tempo = std::stod(tempo);
        } catch (const std::exception&) {
            ++local.skipped;
            continue;
        }
        songs.push_back(s);
        if (limit > 0 && static_cast<int>(songs.size()) >= limit) break;
    }

    local.loaded = static_cast<int>(songs.size());
    local.time = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    if (stats)
        *stats = local;
    return songs;
}