#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include "SearchEngine.h"
#include "Tokenizer.h"
#include "AVLTree.h"
#include "benchmark.h"

// Helper check and print result
static int g_totalChecks = 0;
static int g_failedChecks = 0;

void check(bool condition, const std::string &description)
{
    g_totalChecks++;
    if (condition)
    {
        std::cout << "  [PASS] " << description << "\n";
    }
    else
    {
        g_failedChecks++;
        std::cout << "  [FAIL] " << description << "\n";
    }
}

template <typename T>
void checkEqual(const T &actual, const T &expected, const std::string &description)
{
    g_totalChecks++;
    if (actual == expected)
    {
        std::cout << "  [PASS] " << description << "\n";
    }
    else
    {
        g_failedChecks++;
        std::cout << "  [FAIL] " << description
                  << " (expected: " << expected << ", got: " << actual << ")\n";
    }
}

void checkEqual(const std::vector<int> &actual, const std::vector<int> &expected,
                const std::string &description)
{
    g_totalChecks++;
    if (actual == expected)
    {
        std::cout << "  [PASS] " << description << "\n";
    }
    else
    {
        g_failedChecks++;
        auto toStr = [](const std::vector<int> &v)
        {
            std::string s = "[";
            for (size_t i = 0; i < v.size(); ++i)
            {
                s += std::to_string(v[i]);
                if (i + 1 < v.size())
                    s += ", ";
            }
            s += "]";
            return s;
        };
        std::cout << "  [FAIL] " << description
                  << " (expected: " << toStr(expected) << ", got: " << toStr(actual) << ")\n";
    }
}

// Test Tokenizer
void testTokenizer()
{
    std::cout << "\n=== Testing Tokenizer ===\n";

    std::vector<std::string> result = Tokenizer::tokenize("Hello world");
    checkEqual<size_t>(result.size(), 2, "tokenize('Hello world') size == 2");
    if (result.size() >= 2)
    {
        checkEqual(result[0], std::string("hello"), "result[0] == 'hello'");
        checkEqual(result[1], std::string("world"), "result[1] == 'world'");
    }

    result = Tokenizer::tokenize("  spaced   out \t words\n");
    checkEqual<size_t>(result.size(), 3, "tokenize with extra whitespace size == 3");
    if (result.size() >= 3)
    {
        checkEqual(result[0], std::string("spaced"), "result[0] == 'spaced'");
        checkEqual(result[1], std::string("out"), "result[1] == 'out'");
        checkEqual(result[2], std::string("words"), "result[2] == 'words'");
    }

    result = Tokenizer::tokenize("");
    check(result.empty(), "tokenize('') is empty");

    result = Tokenizer::tokenize("single");
    checkEqual<size_t>(result.size(), 1, "tokenize('single') size == 1");
    if (!result.empty())
    {
        checkEqual(result[0], std::string("single"), "result[0] == 'single'");
    }
}

// Test SearchEngine: simple add & search
void testSearchEngineAddAndSearch()
{
    std::cout << "\n=== Testing SearchEngine (add/search) ===\n";

    SearchEngine engine;
    engine.addDocument("one");
    engine.addDocument("two");
    engine.addDocument("one");

    std::vector<int> found = engine.search("one");
    checkEqual<size_t>(found.size(), 2, "search('one') returns 2 results");
    if (found.size() >= 2)
    {
        checkEqual(found[0], 0, "found[0] == 0");
        checkEqual(found[1], 2, "found[1] == 2");
    }

    found = engine.search("two");
    checkEqual<size_t>(found.size(), 1, "search('two') returns 1 result");
    if (!found.empty())
    {
        checkEqual(found[0], 1, "found[0] == 1");
    }

    found = engine.search("missing");
    check(found.empty(), "search('missing') returns empty");
}

// Test SearchEngine: đếm số lần search() và term frequency
void testSearchEngineCounters()
{
    std::cout << "\n=== Testing SearchEngine (counters) ===\n";

    int callsBefore = SearchEngine::getSearchCount();

    SearchEngine engine;
    engine.addDocument("apple banana apple");
    engine.addDocument("banana orange apple");
    engine.addDocument("orange orange");

    int freqApple = engine.getFrequency("apple");
    int freqBanana = engine.getFrequency("banana");
    int freqOrange = engine.getFrequency("orange");
    int freqMissing = engine.getFrequency("grape");

    checkEqual(freqApple, 3, "term frequency 'apple' == 3 (2+1)");
    checkEqual(freqBanana, 2, "term frequency 'banana' == 2 (1+1)");
    checkEqual(freqOrange, 3, "term frequency 'orange' == 3 (1+2)");
    checkEqual(freqMissing, 0, "term frequency 'grape' == 0 (does not exist)");

    std::cout << "\n  Term frequency:\n";
    std::cout << "    apple  : " << freqApple << "\n";
    std::cout << "    banana : " << freqBanana << "\n";
    std::cout << "    orange : " << freqOrange << "\n";

    engine.search("apple");
    engine.search("banana");
    engine.search("orange");
    engine.search("grape");

    int callsAfter = SearchEngine::getSearchCount();
    int callsThisTest = callsAfter - callsBefore;

    checkEqual(callsThisTest, 4, "search() is called exactly 4 times in this test");

    std::cout << "\n  Total number of search() calls (entire program): "
              << SearchEngine::getSearchCount() << "\n";
}

//  Test: tính chất AVL được cập nhật đúng sau mỗi lần addDocument()
void testSearchEngineAVLUpdates()
{
    std::cout << "\n=== Testing SearchEngine (AVL update properties) ===\n";

    SearchEngine engine;

    int prevSize = engine.getIndexSize();
    checkEqual(prevSize, 0, "index size == 0 before adding document");

    std::vector<std::string> documents = {
        "apple banana",
        "banana cherry",
        "apple cherry date",
        "date elderberry fig",
        "fig grape"};

    for (size_t i = 0; i < documents.size(); ++i)
    {
        engine.addDocument(documents[i]);

        int newSize = engine.getIndexSize();
        int newHeight = engine.getIndexHeight();

        std::cout << "\n  after addDocument(\"" << documents[i] << "\"):\n";
        std::cout << "    index size   = " << newSize << "\n";
        std::cout << "    index height = " << newHeight << "\n";

        check(newSize >= prevSize, "index size is not decreased after adding document");

        if (newSize > 0)
        {
            double maxAllowedHeight = 1.45 * std::log2(newSize + 2.0) + 1;
            check(newHeight <= maxAllowedHeight + 1,
                  "Height is in AVL limit (balanced)");
        }

        prevSize = newSize;
    }

    std::cout << "\n  final AVL Tree after all document:\n";
    engine.printIndexTree();
    std::cout << "\n";

    checkEqual(engine.getIndexSize(), 7, "total number of words index == 7");

    std::cout << "\n check search() for all 7 words in index:\n";

    struct ExpectedResult
    {
        std::string word;
        std::vector<int> expectedDocs;
    };

    std::vector<ExpectedResult> expectations = {
        {"apple", {0, 2}},
        {"banana", {0, 1}},
        {"cherry", {1, 2}},
        {"date", {2, 3}},
        {"elderberry", {3}},
        {"fig", {3, 4}},
        {"grape", {4}}};

    for (const ExpectedResult &exp : expectations)
    {
        SearchEngine::resetSearchCount();

        std::vector<int> result = engine.search(exp.word);

        int callsForThisWord = SearchEngine::getSearchCount();
        int stepsForThisWord = engine.getLastSearchSteps();

        std::cout << "    search('" << exp.word << "') -> [";
        for (size_t i = 0; i < result.size(); ++i)
        {
            std::cout << result[i];
            if (i + 1 < result.size())
                std::cout << ", ";
        }
        std::cout << "]  (number of calls: " << callsForThisWord
                  << ", steps of tree traversal: " << stepsForThisWord << ")\n";

        checkEqual(result, exp.expectedDocs,
                   "search('" + exp.word + "') returns true document ID list");
        checkEqual(callsForThisWord, 1,
                   "search('" + exp.word + "') only called search() once");

        int treeHeight = engine.getIndexHeight();
        check(stepsForThisWord >= 1 && stepsForThisWord <= treeHeight,
              "Number of calls is appropriate (1 <= steps <= height)");
    }

    SearchEngine::resetSearchCount();
    std::vector<int> missing = engine.search("kiwi");
    int callsForMissing = SearchEngine::getSearchCount();
    int stepsForMissing = engine.getLastSearchSteps();

    std::cout << "    search('kiwi') -> []  (number of calls: " << callsForMissing
              << ", Steps of tree traversal: " << stepsForMissing << ")\n";

    check(missing.empty(), "search('kiwi') returns empty because it does not exist");
    checkEqual(callsForMissing, 1, "search('kiwi') only called Search for once");
}

// Helper: nạp document từ file txt, mỗi dòng là 1 document
int loadDocumentsFromFile(SearchEngine &engine, const std::string &filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "  [ERROR] Can't open file: " << filepath << "\n";
        return -1;
    }

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

    file.close();
    return count;
}

// Helper: trích ra N từ xuất hiện nhiều nhất trong file, để test/demo luôn dùng từ khóa CÓ THẬT trong document.txt, không phụ thuộc nội dung cụ thể
std::vector<std::string> extractTopWords(const std::string &filepath, int topN)
{
    std::ifstream file(filepath);
    std::map<std::string, int> freq;
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;
        std::vector<std::string> words = Tokenizer::tokenize(line);
        for (const std::string &w : words)
            freq[w]++;
    }
    file.close();

    std::vector<std::pair<std::string, int>> sorted(freq.begin(), freq.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b)
              { return a.second > b.second; });

    std::vector<std::string> result;
    for (int i = 0; i < std::min((int)sorted.size(), topN); ++i)
        result.push_back(sorted[i].first);
    return result;
}

// Test: nạp document từ file data/document.txt và kiểm tra search
void testLoadDocumentsFromFile()
{
    std::cout << "\n=== Testing SearchEngine (load from document.txt) ===\n";

    SearchEngine engine;
    int loadedCount = loadDocumentsFromFile(engine, "data/document.txt");

    check(loadedCount > 0, "read at least one document from file");
    std::cout << "  Loaded " << loadedCount << " documents from file.\n";

    std::cout << "\n  AVL Tree after loading file:\n";
    std::cout << "    size   = " << engine.getIndexSize() << "\n";
    std::cout << "    height = " << engine.getIndexHeight() << "\n";

    // Lấy 2 từ xuất hiện nhiều nhất trong chính file document.txt (không hard-code)
    std::vector<std::string> topWords = extractTopWords("data/document.txt", 2);
    check(!topWords.empty(), "found at least 1 word from document.txt");

    if (!topWords.empty())
    {
        std::vector<int> result1 = engine.search(topWords[0]);
        check(!result1.empty(), "search('" + topWords[0] + "') found at least 1 document");

        std::cout << "\n  search('" << topWords[0] << "')  -> [";
        for (size_t i = 0; i < result1.size(); ++i)
        {
            std::cout << result1[i];
            if (i + 1 < result1.size())
                std::cout << ", ";
        }
        std::cout << "]\n";
    }

    if (topWords.size() >= 2)
    {
        std::vector<int> result2 = engine.search(topWords[1]);
        check(!result2.empty(), "search('" + topWords[1] + "') found at least 1 document");

        std::cout << "  search('" << topWords[1] << "') -> [";
        for (size_t i = 0; i < result2.size(); ++i)
        {
            std::cout << result2[i];
            if (i + 1 < result2.size())
                std::cout << ", ";
        }
        std::cout << "]\n";
    }

    // words not appear example
    std::vector<int> resultMissing = engine.search("zzzznotexistxyz123");
    check(resultMissing.empty(), "search('zzzznotexistxyz123') returns empty because it does not exist");
}

// Test AVLTree
void testAVLTreeOperations()
{
    std::cout << "\n=== Testing AVLTree ===\n";

    AVLTree<std::string, std::vector<int>> tree;
    check(tree.empty(), "new tree is empty");
    checkEqual(tree.getHeight(), 0, "new tree height == 0");
    checkEqual<size_t>(tree.getSize(), 0, "new tree size == 0");

    tree.insert("b", {2});
    tree.insert("a", {1});
    tree.insert("c", {3});
    tree.insert("d", {4});

    std::cout << "\n  AVL tree after inserting b, a, c, d:\n";
    tree.printTreeStructure();
    std::cout << "\n";

    check(!tree.empty(), "tree not empty after inserts");
    checkEqual<size_t>(tree.getSize(), 4, "tree size == 4 after 4 inserts");
    check(tree.contains("a"), "tree contains 'a'");
    check(tree.contains("b"), "tree contains 'b'");
    check(tree.contains("c"), "tree contains 'c'");
    check(tree.contains("d"), "tree contains 'd'");
    check(!tree.contains("x"), "tree does not contain 'x'");

    tree.remove("b");

    std::cout << "\n  AVL tree after removing 'b':\n";
    tree.printTreeStructure();
    std::cout << "\n";

    checkEqual<size_t>(tree.getSize(), 3, "tree size == 3 after removing 'b'");
    check(!tree.contains("b"), "tree no longer contains 'b'");
    check(tree.contains("a"), "tree still contains 'a'");
    check(tree.contains("c"), "tree still contains 'c'");
    check(tree.contains("d"), "tree still contains 'd'");

    tree.clear();
    check(tree.empty(), "tree empty after clear()");
    checkEqual<size_t>(tree.getSize(), 0, "tree size == 0 after clear()");
}

// Print benchmark result (data received from benchmark.cpp)
void printBenchmarkResult(const BenchmarkResult &r)
{
    std::cout << "\n=== BENCHMARK: AVL Tree vs map vs unordered_map ===\n";

    std::cout << std::left
              << std::setw(20) << "Structure"
              << std::setw(16) << "Search (ms)"
              << std::setw(14) << "Avg Steps"
              << std::setw(10) << "Height"
              << "\n";
    std::cout << std::string(60, '-') << "\n";

    auto printRow = [](const StructureResult &s)
    {
        std::cout << std::left << std::setw(20) << s.name
                  << std::setw(16) << std::fixed << std::setprecision(4) << s.searchTimeMs
                  << std::setw(14);
        if (s.avgSteps < 0)
            std::cout << "-";
        else
            std::cout << std::fixed << std::setprecision(2) << s.avgSteps;
        std::cout << std::setw(10);
        if (s.height < 0)
            std::cout << "-";
        else
            std::cout << s.height;
        std::cout << "\n";
    };

    printRow(r.avl);
    printRow(r.stdMap);
    printRow(r.unorderedMap);

    std::cout << "\nAVL tree height in reality: " << r.actualHeight
              << " | In theory: (1.45 * log2(n)): "
              << std::fixed << std::setprecision(2) << r.theoreticalHeight << "\n";
}

// simple search, ranked, boolean, fuzzy, stats, benchmark
void runDemo()
{
    std::cout << "\n=== SEARCH ENGINE DEMO ===\n";

    SearchEngine engine;
    int loaded = loadDocumentsFromFile(engine, "data/document.txt");
    std::cout << "Loaded " << loaded << " documents from data/document.txt\n";

    // Take words from real file (ex: document.exe)
    std::vector<std::string> topWords = extractTopWords("data/document.txt", 5);

    if (topWords.empty())
    {
        std::cout << "\n[WARNING] Could not extract any keywords from document.txt, skipping demo.\n";
        return;
    }

    // Basic search
    std::cout << "\n--- Basic Search ---\n";
    std::string keyword = topWords[0];
    std::vector<int> basic = engine.search(keyword);
    std::cout << "Search \"" << keyword << "\" -> Document IDs: [";
    for (size_t i = 0; i < basic.size(); ++i)
    {
        std::cout << basic[i];
        if (i + 1 < basic.size())
            std::cout << ", ";
    }
    std::cout << "]\n";
    std::cout << "  Steps taken (AVL tree): " << engine.getLastSearchSteps() << "\n";
    engine.printDocs(basic);

    // --- Ranked Search (TF-IDF) ---
    std::cout << "\n--- Ranked Search (TF-IDF) ---\n";
    std::vector<SearchEngine::ScoredDoc> ranked = engine.searchRanked(keyword);
    std::cout << "Search \"" << keyword << "\" (ranked):\n";
    int rank = 1;
    for (const auto &sd : ranked)
    {
        std::cout << "  " << rank++ << ". Document " << sd.docId
                  << " | score: " << std::fixed << std::setprecision(4) << sd.score
                  << "\n";
    }

    // --- Boolean Query ---
    // Dùng 2-3 từ khóa lấy được từ topWords thay vì hard-code, đảm bảo luôn có
    // ý nghĩa với bất kỳ nội dung document.txt nào
    std::cout << "\n--- Boolean Query ---\n";
    std::string wordA = topWords[0];
    std::string wordB = (topWords.size() >= 2) ? topWords[1] : topWords[0];
    std::string wordC = (topWords.size() >= 3) ? topWords[2] : wordB;

    std::vector<int> andResult = engine.searchAND({wordA, wordB});
    std::vector<int> orResult = engine.searchOR({wordA, wordC});
    auto printVec = [](const std::vector<int> &v)
    {
        std::cout << "[";
        for (size_t i = 0; i < v.size(); ++i)
        {
            std::cout << v[i];
            if (i + 1 < v.size())
                std::cout << ", ";
        }
        std::cout << "]";
    };
    std::cout << "Search AND [\"" << wordA << "\", \"" << wordB << "\"] -> ";
    printVec(andResult);
    std::cout << "\n";
    std::cout << "Search OR  [\"" << wordA << "\", \"" << wordC << "\"] -> ";
    printVec(orResult);
    std::cout << "\n";

    // --- Fuzzy Search ---
    // Tạo lỗi gõ bằng cách nhân đôi ký tự cuối của từ khóa thật -> chắc chắn không trùng với bất kỳ từ nào trong index (rất khó xảy ra trùng ngẫu nhiên)
    std::cout << "\n--- Fuzzy Search ---\n";
    std::string typo = keyword + std::string(1, keyword.back());
    std::vector<int> fuzzyExact = engine.search(typo);
    if (fuzzyExact.empty())
    {
        std::cout << "Search \"" << typo << "\" -> No exact match found.\n";
        std::vector<std::string> suggestions = engine.suggestSimilar(typo, 3);
        std::cout << "Did you mean: ";
        for (size_t i = 0; i < suggestions.size(); ++i)
        {
            std::cout << "\"" << suggestions[i] << "\"";
            if (i + 1 < suggestions.size())
                std::cout << ", ";
        }
        std::cout << "\n";
    }

    // Index Stats
    std::cout << "\n--- Index Stats ---\n";
    std::cout << "Index height: " << engine.getIndexHeight() << "\n";
    std::cout << "Index size (unique words): " << engine.getIndexSize() << "\n";
    std::cout << "Total searches performed: " << SearchEngine::getSearchCount() << "\n";

    // Benchmark
    // Dùng topWords (đã trích từ document.txt) làm tập từ khóa benchmark
    BenchmarkResult bench = runBenchmark(engine, topWords);
    printBenchmarkResult(bench);

    std::cout << "\nDemo finished.\n";
}

int main()
{
    std::cout << "Running SearchEngine tests...\n";

    testTokenizer();
    testSearchEngineAddAndSearch();
    testSearchEngineCounters();
    testSearchEngineAVLUpdates();
    testLoadDocumentsFromFile();
    testAVLTreeOperations();

    std::cout << "\n========================================\n";
    std::cout << "Total checks: " << g_totalChecks << "\n";
    std::cout << "Passed:       " << (g_totalChecks - g_failedChecks) << "\n";
    std::cout << "Failed:       " << g_failedChecks << "\n";
    std::cout << "========================================\n";

    if (g_failedChecks == 0)
    {
        std::cout << "All tests passed.\n";
    }
    else
    {
        std::cout << "SOME TESTS FAILED.\n";
    }

    runDemo();

    return g_failedChecks == 0 ? 0 : 1;
}