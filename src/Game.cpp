#include "Game.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

//helper fucntions
static std::string toLowercase(const std::string& s) {
    std::string lower;
    lower.reserve(s.size());
    for (char c : s) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return lower;
}
static double timeBetween(const std::chrono::steady_clock::time_point& t0, const std::chrono::steady_clock::time_point& t1) {
    return std::chrono::duration<double, std::micro>(t1 - t0).count();
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
// 1 = mystery is higher, -1 = mystery is lower, 0 = same
static int compare(double mystery, double guess) {
    if (mystery > guess) 
        return 1;
    if (mystery < guess) 
        return -1;
    return 0;
}


// Game funcitons
Game::Game(const std::vector<Song>& songs, const std::vector<CategorySets>& sets, int maxGuesses) : songs_(songs), sets_(sets), maxGuesses_(maxGuesses) {
    if (sets_.empty()) throw std::invalid_argument("Game needs at least one CategorySets");
    titleKey_.reserve(songs_.size());
    artistKey_.reserve(songs_.size());
    for (int i = 0; i < static_cast<int>(songs_.size()); ++i) {
        titleKey_.push_back(toLowercase(songs_[i].name));
        artistKey_.push_back(toLowercase(songs_[i].artist));
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
    const double inf = std::numeric_limits<double>::infinity();
    if (direction == 1) {
        r.lo = std::max(r.lo, std::nextafter(guessValue, inf));
    } else if (direction == -1) {
        r.hi = std::min(r.hi, std::nextafter(guessValue, -inf));
    } else {
        r.lo = std::max(r.lo, guessValue);
        r.hi = std::min(r.hi, guessValue);
    }
}
std::vector<int> Game::filterWith(const CategorySets& set, FilterTiming& timing) const {
    auto t0 = std::chrono::steady_clock::now();
    std::vector<int> yearMatches = set.year->rangeSearch(year_.lo, year_.hi);
    std::vector<int> tempoMatches = set.tempo->rangeSearch(tempo_.lo, tempo_.hi);
    std::vector<int> popMatches = set.popularity->rangeSearch(popularity_.lo, popularity_.hi);
    auto t1 = std::chrono::steady_clock::now();

    // Intersect the three sets of matches
    std::unordered_set<int> inYear(yearMatches.begin(), yearMatches.end());
    std::unordered_set<int> inYearAndTempo;
    for (int id : tempoMatches) {
        if (inYear.count(id))
            inYearAndTempo.insert(id);
    }
    std::vector<int> result;
    for (int id : popMatches) {
        if (inYearAndTempo.count(id) && passesOtherClues(id))
            result.push_back(id);
    }
    timing.matches = static_cast<int>(result.size());
    timing.treeMicros = timeBetween(t0, t1);
    return result;
}

bool Game::passesOtherClues(int id) const {
    if (guessed_.count(id)) {
        return false;
    }
    const std::string& artist = artistKey_[id];
    if (!requiredArtist_.empty()) {
        return artist == requiredArtist_;
    }
    return excludedArtists_.count(artist) == 0;
}

GuessResult Game::guess(int songId) {
    if (mystery_ < 0) {
        throw std::logic_error("guess: game not started");
    }
    if (songId < 0 || songId >= static_cast<int>(songs_.size())) {
        throw std::invalid_argument("guess: songId out of range");
    }
    if (isOver()) {
        throw std::logic_error("guess: game is over");
    }
    if (guessed_.count(songId)) {
        throw std::invalid_argument("guess: song already guessed");
    }

    const Song& mysterySong = songs_[mystery_];
    const Song& guessSong = songs_[songId];

    GuessResult result;
    result.songId = songId;
    result.correct = (songId == mystery_ || (titleKey_[songId] == titleKey_[mystery_] && artistKey_[songId] == artistKey_[mystery_]));
    if (result.correct) {
        won_ = true;
        candidates_ = {mystery_};
        result.matches = 1;
        history_.push_back(result);
        return result;
    }
    result.artistMatch = (artistKey_[songId] == artistKey_[mystery_]);
    result.year = compare(mysterySong.year, guessSong.year);
    result.tempo = compare(mysterySong.tempo, guessSong.tempo);
    result.popularity = compare(mysterySong.popularity, guessSong.popularity);

    // Record and tighten the clues
    guessed_.insert(songId);
    tighten(year_, guessSong.year, result.year);
    tighten(tempo_, guessSong.tempo, result.tempo);
    tighten(popularity_, guessSong.popularity, result.popularity);
    if (result.artistMatch)
        requiredArtist_ = artistKey_[songId];
    else
        excludedArtists_.insert(artistKey_[songId]);

    // Filter the songs with each structure and check if they all agree
    std::vector<int> reference;
    for (int i = 0; i < static_cast<int>(sets_.size()); ++i) {
        FilterTiming timing;
        timing.structure = sets_[i].name;
        std::vector<int> matches = filterWith(sets_[i], timing);
        result.timings.push_back(timing);
        if (i == 0) {
            reference = matches;
            if (verify_) std::sort(reference.begin(), reference.end());
        } else if (verify_) {
            std::sort(matches.begin(), matches.end());
            if (matches != reference)
                result.structuresAgree = false;
        }
    }
    candidates_ = reference;
    result.matches = static_cast<int>(candidates_.size());
    history_.push_back(result);
    return result;
}
std::vector<int> Game::findByTitle(const std::string& query, int limit) const {
    std::string lowerQuery = toLowercase(trim(query));
    std::vector<int> found;
    if (lowerQuery.empty())
        return found;
    for (const auto& pair : byTitle_) {
        if (pair.first.find(lowerQuery) != std::string::npos) {
            found.insert(found.end(), pair.second.begin(), pair.second.end());
        }
    }
    // Sort by popularity descending
    std::sort(found.begin(), found.end(), [this](int a, int b) {
        return songs_[a].popularity > songs_[b].popularity;
    });
    if (limit > 0 && static_cast<int>(found.size()) > limit) {
        found.resize(limit);
    }
    return found;
}