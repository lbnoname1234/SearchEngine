#include <iostream>
#include <vector>
#include <sstream>
#include "Tokenizer.h"
using namespace std;
vector<string> Tokenizer::tokenize(const string& text)
{
    vector<string> words;

    stringstream ss(text);
    string word;

    while(ss >> word)
        words.push_back(word);

    return words;
}