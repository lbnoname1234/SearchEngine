#include <iostream>
#include "SearchEngine.h"
#include "Tokenizer.h"
int SearchEngine::searchCount = 0;
void SearchEngine::addDocument(const string& text){
    Document doc;
    doc.id = docs.size();
    doc.text = text;
    docs.push_back(doc);

    vector<string> words = Tokenizer::tokenize(text);
    for (const string& word : words) {
        AVLTree<string, vector<int>>::AVLNode* node = index.find(word);
        if (node != nullptr) {
            if (node->data.empty() || node->data.back() != doc.id) {
                node->data.push_back(doc.id);
            }
        } else {
            index.insert(word, {doc.id});
        }
        
        AVLTree<string, int>::AVLNode* freqNode = wordFrequency.find(word);
        if (freqNode != nullptr) {
            freqNode->data += 1;
        } else {
            wordFrequency.insert(word, 1);
        }
    }
}
vector<int> SearchEngine::search(const string& keyword){
    searchCount++;
    int steps = 0;
    AVLTree<string, vector<int>>::AVLNode* node = index.find(keyword, steps);
    lastSearchSteps = steps;
    if (node != nullptr) {
        return node->data;
    }
    return {};
}

int SearchEngine::getLastSearchSteps() const {
    return lastSearchSteps;
}

int SearchEngine::getFrequency(const string& keyword) const {
    AVLTree<string, int>::AVLNode* node = wordFrequency.find(keyword);
    if (node != nullptr) return node->data;
    return 0;
}
int SearchEngine::getSearchCount() {
    return searchCount;
}
void SearchEngine::printDocs(const vector<int>& docIDs){
    for (int id : docIDs){

        cout<<"Document "<<id<<": "<<docs[id].text<<endl;
    }
}

void SearchEngine::printIndexTree() const {
    index.printTreeStructure();
}

int SearchEngine::getIndexHeight() const {
    return index.getHeight();
}

int SearchEngine::getIndexSize() const {
    return index.getSize();
}

void SearchEngine::resetSearchCount() {
    searchCount = 0;
}