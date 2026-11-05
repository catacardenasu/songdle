#include "RedBlackTree.h"

RedBlackTree::Node::Node(double k, int songId)
    : key(k), left(nullptr), right(nullptr), parent(nullptr) {
    ids.push_back(songId);
}

RedBlackTree::RedBlackTree() : root_(nullptr), nodeCount_(0), idBytes_(0) {}

RedBlackTree::~RedBlackTree() {
    destroy(root_);
}

void RedBlackTree::destroy(Node* node) {
    if (!node) return;
    destroy(node->left);
    destroy(node->right);
    delete node;
}

bool RedBlackTree::hasId(const Node* node, int songId) {
    for (int id : node->ids) {
        if (id == songId) return true;
    }
    return false;
}

void RedBlackTree::insert(double key, int songId) {
    Node* parent = nullptr;
    Node* cur = root_;
    while (cur) {
        if (key == cur->key) {
            if (hasId(cur, songId)) return;
            int before = static_cast<int>(cur->ids.capacity());
            cur->ids.push_back(songId);
            int after = static_cast<int>(cur->ids.capacity());
            idBytes_ += (after - before) * static_cast<int>(sizeof(int));
            return;
        }
        parent = cur;
        if (key < cur->key) cur = cur->left;
        else cur = cur->right;
    }

    Node* node = new Node(key, songId);
    node->parent = parent;
    if (!parent) {
        root_ = node;
    } else if (key < parent->key) {
        parent->left = node;
    } else {
        parent->right = node;
    }

    nodeCount_ += 1;
    idBytes_ += static_cast<int>(node->ids.capacity() * sizeof(int));
}

std::vector<int> RedBlackTree::rangeSearch(double, double) const {
    return {};
}

int RedBlackTree::memoryBytes() const {
    return nodeCount_ * static_cast<int>(sizeof(Node)) + idBytes_;
}

std::string RedBlackTree::name() const {
    return "Red-black tree";
}
