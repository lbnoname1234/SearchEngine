# SearchEngine

## Overview

A small C++ search engine built around a self-implemented AVL tree used as an inverted index. The project demonstrates core information retrieval concepts (TF-IDF ranking, boolean queries, fuzzy search) layered on top of a custom balanced binary search tree, along with a benchmark comparing the AVL tree against `std::map` and `std::unordered_map`.

## Features

- **Inverted index** built on a custom `AVLTree<K, T>` (self-balancing binary search tree)
- **TF-IDF ranked search** — results ordered by term frequency–inverse document frequency score
- **Boolean queries** — `AND` / `OR` search across multiple keywords
- **Fuzzy search** — Levenshtein distance–based suggestions when a keyword has no exact match
- **Benchmark suite** — compares AVL tree search performance (time, steps, tree height) against `std::map` and `std::unordered_map`
- **File-based corpus loading** — reads documents from `data/document.txt`, one document per line

## Project structure

SearchEngine/
├── include/ # Header files
│ ├── AVLTree.h
│ ├── SearchEngine.h
│ ├── Tokenizer.h
│ ├── document.h
│ └── benchmark.h
├── src/ # Implementation files
│ ├── AVLTree.cpp
│ ├── SearchEngine.cpp
│ ├── tokenizer.cpp
│ └── benchmark.cpp
├── utils/ # Helper utilities
│ ├── StringUtils.h
│ └── StringUtils.cpp
├── main/
│ └── main.cpp # Test suite + interactive demo
└── data/
└── document.txt # Sample corpus, one document per line

## Building and running

From the `SearchEngine` folder on Windows with a C++ compiler installed (e.g. MinGW-w64 / g++):

```powershell
cd c:\Users\ADMIN\Downloads\SearchEngine
g++ -Iinclude -Iutils main/main.cpp src/AVLTree.cpp src/SearchEngine.cpp src/tokenizer.cpp src/benchmark.cpp utils/StringUtils.cpp -o test_main.exe
./test_main.exe
```

Running `test_main.exe` does two things:

1. **Runs the automated test suite** — validates the tokenizer, `SearchEngine` add/search behavior, term frequency counting, AVL tree balancing invariants, and file loading. Results print as `[PASS]` / `[FAIL]` with a final summary.
2. **Runs a live demo** — loads `data/document.txt`, then demonstrates basic search, TF-IDF ranked search, boolean `AND`/`OR` queries, fuzzy search suggestions, index statistics, and the AVL vs. `map` vs. `unordered_map` benchmark.

## How the search works

- Documents are added via `SearchEngine::addDocument(text)`. The tokenizer splits text on whitespace into lowercase tokens.
- Each unique token is stored in an AVL tree index (`AVLTree<string, vector<int>>`) mapping the word to the list of document IDs that contain it.
- A separate AVL tree (`wordFrequency`) tracks the total occurrence count of each word across the whole corpus.
- **`search(keyword)`** returns the raw list of matching document IDs, along with the number of tree-traversal steps taken (`getLastSearchSteps()`).
- **`searchRanked(keyword)`** additionally computes a TF-IDF score per matching document and returns results sorted by relevance.
- **`searchAND(keywords)` / `searchOR(keywords)`** combine results across multiple keywords using set intersection/union.
- **`suggestSimilar(keyword)`** falls back to Levenshtein-distance matching against all indexed words when there's no exact match, useful for typo correction.

## Benchmark

`runBenchmark()` measures search performance across three data structures built from the same indexed data:

| Structure                   | Guarantee           | Notes                                                                            |
| --------------------------- | ------------------- | -------------------------------------------------------------------------------- |
| AVL Tree (custom)           | O(log n) worst-case | Strict rebalancing keeps height close to the theoretical minimum (~1.45 · log₂n) |
| `std::map` (Red-Black tree) | O(log n) worst-case | Looser rebalancing than AVL; comparable performance                              |
| `std::unordered_map`        | O(1) average        | Fastest in practice, but no worst-case guarantee and no ordering                 |

The benchmark reports search time, average traversal steps, and tree height for the AVL implementation, and search time for the two STL structures, to give a concrete performance comparison rather than a purely theoretical one.

## Possible extensions

- Expose an interactive CLI to query the index at runtime instead of using fixed demo keywords
- Cache per-document word counts at index time to avoid re-tokenizing on every TF computation
- Add a REST API wrapper (e.g. via pybind11 + Flask) for a web-based demo
