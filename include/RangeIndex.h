// include/RangeIndex.h
#pragma once
#include <vector>
#include <string>
#include <cstddef>
using namespace std;

template <typename K>
class RangeIndex {
public:
    virtual ~RangeIndex() = default;

   
    virtual void insert(const K& key, int songIdx) = 0;

    
    virtual vector<int> rangeSearch(const K& lo, const K& hi) const = 0;

    virtual size_t size() const = 0;         
    virtual int height() const = 0;          
    virtual size_t memoryBytes() const = 0;  
    virtual string name() const = 0;    // "B+ tree" or "Red-black tree"
};