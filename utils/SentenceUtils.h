#ifndef SENTENCE_UTILS_H
#define SENTENCE_UTILS_H

#include <string>
#include <vector>

class SentenceUtils
{
public:
    // separating by '.' or !', '?' sign
    static std::vector<std::string> splitSentences(const std::string &text);

    // 0-based, returns -1 if not found
    static int findSentenceIndex(const std::vector<std::string> &sentences, const std::string &keyword);
};

#endif