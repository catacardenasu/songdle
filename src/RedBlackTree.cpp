#include "RedBlackTree.h"

RedBlackTree::Node::Node(double k)
    : key(k), color(RED), left(nullptr), right(nullptr), parent(nullptr) {}

RedBlackTree::RedBlackTree()
    : NIL(new Node(0.0)), root_(NIL), nodeCount_(0), idBytes_(0) {
    NIL->color=BLACK;
    NIL->left=NIL->right = NIL;
}

RedBlackTree::~RedBlackTree() {
    destroy(root_);
    delete NIL;
}

void RedBlackTree::destroy(Node* node) {
    if (node==NIL) return;
    destroy(node->left);
    destroy(node->right);
    delete node;
}

bool RedBlackTree::hasId(const Node* node, int songId) {
    for (int id : node->ids) {
        if (id==songId) return true;
    }
    return false;
}

void RedBlackTree::insert(double key, int songId) {
    Node* parent=nullptr;
    Node* current=root_;

    while (current != NIL) {
        if (key==current->key) {
            if (hasId(current, songId)) return;
            int before=static_cast<int>(current->ids.capacity());
            current->ids.push_back(songId);
            int after=static_cast<int>(current->ids.capacity());
            idBytes_ += (after - before)*static_cast<int>(sizeof(int));
            return;
        }
        parent=current;
        if (key < current->key) current=current->left;
        else current=current->right;
    }

    Node* node=new Node(key);
    node->ids.push_back(songId);
    node->left=node->right=NIL;
    node->parent= parent;

    if (parent == nullptr) root_=node;
    else if (key<parent->key) parent->left = node;
    else parent->right = node;

    nodeCount_ += 1;
    idBytes_ += static_cast<int>(node->ids.capacity() * sizeof(int));

    if (node->parent == nullptr) {
        node->color=BLACK;
        return;
    }
    if (node->parent->parent == nullptr) return;

    fixInsert(node);
}

void RedBlackTree::leftRotate(Node* x) {
    Node* y= x->right;

    x->right=y->left;
    if (y->left != NIL) y->left->parent=x;

    y->parent=x->parent;
    if (x->parent == nullptr) root_=y;
    else if (x == x->parent->left) x->parent->left=y;
    else x->parent->right=y;

    y->left=x;
    x->parent=y;
}

void RedBlackTree::rightRotate(Node* x) {
    Node* y = x->left;

    x->left=y->right;
    if (y->right != NIL) y->right->parent=x;

    y->parent=x->parent;
    if (x->parent == nullptr) root_=y;
    else if (x == x->parent->right) x->parent->right=y;
    else x->parent->left = y;

    y->right=x;
    x->parent=y;
}

void RedBlackTree::fixInsert(Node* k) {
    while (k != root_ && k->parent->color == RED) {
        if (k->parent == k->parent->parent->left) {
            Node* uncle=k->parent->parent->right;

            if (uncle->color== RED) {
                k->parent->color=BLACK;
                uncle->color=BLACK;
                k->parent->parent->color=RED;
                k=k->parent->parent;
            } else {
                if (k == k->parent->right) {
                    k=k->parent;
                    leftRotate(k);
                }
                k->parent->color=BLACK;
                k->parent->parent->color= RED;
                rightRotate(k->parent->parent);
            }
        } else {
            Node* uncle=k->parent->parent->left;

            if (uncle->color == RED) {
                k->parent->color=BLACK;
                uncle->color=BLACK;
                k->parent->parent->color=RED;
                k=k->parent->parent;
            } else {
                if (k == k->parent->left) {
                    k=k->parent;
                    rightRotate(k);
                }
                k->parent->color=BLACK;
                k->parent->parent->color=RED;
                leftRotate(k->parent->parent);
            }
        }
    }
    root_->color=BLACK;
}

std::vector<int> RedBlackTree::search(double key) const {
    Node* current=root_;
    while (current != NIL) {
        if (key == current->key) return current->ids;
        if (key<current->key) current=current->left;
        else current=current->right;
    }
    return {};
}

RedBlackTree::Node* RedBlackTree::lowerBound(double key) const {
    Node* current=root_;
    Node* best=nullptr;
    while (current != NIL) {
        if (current->key>=key) {
            best=current;
            current=current->left;
        } else {
            current=current->right;
        }
    }
    return best;
}

RedBlackTree::Node* RedBlackTree::successor(Node* node) const {
    if (node->right != NIL) {
        node=node->right;
        while (node->left != NIL) node=node->left;
        return node;
    }
    Node* parent=node->parent;
    while (parent && node==parent->right) {
        node=parent;
        parent=parent->parent;
    }
    return parent;
}

std::vector<int> RedBlackTree::rangeSearch(double lo, double hi) const {
    std::vector<int> ids;
    if (lo>hi) return ids;
    Node* node=lowerBound(lo);
    while (node && node->key<=hi) {
        ids.insert(ids.end(), node->ids.begin(), node->ids.end());
        node=successor(node);
    }
    return ids;
}

int RedBlackTree::memoryBytes() const {
    return nodeCount_ * static_cast<int>(sizeof(Node))+idBytes_;
}

std::string RedBlackTree::name() const {
    return "Red-black tree";
}

bool RedBlackTree::invariantsHold() const {
    struct Walk {
        static int blackHeight(Node* node, Node* nil, bool& ok) {
            if (node==nil) return 1;
            if (node->color==RED) {
                if (node->left->color==RED || node->right->color==RED) ok=false;
            }
            if (node->left != nil) {
                if (node->left->parent != node || !(node->left->key < node->key)) ok=false;
            }
            if (node->right != nil) {
                if (node->right->parent != node || !(node->right->key > node->key)) ok=false;
            }
            int left=blackHeight(node->left, nil, ok);
            int right=blackHeight(node->right, nil, ok);
            if (left != right) ok=false;
            return left+(node->color == BLACK ? 1 : 0);
        }
    };

    if (root_==NIL) return true;
    if (root_->parent != nullptr || root_->color != BLACK) return false;
    bool ok=true;
    Walk::blackHeight(root_, NIL, ok);
    return ok;
}
