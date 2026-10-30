#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string>
using namespace std;

class StringUtils
{
public:
    static int levenshteinDistance(const string &a, const string &b);
};

#endif