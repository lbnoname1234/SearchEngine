#include "SentenceUtils.h"
#include "Tokenizer.h"
#include <cctype>
#include <algorithm>

// Danh sách tên viết tắt phổ biến
static const std::vector<std::string> COMMON_ABBR = {
    "mr", "mrs", "ms", "dr", "prof", "sr", "jr", "vs", "etc", "e.g", "i.e", "u.s", "u.k"};

// Kiểm tra từ (đã bỏ dấu '.') có trong danh sách viết tắt
static bool isAbbreviation(const std::string &wordBeforeDot)
{
    std::string lower = wordBeforeDot;
    for (char &c : lower)
        c = tolower((unsigned char)c);
    return std::find(COMMON_ABBR.begin(), COMMON_ABBR.end(), lower) != COMMON_ABBR.end();
}

// Lấy từ ngay trước vị trí dấu '.' vừa gặp trong buffer current
static std::string getLastWord(const std::string &current)
{
    if (current.empty())
        return "";
    size_t end = current.size() - 1; // vi tri dau '.'
    size_t start = end;
    while (start > 0 && current[start - 1] != ' ' && current[start - 1] != '\t' && current[start - 1] != '\n')
    {
        start--;
    }
    return current.substr(start, end - start); // khong lay dau '.'
}

std::vector<std::string> SentenceUtils::splitSentences(const std::string &text)
{
    std::vector<std::string> sentences;
    std::string cleanText = text;

    // Bỏ qua ký tự BOM UTF-8 (\xEF\xBB\xBF) nếu có ở đầu văn bản
    if (cleanText.size() >= 3 &&
        (unsigned char)cleanText[0] == 0xEF &&
        (unsigned char)cleanText[1] == 0xBB &&
        (unsigned char)cleanText[2] == 0xBF)
    {
        cleanText = cleanText.substr(3);
    }

    std::string current;

    for (size_t i = 0; i < cleanText.size(); ++i)
    {
        char c = cleanText[i];
        current += c;

        bool isEnder = (c == '.' || c == '!' || c == '?');
        if (!isEnder)
            continue;

        // Bỏ qua nếu dấu '.' nằm giữa 2 chữ số (số thập phân, vd "3.14")
        if (c == '.' && i > 0 && i + 1 < cleanText.size() &&
            isdigit((unsigned char)cleanText[i - 1]) && isdigit((unsigned char)cleanText[i + 1]))
        {
            continue;
        }

        // Bỏ qua nếu từ ngay trước dấu '.' là từ viết tắt phổ biến (vd "Dr.", "Mr.")
        if (c == '.')
        {
            std::string lastWord = getLastWord(current);
            if (isAbbreviation(lastWord))
            {
                continue;
            }
        }

        // Gộp các dấu thành 1 điểm kết cấu duy nhất (vd "..." "?!")
        size_t j = i + 1;
        while (j < cleanText.size() && (cleanText[j] == '.' || cleanText[j] == '!' || cleanText[j] == '?'))
        {
            current += cleanText[j];
            j++;
        }
        i = j - 1;

        size_t start = current.find_first_not_of(" \t\n");
        if (start == std::string::npos)
        {
            current.clear();
            continue;
        }
        size_t end = current.find_last_not_of(" \t\n");
        std::string sentence = current.substr(start, end - start + 1);

        bool hasAlnum = false;
        for (char sc : sentence)
        {
            if (isalnum((unsigned char)sc))
            {
                hasAlnum = true;
                break;
            }
        }
        if (hasAlnum)
            sentences.push_back(sentence);

        current.clear();
    }

    size_t start = current.find_first_not_of(" \t\n");
    if (start != std::string::npos)
    {
        size_t end = current.find_last_not_of(" \t\n");
        std::string sentence = current.substr(start, end - start + 1);
        bool hasAlnum = false;
        for (char sc : sentence)
        {
            if (isalnum((unsigned char)sc))
            {
                hasAlnum = true;
                break;
            }
        }
        if (hasAlnum)
            sentences.push_back(sentence);
    }

    return sentences;
}

int SentenceUtils::findSentenceIndex(const std::vector<std::string> &sentences, const std::string &keyword)
{
    std::string keywordLower = keyword;
    for (char &c : keywordLower)
        c = tolower((unsigned char)c);

    for (size_t i = 0; i < sentences.size(); ++i)
    {
        std::vector<std::string> words = Tokenizer::tokenize(sentences[i]);
        for (const std::string &w : words)
        {
            if (w == keywordLower)
            {
                return (int)i;
            }
        }
    }
    return -1;
}


static size_t commonPrefixLength(const std::string &s1, const std::string &s2)
{
    size_t n = std::min(s1.size(), s2.size());
    size_t i = 0;
    while (i < n && s1[i] == s2[i])
        i++;
    return i;
}

static bool isSubwordElement(const std::string &word, const std::string &kw)
{
    // Từ đơn ngắn (như 1 ký tự 'a', 'b') không bao giờ coi là từ con/element của từ khác
    if (kw.length() < 2 || word.length() <= kw.length())
        return false;

    // 1. Kiểm tra từ ghép phân tách bởi '-', '_', '/' (ví dụ "self-balancing", "sub-tree")
    size_t start = 0;
    while (start < word.size())
    {
        size_t delim = word.find_first_of("-_/", start);
        if (delim == std::string::npos)
            delim = word.size();
        std::string part = word.substr(start, delim - start);
        if (!part.empty())
        {
            if (part == kw)
                return true;
            if (kw.length() >= 3)
            {
                if (part.find(kw) != std::string::npos)
                    return true;
                size_t c = commonPrefixLength(part, kw);
                if (c >= 4 && c >= std::min(part.size(), kw.size()) - 2)
                    return true;
            }
        }
        start = delim + 1;
    }

    // 2. Kiểm tra tiếp đầu ngữ hoặc gốc từ trên toàn bộ từ (ví dụ "balancing" và "balance")
    if (kw.length() >= 3)
    {
        if (word.rfind(kw, 0) == 0)
            return true;
        size_t c = commonPrefixLength(word, kw);
        if (c >= 4 && c >= std::min(word.size(), kw.size()) - 2)
            return true;
    }

    return false;
}

std::vector<WordOccurrence> SentenceUtils::findAllPositions(const std::string &docText, const std::string &keyword)
{
    if (keyword.empty())
        return {};
    return findAllPositions(docText, std::vector<std::string>{keyword});
}

std::vector<WordOccurrence> SentenceUtils::findAllPositions(const std::string &docText, const std::vector<std::string> &keywords)
{
    std::vector<WordOccurrence> occurrences;
    if (docText.empty() || keywords.empty())
        return occurrences;

    std::vector<std::string> cleanKws;
    for (const auto &kw : keywords)
    {
        std::string lower = kw;
        for (char &c : lower)
            c = tolower((unsigned char)c);
        if (!lower.empty())
            cleanKws.push_back(lower);
    }
    if (cleanKws.empty())
        return occurrences;

    std::vector<std::string> sentences = splitSentences(docText);
    int globalWordOffset = 0;

    for (size_t sIdx = 0; sIdx < sentences.size(); ++sIdx)
    {
        std::vector<std::string> words = Tokenizer::tokenize(sentences[sIdx]);
        for (size_t wIdx = 0; wIdx < words.size(); ++wIdx)
        {
            const std::string &w = words[wIdx];
            for (const auto &kw : cleanKws)
            {
                if (w == kw)
                {
                    occurrences.push_back({
                        (int)sIdx,
                        (int)wIdx,
                        globalWordOffset + (int)wIdx,
                        w,
                        true,
                        sentences[sIdx]
                    });
                    break;
                }
                else if (isSubwordElement(w, kw))
                {
                    occurrences.push_back({
                        (int)sIdx,
                        (int)wIdx,
                        globalWordOffset + (int)wIdx,
                        w,
                        false,
                        sentences[sIdx]
                    });
                    break;
                }
            }
        }
        globalWordOffset += (int)words.size();
    }

    return occurrences;
}
