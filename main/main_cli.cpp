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
        // Loại bỏ BOM UTF-8 (\xEF\xBB\xBF) nếu có ở đầu dòng
        if (line.size() >= 3 &&
            (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF)
        {
            line = line.substr(3);
        }
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
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

void printJsonPositions(const std::vector<WordOccurrence> &positions)
{
    std::cout << "[";
    for (size_t p = 0; p < positions.size(); ++p)
    {
        const auto &pos = positions[p];
        std::cout << "{"
                  << "\"sentenceIndex\":" << pos.sentenceIndex
                  << ",\"wordIndex\":" << pos.wordIndex
                  << ",\"globalWordIndex\":" << pos.globalWordIndex
                  << ",\"matchedWord\":\"" << jsonEscape(pos.matchedWord) << "\""
                  << ",\"isExact\":" << (pos.isExact ? "true" : "false")
                  << ",\"snippet\":\"" << jsonEscape(pos.sentenceText) << "\""
                  << "}";
        if (p + 1 < positions.size())
            std::cout << ",";
    }
    std::cout << "]";
}

void printJsonRanked(const std::vector<SearchEngine::ScoredDoc> &results, SearchEngine &engine, const std::string &keyword)
{
    std::cout << "[";
    for (size_t i = 0; i < results.size(); ++i)
    {
        std::string docText = engine.getDocText(results[i].docId);
        std::vector<WordOccurrence> occurrences = SentenceUtils::findAllPositions(docText, keyword);
        int sentIdx = occurrences.empty() ? -1 : occurrences[0].sentenceIndex;
        std::string firstSnippet = occurrences.empty() ? "" : occurrences[0].sentenceText;

        std::cout << "{\"docId\":" << results[i].docId
                  << ",\"score\":" << results[i].score
                  << ",\"sentenceIndex\":" << sentIdx
                  << ",\"sentenceText\":\"" << jsonEscape(firstSnippet) << "\""
                  << ",\"totalOccurrences\":" << occurrences.size()
                  << ",\"positions\":";
        printJsonPositions(occurrences);
        std::cout << "}";

        if (i + 1 < results.size())
            std::cout << ",";
    }
    std::cout << "]";
}

void printJsonWithPositions(const std::vector<int> &docIds, SearchEngine &engine, const std::vector<std::string> &keywords)
{
    std::cout << "[";
    for (size_t i = 0; i < docIds.size(); ++i)
    {
        int docId = docIds[i];
        std::string docText = engine.getDocText(docId);
        std::vector<WordOccurrence> occurrences = SentenceUtils::findAllPositions(docText, keywords);
        int sentIdx = occurrences.empty() ? -1 : occurrences[0].sentenceIndex;
        std::string firstSnippet = occurrences.empty() ? "" : occurrences[0].sentenceText;

        std::cout << "{\"docId\":" << docId
                  << ",\"sentenceIndex\":" << sentIdx
                  << ",\"sentenceText\":\"" << jsonEscape(firstSnippet) << "\""
                  << ",\"totalOccurrences\":" << occurrences.size()
                  << ",\"positions\":";
        printJsonPositions(occurrences);
        std::cout << "}";

        if (i + 1 < docIds.size())
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

void printJsonBenchmark(const BenchmarkResult &r, int repeatCount = 0)
{
    std::cout << "{"
              << "\"avl\":{\"name\":\"" << r.avl.name << "\",\"timeMs\":" << r.avl.searchTimeMs
              << ",\"avgSteps\":" << r.avl.avgSteps << ",\"height\":" << r.avl.height << "},"
              << "\"stdMap\":{\"name\":\"" << r.stdMap.name << "\",\"timeMs\":" << r.stdMap.searchTimeMs << "},"
              << "\"unorderedMap\":{\"name\":\"" << r.unorderedMap.name << "\",\"timeMs\":" << r.unorderedMap.searchTimeMs << "},"
              << "\"indexSize\":" << r.indexSize
              << ",\"actualHeight\":" << r.actualHeight
              << ",\"theoreticalHeight\":" << r.theoreticalHeight;
    if (repeatCount > 0)
    {
        std::cout << ",\"repeatCount\":" << repeatCount;
    }
    std::cout << "}";
}

// Tách danh sách từ khóa theo dấu phẩy hoặc khoảng trắng (hỗ trợ cả từ lặp lại)
std::vector<std::string> splitByComma(const std::string &s)
{
    std::vector<std::string> result;
    std::string clean = s;
    for (char &c : clean)
    {
        if (c == ',')
            c = ' ';
    }
    std::stringstream ss(clean);
    std::string token;
    while (ss >> token)
    {
        for (char &c : token)
            c = (char)tolower((unsigned char)c);
        result.push_back(token);
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
        printJsonWithPositions(results, engine, {keyword});
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
        printJsonWithPositions(results, engine, keywords);
    }
    else if (mode == "or")
    {
        std::vector<std::string> keywords = splitByComma(keyword);
        auto results = engine.searchOR(keywords);
        printJsonWithPositions(results, engine, keywords);
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
        int repeatCount = 0;
        if (!keyword.empty())
        {
            testWords = splitByComma(keyword);
            if (testWords.size() == 1)
            {
                repeatCount = engine.getFrequency(testWords[0]);
            }
            else
            {
                repeatCount = (int)testWords.size();
            }
        }
        BenchmarkResult result = runBenchmark(engine, testWords);
        printJsonBenchmark(result, repeatCount);
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