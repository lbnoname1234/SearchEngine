# SearchEngine

[![Live Demo](https://img.shields.io/badge/Live%20Demo-Render-46E3B7?style=for-the-badge&logo=render&logoColor=white)](https://searchengine-vvz7.onrender.com)
**Live Web Application:** [https://searchengine-vvz7.onrender.com](https://searchengine-vvz7.onrender.com)

A high-performance C++ Search Engine built around a custom, self-implemented AVL tree used as an inverted index. The project demonstrates fundamental Information Retrieval (IR) algorithms (TF-IDF ranking, boolean queries, fuzzy search with Levenshtein distance) layered on top of a strictly balanced binary search tree, complemented by a real-time benchmarking suite and an interactive web dashboard.

---

## Key Features

- **Custom AVL Tree Inverted Index (`AVLTree<K, T>`)**:
  - Templated self-balancing binary search tree maintaining strict balance factor $|BF| \le 1$.
  - Guaranteed $O(\log n)$ worst-case search, insertion, and deletion.
  - Tree-traversal step counter (`getLastSearchSteps()`) tracking the exact depth traversed per query.
- **TF-IDF Ranked Retrieval**:
  - Calculates term relevance using Term Frequency ($TF$) and Inverse Document Frequency ($IDF$).
  - Returns ranked document snippets ordered by relevance score.
- **Boolean Search**:
  - Multi-keyword `AND` query (set intersection).
  - Multi-keyword `OR` query (set union).
- **Fuzzy Search & Typo Suggestions**:
  - Suggests the closest indexed vocabulary terms using Levenshtein edit distance when an exact match is missing.
- **Position & Sub-Word Element Highlighting**:
  - Segment documents into sentences and tracks exact word positions (sentence index, word index, global offset).
  - Strict sub-word matching for hyphenated/compound terms (e.g., matching `balance` in `self-balancing`) without false-matching unrelated single characters.
  - Cleanses UTF-8 BOM (`0xEF, 0xBB, 0xBF`) and carriage returns for clean snippet rendering.
- **Real-Time Dynamic Benchmark**:
  - Compares AVL Tree vs. `std::map` (Red-Black tree) vs. `std::unordered_map` (Hash table).
  - Measures execution time (ms), theoretical height ($1.45 \times \log_2 n$) vs. actual height, and average steps traversed.
  - Supports query-specific benchmarking, step averaging for multi-word/repeated queries, and repetition count weighting.
- **Modern Web Dashboard**:
  - Lightweight Flask backend communicating with the C++ engine through a high-speed CLI binary.
  - Clean vanilla HTML5, CSS3, and JavaScript UI with high-contrast keyword highlighting.
  - Multi-file corpus management: upload multiple `.txt` files in append mode, remove specific files, or clear all.

---

## Project Structure

```text
SearchEngine/
├── include/
│   ├── AVLTree.h          # Templated AVL Tree header & class definitions
│   ├── SearchEngine.h     # Search engine core interface & indexing logic
│   ├── Tokenizer.h        # String tokenizer interface
│   ├── document.h         # Document entity struct
│   └── benchmark.h        # Benchmark structures and prototype
├── src/
│   ├── AVLTree.cpp        # AVL Tree method implementations
│   ├── SearchEngine.cpp   # SearchEngine indexing, TF-IDF, and query methods
│   ├── tokenizer.cpp      # Token extraction and normalization
│   └── benchmark.cpp      # AVL vs std::map vs unordered_map benchmark
├── utils/
│   ├── StringUtils.h      # Levenshtein distance & string helpers
│   ├── StringUtils.cpp    # String manipulation implementation
│   ├── SentenceUtils.h    # Sentence splitting & position occurrence tracker
│   └── SentenceUtils.cpp  # Sentence boundary parsing & sub-word matching
├── main/
│   ├── main.cpp           # 78 automated unit tests + console demo
│   └── main_cli.cpp       # CLI wrapper returning structured JSON for API/Web
├── web/
│   ├── app.py             # Flask application & REST API endpoints
│   ├── search_cli.exe     # Compiled C++ binary for Windows
│   ├── static/
│   │   ├── style.css      # Modern stylesheet with accessible theme
│   │   └── script.js      # Frontend controller, search, & benchmark renderer
│   ├── templates/
│   │   └── index.html     # Web dashboard layout
│   └── data/
│       ├── document.txt   # Active compiled corpus file
│       ├── files_meta.json# Metadata for uploaded corpus files
│       └── uploads/       # Directory storing uploaded document files
├── data/
│   └── document.txt       # Default sample corpus
└── requirements.txt       # Python web dependencies (Flask, gunicorn)
```

---

## Getting Started

### Prerequisites

- **C++ Compiler**: GCC / G++ (MinGW-w64 on Windows or standard `g++` on Linux/macOS) supporting C++17 or newer.
- **Python**: Python 3.10+ (for running the web dashboard).

---

### 1. Building and Running the Test Suite (CLI Demo)

To compile and run the 78 automated unit tests and interactive console demo:

#### On Windows (PowerShell):

```powershell
g++ -O2 -Iinclude -Iutils main/main.cpp src/AVLTree.cpp src/SearchEngine.cpp src/tokenizer.cpp src/benchmark.cpp utils/StringUtils.cpp utils/SentenceUtils.cpp -o test_main.exe
.\test_main.exe
```

#### On Linux / macOS:

```bash
g++ -O2 -Iinclude -Iutils main/main.cpp src/AVLTree.cpp src/SearchEngine.cpp src/tokenizer.cpp src/benchmark.cpp utils/StringUtils.cpp utils/SentenceUtils.cpp -o test_main
./test_main
```

**What `test_main` does:**

1. **Automated Unit Tests**: Executes 78 assertions verifying Tokenizer correctness, AVL balancing invariants, word frequency counts, file parsing, and search results.
2. **Interactive Live Demo**: Indexes `data/document.txt` and prints results for Basic Search, Ranked TF-IDF, Boolean `AND`/`OR`, Fuzzy Suggestions, and the performance benchmark.

---

### 2. Compiling the Web CLI Binary

The web interface communicates directly with the C++ engine via `search_cli`:

#### On Windows:

```powershell
g++ -O2 -Iinclude -Iutils main/main_cli.cpp src/AVLTree.cpp src/SearchEngine.cpp src/tokenizer.cpp utils/StringUtils.cpp utils/SentenceUtils.cpp src/benchmark.cpp -o web/search_cli.exe
```

#### On Linux / Render Deployment:

```bash
g++ -O2 -Iinclude -Iutils main/main_cli.cpp src/AVLTree.cpp src/SearchEngine.cpp src/tokenizer.cpp utils/StringUtils.cpp utils/SentenceUtils.cpp src/benchmark.cpp -o web/search_cli
```

---

### 3. Running the Web Application

> **Live Deployment:** The application is live at [https://searchengine-vvz7.onrender.com](https://searchengine-vvz7.onrender.com).

To run locally:

1. Install Python dependencies:

   ```bash
   pip install -r requirements.txt
   ```

2. Start the local Flask web server:

   ```powershell
   python web/app.py
   ```

3. Open your browser and navigate to:
   ```text
   http://127.0.0.1:5000
   ```

---

## CLI Usage (JSON API)

`main_cli` provides a scriptable interface that outputs structured JSON for external tools:

```bash
# Basic exact search
./search_cli basic "tree"

# TF-IDF ranked search
./search_cli ranked "balance"

# Boolean AND query
./search_cli and "tree, search"

# Boolean OR query
./search_cli or "binary, red"

# Typo suggestions
./search_cli suggest "balnce"

# Run benchmark on custom terms (supports repeated words)
./search_cli benchmark "tree, balance, tree"

# Index statistics
./search_cli stats
```

---

## Data Structure Comparison (Benchmark)

| Data Structure             |      Lookup Time Complexity      | Ordering  | Rebalance Overhead | Use Case in Engine                |
| :------------------------- | :------------------------------: | :-------: | :----------------: | :-------------------------------- | ------- | ----------------------------------------- |
| **AVL Tree (Custom)**      | $\mathcal{O}(\log n)$ worst-case |  Ordered  |     Strict ($      | BF                                | \le 1$) | Primary inverted index & term frequencies |
| **`std::map` (Red-Black)** | $\mathcal{O}(\log n)$ worst-case |  Ordered  |      Relaxed       | Benchmark baseline (balanced BST) |
| **`std::unordered_map`**   |     $\mathcal{O}(1)$ average     | Unordered |    Hash buckets    | Benchmark baseline (hash table)   |

- **Height Guarantee**: The AVL tree height is bounded strictly by $h \le 1.44 \times \log_2(n + 2)$, preventing degenerate tree degradation $\mathcal{O}(n)$ found in un-balanced BSTs.
- **Steps Traversal**: Each query reports the exact number of tree node comparisons required from root to match, allowing direct verification of algorithmic depth.

---

## License

This project is developed for educational and demonstration purposes. Feel free to use and extend the codebase.
