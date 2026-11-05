#pragma once
#include "TreeBase.h"

#include <algorithm>
#include <utility>
#include <vector>

// A simple reference index: sorted array + binary search.
// It is NOT one of the two structures being compared. It is used
//   1) as the "known correct" answer in tests/test_same_results.cpp
//   2) as a stand-in so the game runs before the trees are merged.
class SortedVectorIndex : public TreeBase {
public:
    void insert(double key, int songId) override {
        data_.emplace_back(key, songId);
        sorted_ = false;
    }

    std::vector<int> rangeSearch(double lo, double hi) const override {
        std::vector<int> out;
        if (lo > hi) return out;
        ensureSorted();
        auto it = std::lower_bound(data_.begin(), data_.end(), lo,
                                   [](const Entry& e, double v) { return e.first < v; });
        for (; it != data_.end() && it->first <= hi; ++it) out.push_back(it->second);
        return out;
    }

    int memoryBytes() const override {
        return static_cast<int>(sizeof(*this) + data_.capacity() * sizeof(Entry));
    }

    std::string name() const override { return "Sorted vector (reference)"; }

private:
    using Entry = std::pair<double, int>;

    void ensureSorted() const {
        if (!sorted_) {
            std::sort(data_.begin(), data_.end());
            sorted_ = true;
        }
    }

    mutable std::vector<Entry> data_;
    mutable bool sorted_ = true;
};