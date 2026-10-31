#include <iostream>
#include <cmath>
#include <algorithm>
#include "SearchEngine.h"
#include "Tokenizer.h"
#include "../utils/StringUtils.h"
int SearchEngine::searchCount = 0;
void SearchEngine::addDocument(const string &text)
{
    Document doc;
    doc.id = docs.size();
    doc.text = text;

    vector<string> words = Tokenizer::tokenize(text);
    doc.wordCount = words.size();
    docs.push_back(doc);
    for (const string &word : words)
    {
        AVLTree<string, vector<int>>::AVLNode *node = index.find(word);
        if (node != nullptr)
        {
            if (node->data.empty() || node->data.back() != doc.id)
            {
                node->data.push_back(doc.id);
            }
        }
        else
        {
            index.insert(word, {doc.id});
        }

        AVLTree<string, int>::AVLNode *freqNode = wordFrequency.find(word);
        if (freqNode != nullptr)
        {
            freqNode->data += 1;
        }
        else
        {
            wordFrequency.insert(word, 1);
        }
    }
}
vector<int> SearchEngine::search(const string &keyword)
{
    searchCount++;
    int steps = 0;
    AVLTree<string, vector<int>>::AVLNode *node = index.find(keyword, steps);
    lastSearchSteps = steps;
    if (node != nullptr)
    {
        return node->data;
    }
    return {};
}

int SearchEngine::getLastSearchSteps() const
{
    return lastSearchSteps;
}

int SearchEngine::getFrequency(const string &keyword) const
{
    AVLTree<string, int>::AVLNode *node = wordFrequency.find(keyword);
    if (node != nullptr)
        return node->data;
    return 0;
}
int SearchEngine::getSearchCount()
{
    return searchCount;
}
void SearchEngine::printDocs(const vector<int> &docIDs)
{
    for (int id : docIDs)
    {

        cout << "Document " << id << ": " << docs[id].text << endl;
    }
}

void SearchEngine::printIndexTree() const
{
    index.printTreeStructure();
}

int SearchEngine::getIndexHeight() const
{
    return index.getHeight();
}

int SearchEngine::getIndexSize() const
{
    return index.getSize();
}

void SearchEngine::resetSearchCount()
{
    searchCount = 0;
}

// ===== TF-IDF Ranking =====
double SearchEngine::computeTF(const string &word, int docId) const
{
    if (docId < 0 || docId >= (int)docs.size() || docs[docId].wordCount == 0)
        return 0.0;
    vector<string> words = Tokenizer::tokenize(docs[docId].text);
    int count = 0;
    for (const string &w : words)
        if (w == word)
            count++;
    return (double)count / docs[docId].wordCount;
}

double SearchEngine::computeIDF(const string &word) const
{
    AVLTree<string, vector<int>>::AVLNode *node = index.find(word);
    if (node == nullptr || node->data.empty())
        return 0.0;
    return log((double)docs.size() / (1.0 + node->data.size()));
}

vector<SearchEngine::ScoredDoc> SearchEngine::searchRanked(const string &keyword)
{
    vector<int> docIds = search(keyword);
    double idf = computeIDF(keyword);
    vector<ScoredDoc> results;
    for (int id : docIds)
    {
        double tf = computeTF(keyword, id);
        results.push_back({id, tf * idf});
    }
    sort(results.begin(), results.end(), [](const ScoredDoc &a, const ScoredDoc &b)
         { return a.score > b.score; });
    return results;
}

// ===== Boolean Query =====
vector<int> SearchEngine::searchAND(const vector<string> &keywords)
{
    if (keywords.empty())
        return {};
    vector<int> result = search(keywords[0]);
    sort(result.begin(), result.end());
    for (size_t i = 1; i < keywords.size(); i++)
    {
        vector<int> next = search(keywords[i]);
        sort(next.begin(), next.end());
        vector<int> intersected;
        set_intersection(result.begin(), result.end(),
                         next.begin(), next.end(),
                         back_inserter(intersected));
        result = intersected;
    }
    return result;
}

vector<int> SearchEngine::searchOR(const vector<string> &keywords)
{
    vector<int> result;
    for (const string &kw : keywords)
    {
        vector<int> ids = search(kw);
        result.insert(result.end(), ids.begin(), ids.end());
    }
    sort(result.begin(), result.end());
    result.erase(unique(result.begin(), result.end()), result.end());
    return result;
}

// ===== Fuzzy Search =====
void SearchEngine::collectWords(AVLTree<string, int>::AVLNode *node, vector<string> &words) const
{
    if (node == nullptr)
        return;
    collectWords(node->pLeft, words);
    words.push_back(node->key);
    collectWords(node->pRight, words);
}

vector<string> SearchEngine::suggestSimilar(const string &keyword, int maxSuggestions)
{
    vector<string> allWords;
    collectWords(wordFrequency.getRoot(), allWords);

    vector<pair<int, string>> distances;
    for (const string &w : allWords)
    {
        int d = StringUtils::levenshteinDistance(keyword, w);
        distances.push_back({d, w});
    }
    sort(distances.begin(), distances.end());

    vector<string> result;
    for (int i = 0; i < min((int)distances.size(), maxSuggestions); i++)
    {
        result.push_back(distances[i].second);
    }
    return result;
}