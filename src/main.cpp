#include "DataLoader.h"
#include "GameUI.h"
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

#include <iostream>

int main(int argc, char** argv) {
    const std::string path = argc > 1 ? argv[1] : "data/tracks.csv";

    std::cout << "Loading " << path << "...\n";
    LoadStats stats;
    std::vector<Song> songs;
    try {
        songs = DataLoader::load(path, &stats);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    std::cout << "Loaded " << stats.loaded << " songs (" << stats.skipped
              << " rows skipped) in " << stats.time << " ms\n";
    if (songs.empty()) return 1;

    std::vector<CategorySets> sets;
#ifdef HAVE_HASH
    sets.push_back(buildIndexSet<HashMapIndex>("Hash map", songs));
#endif
#ifdef HAVE_RB
    sets.push_back(buildIndexSet<RedBlackTree>("Red-black tree", songs));
#endif
    if (sets.empty()) {  // before the trees are merged
        std::cout << "(Trees not found yet: using the sorted-vector stand-in.)\n";
        sets.push_back(buildCategorySet<SortedVectorIndex>("Sorted vector (stand-in)", songs));
    }
    for (const CategorySets& s : sets)
        std::cout << "Built " << s.name << " in " << s.buildTime << " ms\n";

    try {
        GameUI ui(songs, sets);
        ui.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}