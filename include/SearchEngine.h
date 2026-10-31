#pragma once
#include <vector>
#include <string>
#include <iostream>
#include "AVLTree.h"
#include "document.h"

class SearchEngine
{
private:
    AVLTree<string, vector<int>> index;

    vector<Document> docs;

    AVLTree<string, int> wordFrequency;

    static int searchCount;

    int lastSearchSteps;
    void collectWords(AVLTree<string, int>::AVLNode *node, vector<string> &words) const;

public:
    void addDocument(const string &text);

    vector<int> search(const string &keyword);

    void printDocs(const vector<int> &docIDs);

    int getFrequency(const string &keyword) const;

    static int getSearchCount();

    int getIndexHeight() const;

    int getIndexSize() const;

    void printIndexTree() const;

    static void resetSearchCount();

    int getLastSearchSteps() const;
    string getDocText(int docId) const
    {
        if (docId < 0 || docId >= (int)docs.size())
            return "";
        return docs[docId].text;
    }

    struct ScoredDoc
    {
        int docId;
        double score;
    };
    double computeTF(const string &word, int docId) const;
    double computeIDF(const string &word) const;
    vector<ScoredDoc> searchRanked(const string &keyword);

    // Boolean Query
    vector<int> searchAND(const vector<string> &keywords);
    vector<int> searchOR(const vector<string> &keywords);

    // Fuzzy Search
    vector<string> suggestSimilar(const string &keyword, int maxSuggestions = 3);
    int getDocCount() const { return docs.size(); }
};
