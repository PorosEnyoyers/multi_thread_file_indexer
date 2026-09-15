#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace custom
{
    // Thread-safe inverted index: word -> set of files that contain it.
    //
    // The map is split into SHARD_COUNT independent buckets, each with its own
    // mutex. Threads indexing different words usually hit different shards, so
    // they don't contend on a single global lock. This is the main scalability
    // trick that lets many workers write the index in parallel.
    class inverted_index
    {
    public:
        void add(const std::string& word, const std::string& path)
        {
            shard& s = shard_for(word);
            std::lock_guard<std::mutex> lk{s.mut};
            s.map[word].insert(path);
        }

        // Returns the sorted list of files containing `word` (empty if none).
        std::vector<std::string> query(const std::string& word) const
        {
            const shard& s = shard_for(word);
            std::lock_guard<std::mutex> lk{s.mut};
            auto it = s.map.find(word);
            if (it == s.map.end())
            {
                return {};
            }
            std::vector<std::string> result(it->second.begin(), it->second.end());
            std::sort(result.begin(), result.end());
            return result;
        }

        std::size_t unique_words() const
        {
            std::size_t total = 0;
            for (const shard& s : m_shards)
            {
                std::lock_guard<std::mutex> lk{s.mut};
                total += s.map.size();
            }
            return total;
        }

        // Visit every (word, set-of-files) entry, one shard at a time. Used to
        // dump the finished index into a persistent store. Not meant to run
        // concurrently with writers: call it only after indexing has finished.
        // fn is invoked as: fn(const std::string& word,
        //                      const std::unordered_set<std::string>& paths)
        template<typename Fn>
        void for_each(Fn&& fn) const
        {
            for (const shard& s : m_shards)
            {
                std::lock_guard<std::mutex> lk{s.mut};
                for (const auto& [word, paths] : s.map)
                {
                    fn(word, paths);
                }
            }
        }

    private:
        static constexpr std::size_t SHARD_COUNT = 16;

        struct shard
        {
            mutable std::mutex mut;
            std::unordered_map<std::string, std::unordered_set<std::string>> map;
        };

        shard& shard_for(const std::string& word)
        {
            return m_shards[std::hash<std::string>{}(word) % SHARD_COUNT];
        }
        const shard& shard_for(const std::string& word) const
        {
            return m_shards[std::hash<std::string>{}(word) % SHARD_COUNT];
        }

        std::array<shard, SHARD_COUNT> m_shards;
    };
}
