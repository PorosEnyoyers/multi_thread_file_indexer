#pragma once

#include <atomic>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <future>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "./inverted_index.h"
#include "./text_util.h"
#include "./thread_pool.h"

namespace custom
{
    // Multithreaded file indexer.
    //
    // index_directory() walks a directory tree on the calling thread (the
    // "producer") and submits one indexing task per file to the thread pool.
    // Workers (the "consumers") read each file, tokenize it, and populate the
    // shared inverted index in parallel. index_directory() blocks until every
    // file has been processed.
    class file_indexer
    {
    public:
        explicit file_indexer(std::size_t thread_count)
            : m_pool{thread_count}
        {
        }

        void index_directory(const std::filesystem::path& root)
        {
            namespace fs = std::filesystem;
            std::vector<std::future<void>> pending;
            std::error_code ec;

            for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
                 it != end;
                 it.increment(ec))
            {
                if (ec)
                {
                    ec.clear();
                    continue;
                }
                if (!it->is_regular_file(ec))
                {
                    continue;
                }
                const fs::path path = it->path();
                pending.push_back(m_pool.submit([this, path] { index_file(path); }));
            }

            // Wait for all tasks; get() also propagates any worker exception.
            for (auto& f : pending)
            {
                f.get();
            }
        }

        std::vector<std::string> query(const std::string& word) const
        {
            return m_index.query(normalize_word(word));
        }

        // Read-only access to the finished index (used to persist it).
        const inverted_index& index() const noexcept { return m_index; }

        std::size_t indexed_files() const noexcept { return m_file_count.load(); }
        std::size_t unique_words() const { return m_index.unique_words(); }

    private:
        void index_file(const std::filesystem::path& path)
        {
            std::ifstream in(path, std::ios::binary);
            if (!in)
            {
                return;
            }
            std::ostringstream ss;
            ss << in.rdbuf();
            const std::string content = ss.str();

            // Collect the file's distinct words locally first, then touch the
            // shared index once per word to keep lock traffic low.
            std::unordered_set<std::string> words;
            std::string token;
            for (unsigned char c : content)
            {
                if (std::isalnum(c))
                {
                    token.push_back(static_cast<char>(std::tolower(c)));
                }
                else if (!token.empty())
                {
                    words.insert(token);
                    token.clear();
                }
            }
            if (!token.empty())
            {
                words.insert(token);
            }

            const std::string path_str = path.string();
            for (const auto& w : words)
            {
                m_index.add(w, path_str);
            }
            m_file_count.fetch_add(1, std::memory_order_relaxed);
        }

        thread_pool m_pool;
        inverted_index m_index;
        std::atomic<std::size_t> m_file_count{0};
    };
}
