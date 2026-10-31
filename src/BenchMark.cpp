#include "benchmark.h"
#include <chrono>
#include <map>
#include <unordered_map>
#include <cmath>

using namespace std;
using Clock = chrono::high_resolution_clock;

BenchmarkResult runBenchmark(SearchEngine &engine, const vector<string> &testWords)
{
    BenchmarkResult result;
    result.indexSize = engine.getIndexSize();
    result.actualHeight = engine.getIndexHeight();
    result.theoreticalHeight = (result.indexSize > 0)
                                   ? 1.45 * log2(result.indexSize + 2.0)
                                   : 0.0;

    if (testWords.empty())
    {
        result.avl = {"AVL Tree", 0.0, 0.0, result.actualHeight};
        result.stdMap = {"std::map", 0.0, -1, -1};
        result.unorderedMap = {"unordered_map", 0.0, -1, -1};
        return result;
    }

    // Build map / unordered_map từ cùng kết quả search để so sánh công bằng
    map<string, vector<int>> stdMapIndex;
    unordered_map<string, vector<int>> unorderedIndex;
    for (const string &w : testWords)
    {
        vector<int> docIds = engine.search(w);
        stdMapIndex[w] = docIds;
        unorderedIndex[w] = docIds;
    }

    const int REPEAT = 1000;

    // Đo AVL Tree
    auto t1 = Clock::now();
    for (int r = 0; r < REPEAT; r++)
        for (const string &w : testWords)
            engine.search(w);
    auto t2 = Clock::now();

    // Đo std::map
    for (int r = 0; r < REPEAT; r++)
        for (const string &w : testWords)
            volatile auto it = stdMapIndex.find(w);
    auto t3 = Clock::now();

    // Đo unordered_map
    for (int r = 0; r < REPEAT; r++)
        for (const string &w : testWords)
            volatile auto it = unorderedIndex.find(w);
    auto t4 = Clock::now();

    // Đo số bước duyệt trung bình của AVL tree
    double totalSteps = 0;
    for (const string &w : testWords)
    {
        engine.search(w);
        totalSteps += engine.getLastSearchSteps();
    }
    double avgSteps = totalSteps / testWords.size();

    result.avl = {
        "AVL Tree",
        chrono::duration<double, milli>(t2 - t1).count(),
        avgSteps,
        result.actualHeight};
    result.stdMap = {
        "std::map (RB-tree)",
        chrono::duration<double, milli>(t3 - t2).count(),
        -1,
        -1};
    result.unorderedMap = {
        "unordered_map",
        chrono::duration<double, milli>(t4 - t3).count(),
        -1,
        -1};

    return result;
}