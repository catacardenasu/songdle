#pragma once
#include <string>

struct Song {
    std::string name;    // title
    std::string artist;  // first listed artist
    int year = 0;        // release year
    int popularity = 0;  // 0-100
    double tempo = 0.0;  // beats per minute
};