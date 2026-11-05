#pragma once
#include <string>
#include <vector>

// Both BPlusTree and RedBlackTree inherit from this class so the game,
// benchmark and tests can treat them the same way.
// Rules:
//  * Keys are doubles. Year and popularity are whole numbers, which doubles
//    store exactly, so one key type covers all three indexes.
//  * DUPLICATE KEYS MUST BE ALLOWED. Thousands of songs share a year.
//  * rangeSearch(lo, hi) is INCLUSIVE on both ends: it returns the song id of
//    every entry with lo <= key <= hi, in any order, each id exactly once.
//  * If lo > hi, return an empty vector.
//  * memoryBytes() estimates the bytes the structure uses
//  * lo can be std::numeric_limits<double>::lowest() and hi can be max() when a
//    clue hasn't narrowed that value yet. Never loop from lo to hi directly:
//    clamp to the smallest/largest key you actually stored.
//    (e.g. nodeCount * sizeof(Node) + any vectors' capacity).
// ---------------------------------------------------------------------------
class TreeBase {
public:
    virtual ~TreeBase() = default;
    virtual void insert(double key, int songId) = 0;
    virtual std::vector<int> rangeSearch(double lo, double hi) const = 0;
    virtual int memoryBytes() const = 0;
    virtual std::string name() const = 0;
};