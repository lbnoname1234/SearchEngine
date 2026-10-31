#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <string>
using namespace std;
struct Document
{
    int id;
    string text;
    int wordCount = 0;
};

#endif