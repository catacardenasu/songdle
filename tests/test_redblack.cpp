#include "RedBlackTree.h"

#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

static int fail(const std::string& message) {
    std::cerr << message << "\n";
    return 1;
}

static std::vector<int> referenceRange(const std::map<double, std::vector<int>>& ref,
                                       double lo, double hi) {
    std::vector<int> ids;
    if (lo > hi) return ids;
    for (auto it = ref.lower_bound(lo); it != ref.end() && it->first <= hi; ++it) {
        ids.insert(ids.end(), it->second.begin(), it->second.end());
    }
    return ids;
}

static void insertBoth(RedBlackTree& tree, std::map<double, std::vector<int>>& ref,
                       double key, int songId) {
    tree.insert(key, songId);
    std::vector<int>& ids = ref[key];
    for (int id : ids) {
        if (id == songId) return;
    }
    ids.push_back(songId);
}

static int checkAgainst(RedBlackTree& tree, const std::map<double, std::vector<int>>& ref,
                        std::mt19937& rng, const std::string& label) {
    if (!tree.invariantsHold()) return fail(label + ": red-black invariants failed");
    for (const auto& entry : ref) {
        if (tree.search(entry.first) != entry.second) return fail(label + ": search mismatch");
    }
    if (tree.search(-1.0) != std::vector<int>{}) return fail(label + ": missing key");

    std::uniform_real_distribution<double> bound(-10.0, 230.0);
    for (int i = 0; i < 100; ++i) {
        double lo = bound(rng);
        double hi = bound(rng);
        if (tree.rangeSearch(lo, hi) != referenceRange(ref, lo, hi)) {
            return fail(label + ": range mismatch");
        }
    }
    if (!tree.rangeSearch(5, 1).empty()) return fail(label + ": lo > hi should be empty");
    return 0;
}

int main() {
    RedBlackTree empty;
    if (!empty.invariantsHold()) return fail("empty tree broke the invariants");
    if (!empty.search(1).empty() || !empty.rangeSearch(0, 10).empty()) {
        return fail("empty tree should not find songs");
    }
    if (!empty.rangeSearch(8, 2).empty()) return fail("lo > hi should be empty");
    if (empty.memoryBytes() != 0) return fail("empty tree should use no node memory");
    if (empty.name() != "Red-black tree") return fail("unexpected tree name");

    std::mt19937 rng(41);
    std::map<double, std::vector<int>> oneRef;
    RedBlackTree one;
    insertBoth(one, oneRef, 1999, 4);
    if (int status = checkAgainst(one, oneRef, rng, "one node")) return status;
    if (one.memoryBytes() <= 0) return fail("one node should use memory");

    std::map<double, std::vector<int>> yearRef;
    RedBlackTree years;
    for (int id = 0; id < 80; ++id) insertBoth(years, yearRef, 2016, id);
    insertBoth(years, yearRef, 2016, 3);
    if (int status = checkAgainst(years, yearRef, rng, "same year")) return status;

    std::map<double, std::vector<int>> sortedRef;
    RedBlackTree ascending;
    for (int id = 0; id < 200; ++id) insertBoth(ascending, sortedRef, id, id);
    if (int status = checkAgainst(ascending, sortedRef, rng, "ascending")) return status;

    std::map<double, std::vector<int>> descRef;
    RedBlackTree descending;
    for (int id = 200; id >= 0; --id) insertBoth(descending, descRef, id, id);
    if (int status = checkAgainst(descending, descRef, rng, "descending")) return status;

    std::map<double, std::vector<int>> ref;
    RedBlackTree tree;
    std::uniform_int_distribution<int> year(1922, 2020);
    std::uniform_int_distribution<int> popularity(0, 100);
    std::uniform_real_distribution<double> tempo(40.0, 220.0);
    for (int id = 0; id < 4000; ++id) {
        double key = year(rng);
        if (id % 3 == 1) key = popularity(rng);
        else if (id % 3 == 2) key = tempo(rng);
        insertBoth(tree, ref, key, id);
    }
    if (int status = checkAgainst(tree, ref, rng, "mixed keys")) return status;

    std::cout << "red-black correctness check passed\n";
    return 0;
}
