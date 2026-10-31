#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <vector>
#include <string>
#include "SearchEngine.h"

// benchmark result for a data structure
struct StructureResult
{
    std::string name;
    double searchTimeMs;
    double avgSteps; // -1 if not applied
    int height;      // -1 if not applied
};

// Sumarized benchmark result
struct BenchmarkResult
{
    StructureResult avl;
    StructureResult stdMap;
    StructureResult unorderedMap;
    int indexSize;
    int actualHeight;
    double theoreticalHeight;
};

// run BenchMark
BenchmarkResult runBenchmark(SearchEngine &engine, const std::vector<std::string> &testWords);

#endif