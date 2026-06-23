#include "AVLTree.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

using std::max;

template <class K, class T>
AVLTree<K, T>::AVLTree()
    : root(nullptr) {}

template <class K, class T>
AVLTree<K, T>::~AVLTree() {
    clearHelper(root);
    root = nullptr;
}

template <class K, class T>
typename AVLTree<K, T>::AVLNode* AVLTree<K, T>::rotateLeft(AVLNode*& node) {
    AVLNode* right = node->pRight;
    AVLNode* rightLeft = right->pLeft;
    right->pLeft = node;
    node->pRight = rightLeft;
    return right;
}

template <class K, class T> 
typename AVLTree<K, T>::AVLNode* AVLTree<K, T>::rotateRight(AVLNode*& node) {
    AVLNode* left = node->pLeft;
    AVLNode* leftRight = left->pRight;
    left->pRight = node;
    node->pLeft = leftRight;
    return left;
}

template <class K, class T>
void AVLTree<K, T>::clearHelper(AVLNode* node) {
    if (!node) return;
    clearHelper(node->pLeft);
    clearHelper(node->pRight);
    delete node;
}

template <class K, class T>
void AVLTree<K, T>::balanceLeft(AVLNode*& node) {
    AVLNode* left = node->pLeft;
    if (left->balance == LH) {
        node = rotateRight(node);
        node->balance = EH;
        node->pRight->balance = EH;
    } else if (left->balance == RH) {
        AVLNode* leftRight = left->pRight;
        node->pLeft = rotateLeft(node->pLeft);
        node = rotateRight(node);

        if (leftRight->balance == LH) {
            node->balance = EH;
            node->pLeft->balance = EH;
            node->pRight->balance = RH;
        } else if (leftRight->balance == RH) {
            node->balance = EH;
            node->pLeft->balance = LH;
            node->pRight->balance = EH;
        } else {
            node->balance = EH;
            node->pLeft->balance = EH;
            node->pRight->balance = EH;
        }
    }
}

template <class K, class T>
void AVLTree<K, T>::balanceRight(AVLNode*& node) {
    AVLNode* right = node->pRight;
    if (right->balance == RH) {
        node = rotateLeft(node);
        node->balance = EH;
        node->pLeft->balance = EH;
    } else if (right->balance == LH) {
        AVLNode* rightLeft = right->pLeft;
        node->pRight = rotateRight(node->pRight);
        node = rotateLeft(node);

        if (rightLeft->balance == RH) {
            node->balance = EH;
            node->pLeft->balance = LH;
            node->pRight->balance = EH;
        } else if (rightLeft->balance == LH) {
            node->balance = EH;
            node->pLeft->balance = EH;
            node->pRight->balance = RH;
        } else {
            node->balance = EH;
            node->pLeft->balance = EH;
            node->pRight->balance = EH;
        }
    }
}

template <class K, class T>
bool AVLTree<K, T>::insertRec(AVLNode*& node, const K& key, const T& value, bool& taller) {
    if (node == nullptr) {
        node = new AVLNode(key, value);
        taller = true;
        return true;
    }
    if (key > node->key) {
        if (!insertRec(node->pRight, key, value, taller)) return false;
        if (taller) {
            if (node->balance == RH) {
                balanceRight(node);
                taller = false;
            } else if (node->balance == EH) {
                node->balance = RH;
            } else {
                node->balance = EH;
                taller = false;
            }
        }
    } else if (key < node->key) {
        if (!insertRec(node->pLeft, key, value, taller)) return false;
        if (taller) {
            if (node->balance == LH) {
                balanceLeft(node);
                taller = false;
            } else if (node->balance == EH) {
                node->balance = LH;
            } else {
                node->balance = EH;
                taller = false;
            }
        }
    } else {
        taller = false;
        return false;
    }
    return true;
}

template <class K, class T>
void AVLTree<K, T>::insert(const K& key, const T& value) {
    bool taller = false;
    insertRec(root, key, value, taller);
}

template <class K, class T>
void AVLTree<K, T>::balanceIfRemoveLeft(AVLNode*& node, bool& shorter) {
    if (node->balance == LH) {
        node->balance = EH;
        shorter = true;
    } else if (node->balance == EH) {
        node->balance = RH;
        shorter = false;
    } else {
        AVLNode* right = node->pRight;
        if (!right) return;
        if (right->balance == RH) {
            node = rotateLeft(node);
            node->balance = EH;
            node->pLeft->balance = EH;
            shorter = true;
        } else if (right->balance == EH) {
            node = rotateLeft(node);
            node->balance = LH;
            node->pLeft->balance = RH;
            shorter = false;
        } else {
            AVLNode* rightLeft = right->pLeft;
            node->pRight = rotateRight(node->pRight);
            node = rotateLeft(node);

            if (rightLeft->balance == RH) {
                node->balance = EH;
                node->pLeft->balance = LH;
                node->pRight->balance = EH;
            } else if (rightLeft->balance == LH) {
                node->balance = EH;
                node->pLeft->balance = EH;
                node->pRight->balance = RH;
            } else {
                node->balance = EH;
                node->pLeft->balance = EH;
                node->pRight->balance = EH;
            }
            shorter = true;
        }
    }
}

template <class K, class T>
void AVLTree<K, T>::balanceIfRemoveRight(AVLNode*& node, bool& shorter) {
    if (!node) return;
    if (node->balance == RH) {
        node->balance = EH;
        shorter = true;
    } else if (node->balance == EH) {
        node->balance = LH;
        shorter = false;
    } else {
        AVLNode* left = node->pLeft;
        if (!left) return;

        if (left->balance == LH) {
            node = rotateRight(node);
            node->balance = EH;
            node->pRight->balance = EH;
            shorter = true;
        } else if (left->balance == EH) {
            node = rotateRight(node);
            node->balance = RH;
            node->pRight->balance = LH;
            shorter = false;
        } else {
            AVLNode* leftRight = left->pRight;
            node->pLeft = rotateLeft(node->pLeft);
            node = rotateRight(node);

            if (leftRight->balance == LH) {
                node->balance = EH;
                node->pLeft->balance = EH;
                node->pRight->balance = RH;
            } else if (leftRight->balance == RH) {
                node->balance = EH;
                node->pLeft->balance = LH;
                node->pRight->balance = EH;
            } else {
                node->balance = EH;
                node->pLeft->balance = EH;
                node->pRight->balance = EH;
            }
            shorter = true;
        }
    }
}

template <class K, class T>
bool AVLTree<K, T>::removeRec(AVLNode*& node, const K& key, bool& shorter) {
    if (node == nullptr) return false;
    if (node->key > key) {
        if (!removeRec(node->pLeft, key, shorter)) return false;
        if (shorter) balanceIfRemoveLeft(node, shorter);
    } else if (node->key < key) {
        if (!removeRec(node->pRight, key, shorter)) return false;
        if (shorter) balanceIfRemoveRight(node, shorter);
    } else {
        if (node->pLeft == nullptr) {
            AVLNode* temp = node;
            node = node->pRight;
            delete temp;
            shorter = true;
        } else if (node->pRight == nullptr) {
            AVLNode* temp = node;
            node = node->pLeft;
            delete temp;
            shorter = true;
        } else {
            AVLNode* minLarger = node->pRight;
            while (minLarger->pLeft)
                minLarger = minLarger->pLeft;
            node->key = minLarger->key;
            node->data = minLarger->data;
            removeRec(node->pRight, minLarger->key, shorter);
            if (shorter) balanceIfRemoveRight(node, shorter);
        }
    }
    return true;
}

template <class K, class T>
void AVLTree<K, T>::remove(const K& key) {
    bool shorter = false;
    removeRec(root, key, shorter);
}

template <class K, class T>
bool AVLTree<K, T>::contains(const K& key) const {
    return isContain(key, root);
}

template <class K, class T>
bool AVLTree<K, T>::isContain(const K& key, AVLNode* node) const {
    if (node == nullptr) return false;
    if (node->key == key) return true;
    if (key > node->key) return isContain(key, node->pRight);
    return isContain(key, node->pLeft);
}

template <class K, class T>
int AVLTree<K, T>::getHeight(AVLNode* node) const {
    if (node == nullptr) return 0;
    return 1 + max(getHeight(node->pLeft), getHeight(node->pRight));
}

template <class K, class T>
int AVLTree<K, T>::getHeight() const {
    return getHeight(root);
}

template <class K, class T>
int AVLTree<K, T>::getSize(AVLNode* node) const {
    if (node == nullptr) return 0;
    return 1 + getSize(node->pLeft) + getSize(node->pRight);
}

template <class K, class T>
int AVLTree<K, T>::getSize() const {
    return getSize(root);
}

template <class K, class T>
bool AVLTree<K, T>::empty() const {
    return root == nullptr;
}

template <class K, class T>
void AVLTree<K, T>::clear(AVLNode* node) {
    if (node == nullptr) return;
    clear(node->pLeft);
    clear(node->pRight);
    delete node;
}

template <class K, class T>
void AVLTree<K, T>::clear() {
    clear(root);
    root = nullptr;
}

template <class K, class T>
void AVLTree<K, T>::printTreeStructure() const {
    int height = this->getHeight();
    if (this->root == nullptr) {
        std::cout << "NULL\n";
        return;
    }

    std::queue<AVLNode*> q;
    q.push(root);
    AVLNode* temp;
    int count = 0;
    int maxNode = 1;
    int level = 0;
    int space = static_cast<int>(std::pow(2, height));
    printNSpace(space / 2);

    while (!q.empty()) {
        temp = q.front();
        q.pop();

        if (temp == nullptr) {
            std::cout << " ";
            q.push(nullptr);
            q.push(nullptr);
        } else {
            std::cout << temp->key;
            q.push(temp->pLeft);
            q.push(temp->pRight);
        }

        printNSpace(space);
        count++;

        if (count == maxNode) {
            std::cout << std::endl;
            count = 0;
            maxNode *= 2;
            level++;
            space /= 2;
            printNSpace(space / 2);
        }

        if (level == height) {
            return;
        }
    }
}

template <class K, class T>
void AVLTree<K, T>::printNSpace(int count) const {
    for (int i = 0; i < count; ++i) {
        std::cout << ' ';
    }
}

template <class K, class T>
void AVLTree<K, T>::inorderTraversal(void (*action)(const T&), AVLNode* node) const {
    if (node == nullptr) return;
    inorderTraversal(action, node->pLeft);
    action(node->data);
    inorderTraversal(action, node->pRight);
}

template <class K, class T>
void AVLTree<K, T>::inorderTraversal(void (*action)(const T&)) const {
    inorderTraversal(action, root);
}

template <class K, class T>
typename AVLTree<K, T>::AVLNode* AVLTree<K, T>::getRoot() const {
    return root;
}

template <class K, class T>
typename AVLTree<K, T>::AVLNode* AVLTree<K, T>::find(const K& key) const {
    AVLNode* node = root;
    while (node != nullptr) {
        if (key == node->key) return node;
        node = (key > node->key) ? node->pRight : node->pLeft;
    }
    return nullptr;
}

template <class K, class T>
typename AVLTree<K, T>::AVLNode* AVLTree<K, T>::find(const K& key, int& stepsCount) const {
    AVLNode* node = root;
    stepsCount = 0;
    while (node != nullptr) {
        stepsCount++;
        if (key == node->key) return node;
        node = (key > node->key) ? node->pRight : node->pLeft;
    }
    return nullptr;
}

template class AVLTree<std::string, std::vector<int>>;
template class AVLTree<std::string, int>;