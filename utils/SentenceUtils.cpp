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
    std::string current;

    for (size_t i = 0; i < text.size(); ++i)
    {
        char c = text[i];
        current += c;

        bool isEnder = (c == '.' || c == '!' || c == '?');
        if (!isEnder)
            continue;

        // Bỏ qua nếu dấu '.' nằm giữa 2 chữ số (số thập phân, vd "3.14")
        if (c == '.' && i > 0 && i + 1 < text.size() &&
            isdigit((unsigned char)text[i - 1]) && isdigit((unsigned char)text[i + 1]))
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
        while (j < text.size() && (text[j] == '.' || text[j] == '!' || text[j] == '?'))
        {
            current += text[j];
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