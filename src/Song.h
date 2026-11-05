#pragma once
#include <string>

struct Song {
    std::string name;
    std::string artists; //all artists with ", "
    std::string primaryArtist; // artist that pops up first for clue
    int year = 0;
    int popularity = 0; //0-100
    double tempo = 0.0; //bpm
};