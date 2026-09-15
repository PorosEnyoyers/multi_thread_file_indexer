// Minimal self-contained test harness (no external framework needed).
// Exit code is 0 when every check passes, non-zero otherwise.
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../include/file_indexer.h"
#include "../include/inverted_index.h"
#include "../include/sqlite_index_store.h"
#include "../include/thread_pool.h"
#include "../include/thread_safe_queue.h"
#include "../include/two_lock_queue.h"

namespace
{
    int g_checks = 0;
    int g_failures = 0;

    void check(bool cond, const char* msg)
    {
        ++g_checks;
        if (!cond)
        {
            ++g_failures;
            std::printf("  [FAIL] %s\n", msg);
        }
    }

    void test_ts_queue()
    {
        custom::ts_queue<int> q;
        q.push(1);
        q.push(2);
        int v = 0;
        check(q.try_pop(v) && v == 1, "ts_queue FIFO first");
        check(q.try_pop(v) && v == 2, "ts_queue FIFO second");
        check(!q.try_pop(v), "ts_queue empty returns false");

        // wait_pop must be released by shutdown() so a pool can join workers.
        custom::ts_queue<int> q2;
        std::thread waiter([&] {
            int x = 0;
            check(!q2.wait_pop(x), "ts_queue wait_pop returns false on shutdown");
        });
        q2.shutdown();
        waiter.join();
    }

    void test_two_lock_queue()
    {
        custom::two_lock_queue<int> q;
        int v = 0;
        check(!q.try_pop(v), "two_lock empty returns false");
        q.push(10);
        q.push(20);
        check(q.size() == 2, "two_lock size");
        check(q.try_pop(v) && v == 10, "two_lock FIFO first");
        check(q.try_pop(v) && v == 20, "two_lock FIFO second");
        check(!q.try_pop(v), "two_lock empty again");

        // Concurrent producer + consumer: every pushed value must come out once.
        custom::two_lock_queue<int> cq;
        constexpr int N = 100000;
        std::thread producer([&] {
            for (int i = 0; i < N; ++i)
            {
                cq.push(i);
            }
        });
        long long sum = 0;
        int popped = 0;
        std::thread consumer([&] {
            int got = 0;
            while (popped < N)
            {
                if (cq.try_pop(got))
                {
                    sum += got;
                    ++popped;
                }
            }
        });
        producer.join();
        consumer.join();
        check(sum == static_cast<long long>(N) * (N - 1) / 2, "two_lock concurrent sum intact");
    }

    void test_thread_pool()
    {
        custom::thread_pool pool(4);
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 100; ++i)
        {
            futures.push_back(pool.submit([i] { return i * i; }));
        }
        long long sum = 0;
        for (auto& f : futures)
        {
            sum += f.get();
        }
        long long expected = 0;
        for (int i = 0; i < 100; ++i)
        {
            expected += static_cast<long long>(i) * i;
        }
        check(sum == expected, "thread_pool computes all tasks");
    }

    void test_inverted_index()
    {
        custom::inverted_index idx;
        idx.add("hello", "a.txt");
        idx.add("hello", "b.txt");
        idx.add("hello", "a.txt"); // duplicate path ignored
        idx.add("world", "a.txt");
        check(idx.query("hello").size() == 2, "index dedupes paths per word");
        check(idx.query("missing").empty(), "index miss returns empty");
        check(idx.unique_words() == 2, "index counts unique words");

        // Concurrent adds across shards.
        custom::inverted_index cidx;
        std::vector<std::thread> threads;
        for (int t = 0; t < 8; ++t)
        {
            threads.emplace_back([&, t] {
                for (int i = 0; i < 1000; ++i)
                {
                    cidx.add("w" + std::to_string(i), "file" + std::to_string(t));
                }
            });
        }
        for (auto& th : threads)
        {
            th.join();
        }
        check(cidx.unique_words() == 1000, "index concurrent adds: 1000 unique words");
        check(cidx.query("w0").size() == 8, "index concurrent adds: 8 files per word");
    }

    // Creates a throwaway directory with known files, indexes it in parallel,
    // then round-trips the index through SQLite. Covers file_indexer and
    // sqlite_index_store -- the two most I/O-heavy components.
    void test_file_indexer_and_store()
    {
        namespace fs = std::filesystem;
        const fs::path dir = fs::temp_directory_path() / "mtfi_unit_test";
        std::error_code ec;
        fs::remove_all(dir, ec);
        fs::create_directories(dir, ec);
        {
            std::ofstream(dir / "a.txt") << "Hello, WORLD shared";
            std::ofstream(dir / "b.txt") << "world shared extra";
        }

        custom::file_indexer indexer(4);
        indexer.index_directory(dir);

        check(indexer.indexed_files() == 2, "file_indexer counts 2 files");
        check(indexer.query("hello").size() == 1, "file_indexer normalizes 'Hello,' -> hello");
        check(indexer.query("world").size() == 2, "file_indexer case-insensitive across files");
        check(indexer.query("shared").size() == 2, "file_indexer dedupes word across files");
        check(indexer.query("extra").size() == 1, "file_indexer finds unique word");
        check(indexer.query("missing").empty(), "file_indexer miss returns empty");

        const std::string db = (dir / "index.db").string();
        custom::sqlite_index_store::save(indexer.index(), db);
        {
            custom::sqlite_index_store store(db);
            check(store.query("world").size() == 2, "sqlite roundtrip: world in 2 files");
            check(store.query("hello").size() == 1, "sqlite roundtrip: hello normalized");
            check(store.query("missing").empty(), "sqlite roundtrip: miss returns empty");
            // Persisted result must match the in-memory index (both sorted).
            check(store.query("shared") == indexer.query("shared"),
                  "sqlite roundtrip matches in-memory result");
        }

        // save() overwrites: a second save over the same path must succeed.
        custom::sqlite_index_store::save(indexer.index(), db);
        {
            custom::sqlite_index_store store(db);
            check(store.query("extra").size() == 1, "sqlite re-save is idempotent");
        }

        fs::remove_all(dir, ec);
    }
}

int main()
{
    std::printf("Running unit tests...\n");
    test_ts_queue();
    test_two_lock_queue();
    test_thread_pool();
    test_inverted_index();
    test_file_indexer_and_store();
    std::printf("%d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
