#include "RedBlackTree.h"

RedBlackTree::Node::Node(double k, int songId)
    : key(k), color(RED), left(nullptr), right(nullptr), parent(nullptr) {
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
    insertFixup(node);
}

void RedBlackTree::leftRotate(Node* x) {
    Node* y = x->right;
    x->right = y->left;
    if (y->left) y->left->parent = x;
    y->parent = x->parent;
    if (!x->parent) {
        root_ = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

void RedBlackTree::rightRotate(Node* x) {
    Node* y = x->left;
    x->left = y->right;
    if (y->right) y->right->parent = x;
    y->parent = x->parent;
    if (!x->parent) {
        root_ = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }
    y->right = x;
    x->parent = y;
}

// A new node starts red. Recolor and rotate until no red node has a red parent.
void RedBlackTree::insertFixup(Node* node) {
    while (node->parent && node->parent->color == RED) {
        Node* grand = node->parent->parent;
        if (node->parent == grand->left) {
            Node* uncle = grand->right;
            if (uncle && uncle->color == RED) {
                node->parent->color = BLACK;
                uncle->color = BLACK;
                grand->color = RED;
                node = grand;
            } else {
                if (node == node->parent->right) {
                    node = node->parent;
                    leftRotate(node);
                }
                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                rightRotate(node->parent->parent);
            }
        } else {
            Node* uncle = grand->left;
            if (uncle && uncle->color == RED) {
                node->parent->color = BLACK;
                uncle->color = BLACK;
                grand->color = RED;
                node = grand;
            } else {
                if (node == node->parent->left) {
                    node = node->parent;
                    rightRotate(node);
                }
                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                leftRotate(node->parent->parent);
            }
        }
    }
    root_->color = BLACK;
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
