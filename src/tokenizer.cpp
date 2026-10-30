#include <iostream>
#include <vector>
#include <sstream>
#include "Tokenizer.h"
using namespace std;
vector<string> Tokenizer::tokenize(const string &text)
{
    vector<string> words;

    istringstream ss(text);
    string word;

    while (ss >> word)
    {
        size_t start = 0, end = word.size();
        while (start < end && !std::isalnum((unsigned char)word[start]))
            start++;
        while (end > start && !std::isalnum((unsigned char)word[end - 1]))
            end--;
        string cleaned = word.substr(start, end - start);
        if (cleaned.empty())
            continue;
        for (char &c : cleaned)
            c = std::tolower((unsigned char)c);
        words.push_back(cleaned);
    }
    return words;
}