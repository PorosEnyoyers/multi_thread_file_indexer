#pragma once

#include <cctype>
#include <string>

namespace custom
{
    // Lowercase a word and drop every non-alphanumeric character, so that the
    // same normalization is applied both when indexing and when querying
    // (e.g. "Hello," and "hello" become the same token). Shared by the indexer
    // and the SQLite query path so they can never disagree.
    inline std::string normalize_word(const std::string& word)
    {
        std::string out;
        out.reserve(word.size());
        for (unsigned char c : word)
        {
            if (std::isalnum(c))
            {
                out.push_back(static_cast<char>(std::tolower(c)));
            }
        }
        return out;
    }
}
