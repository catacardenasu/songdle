#pragma once

#include "TreeBase.h"

#include <string>
#include <vector>

class RedBlackTree : public TreeBase {
public:
    RedBlackTree();
    ~RedBlackTree() override;

    RedBlackTree(const RedBlackTree&) = delete;
    RedBlackTree& operator=(const RedBlackTree&) = delete;

    void insert(double key, int songId) override;
    std::vector<int> search(double key) const;
    std::vector<int> rangeSearch(double lo, double hi) const override;
    int memoryBytes() const override;
    std::string name() const override;

    bool invariantsHold() const;

private:
    enum Color { RED, BLACK };

    struct Node {
        double key;
        std::vector<int> ids;
        Color color;
        Node* left;
        Node* right;
        Node* parent;

        Node(double k);
    };

    Node* NIL;
    Node* root_;
    int nodeCount_;
    int idBytes_;

    void destroy(Node* node);
    static bool hasId(const Node* node, int songId);
    void leftRotate(Node* x);
    void rightRotate(Node* x);
    void fixInsert(Node* k);
    Node* lowerBound(double key) const;
    Node* successor(Node* node) const;
};
