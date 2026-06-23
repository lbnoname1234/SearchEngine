#pragma once
#include <vector>
#include <string>
#include <iostream>
#include "AVLTree.h"
#include "document.h"

class SearchEngine {
private:
    AVLTree<string, vector<int>> index;

    vector<Document> docs;

    AVLTree<string, int> wordFrequency;

    static int searchCount;

    int lastSearchSteps;

public:
    void addDocument(const string& text);

    vector<int> search(const string& keyword);

    void printDocs(const vector<int>& docIDs);

    int getFrequency(const string& keyword) const;

    static int getSearchCount(); 
    
    int getIndexHeight() const;

    int getIndexSize() const;

    void printIndexTree() const;

    static void resetSearchCount();

    int getLastSearchSteps() const;
};
