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

std::vector<int> RedBlackTree::search(double key) const {
    Node* cur = root_;
    while (cur) {
        if (key == cur->key) return cur->ids;
        if (key < cur->key) cur = cur->left;
        else cur = cur->right;
    }
    return {};
}

RedBlackTree::Node* RedBlackTree::lowerBound(double key) const {
    Node* cur = root_;
    Node* best = nullptr;
    while (cur) {
        if (cur->key >= key) {
            best = cur;
            cur = cur->left;
        } else {
            cur = cur->right;
        }
    }
    return best;
}

RedBlackTree::Node* RedBlackTree::successor(Node* node) const {
    if (node->right) {
        node = node->right;
        while (node->left) node = node->left;
        return node;
    }
    Node* parent = node->parent;
    while (parent && node == parent->right) {
        node = parent;
        parent = parent->parent;
    }
    return parent;
}

std::vector<int> RedBlackTree::rangeSearch(double lo, double hi) const {
    std::vector<int> ids;
    if (lo > hi) return ids;
    Node* node = lowerBound(lo);
    while (node && node->key <= hi) {
        ids.insert(ids.end(), node->ids.begin(), node->ids.end());
        node = successor(node);
    }
    return ids;
}

int RedBlackTree::memoryBytes() const {
    return nodeCount_ * static_cast<int>(sizeof(Node)) + idBytes_;
}

std::string RedBlackTree::name() const {
    return "Red-black tree";
}

bool RedBlackTree::invariantsHold() const {
    struct Walk {
        static int blackHeight(Node* node, bool& ok) {
            if (!node) return 1;
            if (node->color == RED) {
                if ((node->left && node->left->color == RED) ||
                    (node->right && node->right->color == RED)) {
                    ok = false;
                }
            }
            if (node->left) {
                if (node->left->parent != node || !(node->left->key < node->key)) ok = false;
            }
            if (node->right) {
                if (node->right->parent != node || !(node->right->key > node->key)) ok = false;
            }
            int left = blackHeight(node->left, ok);
            int right = blackHeight(node->right, ok);
            if (left != right) ok = false;
            return left + (node->color == BLACK ? 1 : 0);
        }
    };

    if (!root_) return true;
    if (root_->parent != nullptr || root_->color != BLACK) return false;
    bool ok = true;
    Walk::blackHeight(root_, ok);
    return ok;
}
