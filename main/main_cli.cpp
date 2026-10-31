#include <iostream>
#include <fstream>
#include <sstream>
#include "SearchEngine.h"
#include "Tokenizer.h"
#include "benchmark.h"
#include "../utils/SentenceUtils.h"

int loadDocumentsFromFile(SearchEngine &engine, const std::string &filepath)
{
    std::ifstream file(filepath);
    std::string line;
    int count = 0;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;
        engine.addDocument(line);
        count++;
    }
    return count;
}

// Escape special character for enabled JSON
std::string jsonEscape(const std::string &s)
{
    std::string out;
    for (char c : s)
    {
        switch (c)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
        }
    }
    return out;
}

// ===== In JSON cho từng mode =====

void printJsonBasic(const std::vector<int> &docIds, SearchEngine &engine)
{
    std::cout << "[";
    for (size_t i = 0; i < docIds.size(); ++i)
    {
        std::cout << docIds[i];
        if (i + 1 < docIds.size())
            std::cout << ",";
    }
    std::cout << "]";
}

#include "../utils/SentenceUtils.h"

void printJsonRanked(const std::vector<SearchEngine::ScoredDoc> &results, SearchEngine &engine, const std::string &keyword)
{
    std::cout << "[";
    for (size_t i = 0; i < results.size(); ++i)
    {
        std::string docText = engine.getDocText(results[i].docId);
        std::vector<std::string> sentences = SentenceUtils::splitSentences(docText);
        int sentIdx = SentenceUtils::findSentenceIndex(sentences, keyword);

        std::cout << "{\"docId\":" << results[i].docId
                  << ",\"score\":" << results[i].score
                  << ",\"sentenceIndex\":" << sentIdx
                  << ",\"sentenceText\":\"";
        if (sentIdx >= 0)
            std::cout << jsonEscape(sentences[sentIdx]);
        std::cout << "\"}";

        if (i + 1 < results.size())
            std::cout << ",";
    }
    std::cout << "]";
}

void printJsonIntArray(const std::vector<int> &ids)
{
    std::cout << "[";
    for (size_t i = 0; i < ids.size(); ++i)
    {
        std::cout << ids[i];
        if (i + 1 < ids.size())
            std::cout << ",";
    }
    std::cout << "]";
}

void printJsonStringArray(const std::vector<std::string> &items)
{
    std::cout << "[";
    for (size_t i = 0; i < items.size(); ++i)
    {
        std::cout << "\"" << jsonEscape(items[i]) << "\"";
        if (i + 1 < items.size())
            std::cout << ",";
    }
    std::cout << "]";
}

void printJsonBenchmark(const BenchmarkResult &r)
{
    std::cout << "{"
              << "\"avl\":{\"name\":\"" << r.avl.name << "\",\"timeMs\":" << r.avl.searchTimeMs
              << ",\"avgSteps\":" << r.avl.avgSteps << ",\"height\":" << r.avl.height << "},"
              << "\"stdMap\":{\"name\":\"" << r.stdMap.name << "\",\"timeMs\":" << r.stdMap.searchTimeMs << "},"
              << "\"unorderedMap\":{\"name\":\"" << r.unorderedMap.name << "\",\"timeMs\":" << r.unorderedMap.searchTimeMs << "},"
              << "\"indexSize\":" << r.indexSize
              << ",\"actualHeight\":" << r.actualHeight
              << ",\"theoreticalHeight\":" << r.theoreticalHeight
              << "}";
}

// split string by comma, auto eliminate spaces at start/ end of words
std::vector<std::string> splitByComma(const std::string &s)
{
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");

        if (start == std::string::npos) // token toan khoang trang hoac rong
            continue;
        std::string after_token = token.substr(start, end - start + 1);
        if (!token.empty())
            result.push_back(after_token);
    }
    return result;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: main_cli <mode> [keyword]\n";
        std::cerr << "Modes: basic, ranked, and, or, suggest, benchmark\n";
        return 1;
    }

    std::string mode = argv[1];
    std::string keyword = (argc >= 3) ? argv[2] : "";

    SearchEngine engine;
    loadDocumentsFromFile(engine, "data/document.txt");

    if (mode == "basic")
    {
        auto results = engine.search(keyword);
        printJsonBasic(results, engine);
    }
    else if (mode == "ranked")
    {
        auto results = engine.searchRanked(keyword);
        printJsonRanked(results, engine, keyword);
    }
    else if (mode == "and")
    {
        std::vector<std::string> keywords = splitByComma(keyword);
        auto results = engine.searchAND(keywords);
        printJsonIntArray(results);
    }
    else if (mode == "or")
    {
        std::vector<std::string> keywords = splitByComma(keyword);
        auto results = engine.searchOR(keywords);
        printJsonIntArray(results);
    }
    else if (mode == "suggest")
    {
        auto suggestions = engine.suggestSimilar(keyword, 3);
        printJsonStringArray(suggestions);
    }
    else if (mode == "benchmark")
    {
        std::vector<std::string> testWords = {"the", "a", "is", "of", "to"};
        // Nếu muốn benchmark theo từ thật trong corpus thay vì từ cố định,
        // có thể truyền keyword dạng "word1,word2,word3" và splitByComma như trên
        if (!keyword.empty())
            testWords = splitByComma(keyword);
        BenchmarkResult result = runBenchmark(engine, testWords);
        printJsonBenchmark(result);
    }
    else if (mode == "stats")
    {
        std::cout << "{\"docCount\":" << engine.getDocCount()
                  << ",\"indexSize\":" << engine.getIndexSize()
                  << ",\"height\":" << engine.getIndexHeight() << "}";
    }
    else
    {
        std::cerr << "Unknown mode: " << mode << "\n";
        return 1;
    }

    return 0;
}