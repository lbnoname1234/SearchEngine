#ifndef SENTENCE_UTILS_H
#define SENTENCE_UTILS_H

#include <string>
#include <vector>

struct WordOccurrence
{
    int sentenceIndex;       // 0-based sentence index in document
    int wordIndex;           // 0-based word index within sentence
    int globalWordIndex;     // 0-based word index in document
    std::string matchedWord; // The token that matched
    bool isExact;            // true = exact match, false = substring/element match
    std::string sentenceText;// Full sentence text containing the occurrence
};

class SentenceUtils
{
public:
    // separating by '.' or '!', '?' sign
    static std::vector<std::string> splitSentences(const std::string &text);

    // 0-based, returns -1 if not found
    static int findSentenceIndex(const std::vector<std::string> &sentences, const std::string &keyword);

    // Find all occurrences of a single keyword (exact and element/substring)
    static std::vector<WordOccurrence> findAllPositions(const std::string &docText, const std::string &keyword);

    // Find all occurrences across multiple keywords (e.g. for AND / OR search)
    static std::vector<WordOccurrence> findAllPositions(const std::string &docText, const std::vector<std::string> &keywords);
};

#endif
