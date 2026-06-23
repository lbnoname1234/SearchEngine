#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include "SearchEngine.h"
#include "Tokenizer.h"
#include "AVLTree.h"

// ===== Helper kiểm tra & in kết quả =====
static int g_totalChecks = 0;
static int g_failedChecks = 0;

void check(bool condition, const std::string& description) {
    g_totalChecks++;
    if (condition) {
        std::cout << "  [PASS] " << description << "\n";
    } else {
        g_failedChecks++;
        std::cout << "  [FAIL] " << description << "\n";
    }
}

template <typename T>
void checkEqual(const T& actual, const T& expected, const std::string& description) {
    g_totalChecks++;
    if (actual == expected) {
        std::cout << "  [PASS] " << description << "\n";
    } else {
        g_failedChecks++;
        std::cout << "  [FAIL] " << description
                   << " (expected: " << expected << ", got: " << actual << ")\n";
    }
}

// Overload chuyen biet cho vector<int> (vector khong co operator<< nen
// khong the dung ban template chung o tren)
void checkEqual(const std::vector<int>& actual, const std::vector<int>& expected,
                 const std::string& description) {
    g_totalChecks++;
    if (actual == expected) {
        std::cout << "  [PASS] " << description << "\n";
    } else {
        g_failedChecks++;
        auto toStr = [](const std::vector<int>& v) {
            std::string s = "[";
            for (size_t i = 0; i < v.size(); ++i) {
                s += std::to_string(v[i]);
                if (i + 1 < v.size()) s += ", ";
            }
            s += "]";
            return s;
        };
        std::cout << "  [FAIL] " << description
                   << " (expected: " << toStr(expected) << ", got: " << toStr(actual) << ")\n";
    }
}

// ===== Test Tokenizer =====
void testTokenizer() {
    std::cout << "\n=== Testing Tokenizer ===\n";

    std::vector<std::string> result = Tokenizer::tokenize("Hello world");
    checkEqual<size_t>(result.size(), 2, "tokenize('Hello world') size == 2");
    if (result.size() >= 2) {
        checkEqual(result[0], std::string("Hello"), "result[0] == 'Hello'");
        checkEqual(result[1], std::string("world"), "result[1] == 'world'");
    }

    result = Tokenizer::tokenize("  spaced   out \t words\n");
    checkEqual<size_t>(result.size(), 3, "tokenize with extra whitespace size == 3");
    if (result.size() >= 3) {
        checkEqual(result[0], std::string("spaced"), "result[0] == 'spaced'");
        checkEqual(result[1], std::string("out"), "result[1] == 'out'");
        checkEqual(result[2], std::string("words"), "result[2] == 'words'");
    }

    result = Tokenizer::tokenize("");
    check(result.empty(), "tokenize('') is empty");

    result = Tokenizer::tokenize("single");
    checkEqual<size_t>(result.size(), 1, "tokenize('single') size == 1");
    if (!result.empty()) {
        checkEqual(result[0], std::string("single"), "result[0] == 'single'");
    }
}

// ===== Test SearchEngine: add/search co ban =====
void testSearchEngineAddAndSearch() {
    std::cout << "\n=== Testing SearchEngine (add/search) ===\n";

    SearchEngine engine;
    engine.addDocument("one");
    engine.addDocument("two");
    engine.addDocument("one");

    std::vector<int> found = engine.search("one");
    checkEqual<size_t>(found.size(), 2, "search('one') returns 2 results");
    if (found.size() >= 2) {
        checkEqual(found[0], 0, "found[0] == 0");
        checkEqual(found[1], 2, "found[1] == 2");
    }

    found = engine.search("two");
    checkEqual<size_t>(found.size(), 1, "search('two') returns 1 result");
    if (!found.empty()) {
        checkEqual(found[0], 1, "found[0] == 1");
    }

    found = engine.search("missing");
    check(found.empty(), "search('missing') returns empty");
}

// ===== Test SearchEngine: dem so lan search() va term frequency =====
void testSearchEngineCounters() {
    std::cout << "\n=== Testing SearchEngine (counters) ===\n";

    int callsBefore = SearchEngine::getSearchCount();

    SearchEngine engine;
    engine.addDocument("apple banana apple");   // doc 0: apple x2, banana x1
    engine.addDocument("banana orange apple");  // doc 1: banana x1, orange x1, apple x1
    engine.addDocument("orange orange");        // doc 2: orange x2

    // --- Term frequency: tong so lan xuat hien tren toan bo documents ---
    int freqApple = engine.getFrequency("apple");
    int freqBanana = engine.getFrequency("banana");
    int freqOrange = engine.getFrequency("orange");
    int freqMissing = engine.getFrequency("grape");

    checkEqual(freqApple, 3, "term frequency 'apple' == 3 (2+1)");
    checkEqual(freqBanana, 2, "term frequency 'banana' == 2 (1+1)");
    checkEqual(freqOrange, 3, "term frequency 'orange' == 3 (1+2)");
    checkEqual(freqMissing, 0, "term frequency 'grape' == 0 (khong ton tai)");

    std::cout << "\n  Term frequency:\n";
    std::cout << "    apple  : " << freqApple << "\n";
    std::cout << "    banana : " << freqBanana << "\n";
    std::cout << "    orange : " << freqOrange << "\n";

    // --- Dem so lan goi search() ---
    engine.search("apple");
    engine.search("banana");
    engine.search("orange");
    engine.search("grape");

    int callsAfter = SearchEngine::getSearchCount();
    int callsThisTest = callsAfter - callsBefore;

    checkEqual(callsThisTest, 4, "search() duoc goi dung 4 lan trong test nay");

    std::cout << "\n  Tong so lan search() da goi (toan chuong trinh): "
               << SearchEngine::getSearchCount() << "\n";
}

// ===== Test: tinh chat AVL duoc cap nhat dung sau moi lan addDocument(),
//             kem theo kiem tra search() dung sau khi build xong toan bo index =====
void testSearchEngineAVLUpdates() {
    std::cout << "\n=== Testing SearchEngine (AVL update properties) ===\n";

    SearchEngine engine;

    int prevSize = engine.getIndexSize();
    checkEqual(prevSize, 0, "index size == 0 truoc khi them document nao");

    // Moi document them vao co the lam tang size cua cay (cac tu moi)
    // nhung KHONG bao gio lam giam size, va height luon hop ly (can bang AVL)
    std::vector<std::string> documents = {
        "apple banana",          // doc 0: apple, banana
        "banana cherry",         // doc 1: banana, cherry
        "apple cherry date",     // doc 2: apple, cherry, date
        "date elderberry fig",   // doc 3: date, elderberry, fig
        "fig grape"               // doc 4: fig, grape
    };

    for (size_t i = 0; i < documents.size(); ++i) {
        engine.addDocument(documents[i]);

        int newSize = engine.getIndexSize();
        int newHeight = engine.getIndexHeight();

        std::cout << "\n  Sau addDocument(\"" << documents[i] << "\"):\n";
        std::cout << "    index size   = " << newSize << "\n";
        std::cout << "    index height = " << newHeight << "\n";

        // Tinh chat 1: size khong bao gio giam khi them document
        check(newSize >= prevSize, "index size khong giam sau khi them document");

        // Tinh chat 2: AVL phai can bang -> height <= 1.45 * log2(size + 2) (xap xi ly thuyet AVL)
        // Quy uoc cua project: cay rong height = 0, cay 1 node height = 1
        if (newSize > 0) {
            double maxAllowedHeight = 1.45 * std::log2(newSize + 2.0) + 1;
            check(newHeight <= maxAllowedHeight + 1,
                  "height nam trong gioi han ly thuyet cua AVL (can bang)");
        }

        prevSize = newSize;
    }

    std::cout << "\n  Cay AVL cuoi cung sau tat ca document:\n";
    engine.printIndexTree();
    std::cout << "\n";

    // Tong so tu duy nhat: apple, banana, cherry, date, elderberry, fig, grape = 7 tu
    checkEqual(engine.getIndexSize(), 7, "tong so tu duy nhat trong index == 7");

    // --- Dem so lan goi search() truoc khi bat dau goi trong test nay ---
    int callsBefore = SearchEngine::getSearchCount();

    // --- Search tung tu, reset bo dem truoc moi lan, in so lan goi cho tu do ---
    std::cout << "\n  Kiem tra search() cho tat ca 7 tu trong index:\n";

    struct ExpectedResult {
        std::string word;
        std::vector<int> expectedDocs;
    };

    std::vector<ExpectedResult> expectations = {
        {"apple",      {0, 2}},
        {"banana",     {0, 1}},
        {"cherry",     {1, 2}},
        {"date",       {2, 3}},
        {"elderberry", {3}},
        {"fig",        {3, 4}},
        {"grape",      {4}}
    };

    for (const ExpectedResult& exp : expectations) {
        SearchEngine::resetSearchCount();

        std::vector<int> result = engine.search(exp.word);

        int callsForThisWord = SearchEngine::getSearchCount();
        int stepsForThisWord = engine.getLastSearchSteps();

        std::cout << "    search('" << exp.word << "') -> [";
        for (size_t i = 0; i < result.size(); ++i) {
            std::cout << result[i];
            if (i + 1 < result.size()) std::cout << ", ";
        }
        std::cout << "]  (so lan goi: " << callsForThisWord
                   << ", so buoc duyet cay: " << stepsForThisWord << ")\n";

        checkEqual(result, exp.expectedDocs,
                   "search('" + exp.word + "') tra ve dung danh sach document ID");
        checkEqual(callsForThisWord, 1,
                   "search('" + exp.word + "') chi goi search() dung 1 lan");

        // So buoc duyet cay phai <= height cua cay (khong the di sau hon chieu cao cay)
        int treeHeight = engine.getIndexHeight();
        check(stepsForThisWord >= 1 && stepsForThisWord <= treeHeight,
              "so buoc duyet cay nam trong khoang hop ly (1 <= steps <= height)");
    }

    SearchEngine::resetSearchCount();
    std::vector<int> missing = engine.search("kiwi");
    int callsForMissing = SearchEngine::getSearchCount();
    int stepsForMissing = engine.getLastSearchSteps();

    std::cout << "    search('kiwi') -> []  (so lan goi: " << callsForMissing
               << ", so buoc duyet cay: " << stepsForMissing << ")\n";

    check(missing.empty(), "search('kiwi') tra ve rong vi tu khong ton tai");
    checkEqual(callsForMissing, 1, "search('kiwi') chi goi search() dung 1 lan");
}

// ===== Test AVLTree =====
void testAVLTreeOperations() {
    std::cout << "\n=== Testing AVLTree ===\n";

    AVLTree<std::string, std::vector<int>> tree;
    check(tree.empty(), "new tree is empty");
    checkEqual(tree.getHeight(), 0, "new tree height == 0");
    checkEqual<size_t>(tree.getSize(), 0, "new tree size == 0");

    tree.insert("b", {2});
    tree.insert("a", {1});
    tree.insert("c", {3});
    tree.insert("d", {4});

    std::cout << "\n  Cay AVL sau khi insert b, a, c, d:\n";
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

    std::cout << "\n  Cay AVL sau khi remove 'b':\n";
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

int main() {
    std::cout << "Running SearchEngine tests...\n";

    testTokenizer();
    testSearchEngineAddAndSearch();
    testSearchEngineCounters();
    testSearchEngineAVLUpdates();
    testAVLTreeOperations();

    std::cout << "\n========================================\n";
    std::cout << "Total checks: " << g_totalChecks << "\n";
    std::cout << "Passed:       " << (g_totalChecks - g_failedChecks) << "\n";
    std::cout << "Failed:       " << g_failedChecks << "\n";
    std::cout << "========================================\n";

    if (g_failedChecks == 0) {
        std::cout << "All tests passed.\n";
        return 0;
    } else {
        std::cout << "SOME TESTS FAILED.\n";
        return 1;
    }
}