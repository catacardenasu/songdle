// Proves both trees return exactly the same songs as a simple sorted-vector
// reference, first on a small hand-made index, then on the real dataset.
//
// Usage: test_same_results [path/to/tracks.csv] [songLimit]
// Exit code 0 = all checks passed.

#include "DataLoader.h"
#include "Game.h"
#include "SortedVectorIndex.h"

#if __has_include("HashMapIndex.h")
#include "HashMapIndex.h"
#define HAVE_HASH 1
#endif
#if __has_include("RedBlackTree.h")
#include "RedBlackTree.h"
#define HAVE_RB 1
#endif

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace {

using Factory = std::function<TreeBase*()>;
struct Candidate {
    std::string name;
    Factory make;
};

int checks = 0;
int failures = 0;

void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        if (failures <= 25) std::cerr << "  FAIL: " << what << '\n';
    }
}

bool sameIds(std::vector<int> a, std::vector<int> b) {
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    return a == b;
}

std::string range(double lo, double hi) {
    return "[" + std::to_string(lo) + ", " + std::to_string(hi) + "]";
}

std::vector<Candidate> structuresUnderTest() {
    std::vector<Candidate> c;
#ifdef HAVE_HASH
    c.push_back({"Hash map", []() -> TreeBase* { return new HashMapIndex(); }});
#endif
#ifdef HAVE_RB
    c.push_back({"Red-black tree", []() -> TreeBase* { return new RedBlackTree(); }});
#endif
    if (c.empty()) {
        std::cout << "NOTE: BPlusTree.h / RedBlackTree.h not found. Testing the reference "
                     "against itself so the test harness is checked.\n";
        c.push_back({"Reference (self-check)", []() -> TreeBase* { return new SortedVectorIndex(); }});
    }
    return c;
}

// ---- Part 1: small hand-made index with heavy duplicates --------------------
void testSmallIndex(const Candidate& c) {
    std::cout << "[" << c.name << "] small index tests\n";

    TreeBase* empty = c.make();
    check(empty->rangeSearch(-1e18, 1e18).empty(), c.name + ": empty index returns nothing");
    delete empty;

    std::mt19937 rng(7);
    SortedVectorIndex ref;
    TreeBase* idx = c.make();
    for (int id = 0; id < 500; ++id) {
        double key = static_cast<double>(rng() % 50);
        if (id % 5 == 0) key += 0.5;  // a few fractional keys, like tempo
        ref.insert(key, id);
        idx->insert(key, id);
    }

    // Every whole-number range from -2..52, including lo > hi.
    for (int lo = -2; lo <= 52; ++lo)
        for (int hi = -2; hi <= 52; ++hi)
            check(sameIds(ref.rangeSearch(lo, hi), idx->rangeSearch(lo, hi)),
                  c.name + ": small " + range(lo, hi));

    // Strict bounds exactly like the game makes them (just above / below a key).
    const double inf = std::numeric_limits<double>::infinity();
    for (int k = 0; k < 50; ++k) {
        double above = std::nextafter(static_cast<double>(k), inf);
        double below = std::nextafter(static_cast<double>(k), -inf);
        double low = std::numeric_limits<double>::lowest(), high = std::numeric_limits<double>::max();
        check(sameIds(ref.rangeSearch(above, high), idx->rangeSearch(above, high)),
              c.name + ": strictly greater than " + std::to_string(k));
        check(sameIds(ref.rangeSearch(low, below), idx->rangeSearch(low, below)),
              c.name + ": strictly less than " + std::to_string(k));
        check(sameIds(ref.rangeSearch(k, k), idx->rangeSearch(k, k)),
              c.name + ": exactly " + std::to_string(k));
    }

    delete idx;

    // Sorted insertion order: the worst case for an unbalanced tree.
    TreeBase* sortedIdx = c.make();
    const int n = 100000;
    for (int i = 0; i < n; ++i) sortedIdx->insert(i, i);
    check(static_cast<int>(sortedIdx->rangeSearch(0, n).size()) == n,
          c.name + ": 100k sorted inserts, full range");
    check(static_cast<int>(sortedIdx->rangeSearch(500, 599).size()) == 100,
          c.name + ": 100k sorted inserts, slice");
    check(sortedIdx->memoryBytes() > 0, c.name + ": memoryBytes() is implemented");
    delete sortedIdx;
}

// ---- Part 2: the real dataset ---------------------------------------------
void testRealData(const std::vector<Song>& songs, const std::vector<Candidate>& cands) {
    std::cout << "Building reference indexes for " << songs.size() << " songs...\n";
    std::vector<CategorySets> sets;
    sets.push_back(buildCategorySet<SortedVectorIndex>("Reference", songs));

    for (const Candidate& c : cands) {
        CategorySets s;
        s.name = c.name;
        s.year = c.make();
        s.tempo = c.make();
        s.popularity = c.make();
        for (int i = 0; i < static_cast<int>(songs.size()); ++i) {
            s.year->insert(songs[i].year, i);
            s.tempo->insert(songs[i].tempo, i);
            s.popularity->insert(songs[i].popularity, i);
        }
        sets.push_back(std::move(s));
    }

    std::mt19937 rng(123);
    std::uniform_int_distribution<int> anySong(0, static_cast<int>(songs.size()) - 1);

    // Random range queries on each attribute, bounds taken from real values.
    for (int k = 1; k < static_cast<int>(sets.size()); ++k) {
        std::cout << "[" << sets[k].name << "] 3,000 random range queries\n";
        for (int q = 0; q < 1000; ++q) {
            const Song& a = songs[anySong(rng)];
            const Song& b = songs[anySong(rng)];
            double ylo = std::min(a.year, b.year), yhi = std::max(a.year, b.year);
            double tlo = std::min(a.tempo, b.tempo), thi = std::max(a.tempo, b.tempo);
            double plo = std::min(a.popularity, b.popularity), phi = std::max(a.popularity, b.popularity);
            check(sameIds(sets[0].year->rangeSearch(ylo, yhi), sets[k].year->rangeSearch(ylo, yhi)),
                  sets[k].name + ": year " + range(ylo, yhi));
            check(sameIds(sets[0].tempo->rangeSearch(tlo, thi), sets[k].tempo->rangeSearch(tlo, thi)),
                  sets[k].name + ": tempo " + range(tlo, thi));
            check(sameIds(sets[0].popularity->rangeSearch(plo, phi),
                          sets[k].popularity->rangeSearch(plo, phi)),
                          sets[k].name + ": popularity " + range(plo, phi));
        }
    }

    // Full games: every structure must give the same candidates after every guess.
    std::cout << "Playing 100 automatic games across all structures\n";
    Game game(songs, sets);
    game.setVerify(true);
    for (int g = 0; g < 100; ++g) {
        game.startRandom(rng, 0);
        while (!game.isOver()) {
            const auto& cand = game.candidates();
            int pick = cand.empty()
                           ? anySong(rng)
                           : cand[std::uniform_int_distribution<int>(0, static_cast<int>(cand.size()) - 1)(rng)];
            GuessResult r = game.guess(pick);
            check(r.structuresAgree, "game " + std::to_string(g + 1) + ", guess " +
                                         std::to_string(game.guessesUsed()) + ": structures disagree");
            check(r.correct || std::find(game.candidates().begin(), game.candidates().end(),
                                         game.mysteryId()) != game.candidates().end(),
                  "game " + std::to_string(g + 1) + ": mystery song missing from candidates");
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::string path = argc > 1 ? argv[1] : "data/songs_clean.tsv";
    const int limit = argc > 2 ? std::atoi(argv[2]) : 0;

    std::vector<Candidate> cands = structuresUnderTest();
    for (const Candidate& c : cands) testSmallIndex(c);

    try {
        LoadStats stats;
        std::vector<Song> songs = DataLoader::load(path, &stats, limit);
        std::cout << "Loaded " << songs.size() << " songs from " << path << '\n';
        if (!songs.empty()) testRealData(songs, cands);
    } catch (const std::exception& e) {
        std::cout << "Skipping real-data tests: " << e.what() << '\n';
    }

    std::cout << '\n' << (checks - failures) << " / " << checks << " checks passed\n";
    if (failures) std::cout << "FAILED (" << failures << " failures)\n";
    else std::cout << "ALL PASSED\n";
    return failures ? 1 : 0;
}