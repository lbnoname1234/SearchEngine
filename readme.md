# SearchEngine

## Overview
This repository contains a small C++ search engine project built around an AVL tree index and a simple whitespace tokenizer.

The current `main/main.cpp` file runs a test harness for:
- tokenizer behavior
- document addition and search
- term frequency counting
- AVL tree balancing and structure

## Sample data file
The file `data/document.txt` is provided as a sample corpus placeholder.
It is not automatically loaded by the current code, but it is the right place to store text documents if you later add file-reading support.

Each line in `data/document.txt` can represent one document to index.

## Building and running
From the `SearchEngine` folder on Windows with a C++ compiler installed, run:

```powershell
cd c:\Users\ADMIN\Downloads\SearchEngine
g++ -Iinclude main/main.cpp src/AVLTree.cpp src/SearchEngine.cpp src/tokenizer.cpp -o test_main.exe
./test_main.exe
```

## How the search works
- Documents are added using `SearchEngine::addDocument(text)`.
- The tokenizer splits text on whitespace into tokens.
- Each unique token is stored in an AVL tree index with a list of document IDs.
- Searching for a keyword returns document IDs that contain that word.

## Next steps
- Add file input support in `main/main.cpp` to read `data/document.txt`.
- Use `SearchEngine::addDocument(...)` for each line or document block.
- Add user query input to search the index at runtime.
