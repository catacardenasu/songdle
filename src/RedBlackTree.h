#pragma once

#include "TreeBase.h"

#include <string>
#include <vector>

// One node per distinct key. Songs that share a year, tempo, or popularity
// store their ids together on that node.
class RedBlackTree : public TreeBase {
public:
    RedBlackTree();
    ~RedBlackTree() override;

    RedBlackTree(const RedBlackTree&) = delete;
    RedBlackTree& operator=(const RedBlackTree&) = delete;

    void insert(double key, int songId) override;
    std::vector<int> rangeSearch(double lo, double hi) const override;
    int memoryBytes() const override;
    std::string name() const override;

private:
    enum Color { RED, BLACK };

    struct Node {
        double key;
        std::vector<int> ids;
        Color color;
        Node* left;
        Node* right;
        Node* parent;

        Node(double k, int songId);
    };

    Node* root_;
    int nodeCount_;
    int idBytes_;

    void destroy(Node* node);
    static bool hasId(const Node* node, int songId);
    void leftRotate(Node* x);
    void rightRotate(Node* x);
    void insertFixup(Node* node);
};
