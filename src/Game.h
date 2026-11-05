#pragma once
#include "TreeBase.h"
#include "Song.h"

#include <chrono>
#include <limits>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// One structure's three categories (year, tempo, popularity) plus build stats
struct CategorySets {
    std::string name;
    TreeBase* year = nullptr;
    TreeBase* tempo = nullptr;
    TreeBase* popularity = nullptr;
    double buildTime = 0.0;

    CategorySets() = default;
    ~CategorySets() {
        delete year;
        delete tempo;
        delete popularity;
    }

    // No copying: two CategorySets holding the same pointers would delete the trees twice.
    CategorySets(const CategorySets&) = delete;
    CategorySets& operator=(const CategorySets&) = delete;

    // Moving hands the trees over and leaves the old CategorySets empty
    // (needed for returning from buildIndexSet and for std::vector<CategorySets>).
    CategorySets(CategorySets&& other) : name(std::move(other.name)), year(other.year), tempo(other.tempo), popularity(other.popularity), buildTime(other.buildTime) {other.year = other.tempo = other.popularity = nullptr;}
    CategorySets& operator=(CategorySets&& other){
        if (this != &other) {
            delete year;
            delete tempo;
            delete popularity;
            name = std::move(other.name);
            year = other.year;
            tempo = other.tempo;
            popularity = other.popularity;
            buildTime = other.buildTime;
            other.year = other.tempo = other.popularity = nullptr;
        }
        return *this;
    }

    int memoryBytes() const {
        return year->memoryBytes() + tempo->memoryBytes() + popularity->memoryBytes();
    }
};

// Builds all three categories with structure type `Category` and times it
template <class Category>
CategorySets buildCategorySet(const std::string& name, const std::vector<Song>& songs) {
    CategorySets s;
    s.name = name;
    auto t0 = std::chrono::steady_clock::now();
    s.year = new Category();
    s.tempo = new Category();
    s.popularity = new Category();
    for (int i = 0; i < static_cast<int>(songs.size()); ++i) {
        s.year->insert(songs[i].year, i);
        s.tempo->insert(songs[i].tempo, i);
        s.popularity->insert(songs[i].popularity, i);
    }
    // One tiny query so any lazy setup is counted as build time, not guess time.
    const double low = std::numeric_limits<double>::lowest();
    s.year->rangeSearch(low, low);
    s.tempo->rangeSearch(low, low);
    s.popularity->rangeSearch(low, low);
    s.buildTime = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return s;
}

struct Feedback {
    bool correct = false;
    bool artistMatch = false;
    int year = 0;
    int tempo = 0;
    int popularity = 0;
};

struct FilterTiming {
    std::string structure;
    int matches = 0;
    double treeMicros = 0.0;   // just the three range searches in the tree
    double totalMicros = 0.0;  // range searches + hash-set intersection + artist filter
};

struct GuessResult {
    int songId = -1;
    Feedback feedback;
    int matches = 0;                    // songs that still match the clue
    std::vector<FilterTiming> timings;  // one per structure
    bool structuresAgree = true;        // every structure returned the same ids
};

class Game {
public:
    Game(const std::vector<Song>& songs, const std::vector<CategorySets>& sets, int maxGuesses = 8);
    void start(int mysteryId);
    // Picks a random mystery song with popularity >= minPopularity
    // (play mode uses ~50 so the song is guessable; benchmark uses 0).
    void startRandom(std::mt19937& rng, int minPopularity = 0);
private:
    struct Range {
        double lo = std::numeric_limits<double>::lowest();
        double hi = std::numeric_limits<double>::max();
    };

    static void tighten(Range& r, double guessValue, int direction);
    std::vector<int> filterWith(const CategorySets& set, FilterTiming& timing) const;
    bool passesOtherClues(int id) const;

    const std::vector<Song>& songs_;
    const std::vector<CategorySets>& sets_;
    int maxGuesses_;
    bool verify_ = true;

    std::vector<std::string> titleKey_;   // lowercase titles
    std::vector<std::string> artistKey_;  // lowercase primary artists
    std::unordered_map<std::string, std::vector<int>> byTitle_;

    int mystery_ = -1;
    bool won_ = false;
    Range year_, tempo_, popularity_;
    std::string requiredArtist_;
    std::unordered_set<std::string> excludedArtists_;
    std::unordered_set<int> guessed_;
    std::vector<GuessResult> history_;
    std::vector<int> candidates_;
};