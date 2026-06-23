#pragma once

template <class K, class T>
class AVLTree {
public:
    enum BalanceValue { LH = -1, EH = 0, RH = 1 };

    class AVLNode {
    public:
        BalanceValue balance;
        K key;
        T data;
        AVLNode* pLeft;
        AVLNode* pRight;

        AVLNode(const K& key, const T& value)
            : key(key), data(value), pLeft(nullptr), pRight(nullptr), balance(EH) {}

        friend class VectorStore; // Allow VectorStore to access AVLNode members
    };

protected:
    AVLNode* root;

    AVLNode* rotateLeft(AVLNode*& node);
    AVLNode* rotateRight(AVLNode*& node);
    void clearHelper(AVLNode* node);
    void balanceLeft(AVLNode*& node);
    void balanceRight(AVLNode*& node);

public:
    AVLTree();
    ~AVLTree();

    bool insertRec(AVLNode*& node, const K& key, const T& value, bool& taller);
    void insert(const K& key, const T& value);

    void balanceIfRemoveLeft(AVLNode*& node, bool& shorter);
    void balanceIfRemoveRight(AVLNode*& node, bool& shorter);
    bool removeRec(AVLNode*& node, const K& key, bool& shorter);
    void remove(const K& key);

    bool contains(const K& key) const;
    bool isContain(const K& key, AVLNode* node) const;

    int getHeight(AVLNode* node) const;
    int getHeight() const;
    int getSize(AVLNode* node) const;
    int getSize() const;
    bool empty() const;

    void clear(AVLNode* node);
    void clear();

    void printTreeStructure() const;
    void printNSpace(int count) const;

    void inorderTraversal(void (*action)(const T&), AVLNode* node) const;
    void inorderTraversal(void (*action)(const T&)) const;

    AVLNode* getRoot() const;
    AVLNode* find(const K& key) const;
    AVLNode* find(const K& key, int& stepsCount) const;  
};
