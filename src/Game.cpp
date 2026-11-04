#include "Game.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

static std::string toLowercase(const std::string& s) {
    std::string lower;
    lower.reserve(s.size());
    for (char c : s) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return lower;
}
static double timeBetween(const std::chrono::steady_clock::time_point& t0, const std::chrono::steady_clock::time_point& t1) {
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static std::string trim(const std::string& s) {
    int b = 0;
    int e = static_cast<int>(s.size());
    while (b < e && std::isspace(static_cast<unsigned char>(s[b])))
        ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
        --e;
    return s.substr(b, e - b);
}

Game::Game(const std::vector<Song>& songs, const std::vector<CategorySets>& sets, int maxGuesses) : songs_(songs), sets_(sets), maxGuesses_(maxGuesses) {
    if (sets_.empty()) throw std::invalid_argument("Game needs at least one CategorySets");
    titleKey_.reserve(songs_.size());
    artistKey_.reserve(songs_.size());
    for (int i = 0; i < static_cast<int>(songs_.size()); ++i) {
        titleKey_.push_back(toLowercase(songs_[i].name));
        artistKey_.push_back(toLowercase(songs_[i].primaryArtist));
        byTitle_[titleKey_.back()].push_back(i);
    }
}

void Game::start(int mysteryId) {
    if (mysteryId < 0 || mysteryId >= static_cast<int>(songs_.size())) {
        throw std::invalid_argument("start: mysteryId out of range");
    }
    mystery_ = mysteryId;
    won_ = false;
    year_ = Range();
    tempo_ = Range();
    popularity_ = Range();
    requiredArtist_.clear();
    excludedArtists_.clear();
    guessed_.clear();
    history_.clear();
    candidates_.clear();
}
void Game::startRandom(std::mt19937& rng, int minPopularity) {
    std::vector<int> candidates;
    for (int i = 0; i < static_cast<int>(songs_.size()); ++i) {
        if (songs_[i].popularity >= minPopularity) {
            candidates.push_back(i);
        }
    }
    if (candidates.empty()) {
        throw std::runtime_error("startRandom: no songs meet the minimum popularity");
    }
    std::uniform_int_distribution<int> dist(0, static_cast<int>(candidates.size()) - 1);
    start(candidates[dist(rng)]);
}
void Game::tighten(Range& r, double guessValue, int direction) {
    if (direction < 0) {
        r.hi = std::min(r.hi, guessValue - 1e-9);
    } else if (direction > 0) {
        r.lo = std::max(r.lo, guessValue + 1e-9);
    }
}
