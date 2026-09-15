#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "../include/file_indexer.h"
#include "../include/sqlite_index_store.h"

namespace
{
    double index_and_time(custom::file_indexer& indexer, const std::filesystem::path& dir)
    {
        const auto start = std::chrono::steady_clock::now();
        indexer.index_directory(dir);
        const auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }

    // Works with any source exposing `std::vector<std::string> query(word)`:
    // the in-memory file_indexer and the SQLite-backed store both qualify.
    template<typename QuerySource>
    void run_query_loop(QuerySource& source)
    {
        std::cout << "\nType a word to search (empty line to quit):\n";
        std::string line;
        while (std::cout << "> " && std::getline(std::cin, line))
        {
            if (line.empty())
            {
                break;
            }
            const auto hits = source.query(line);
            if (hits.empty())
            {
                std::cout << "  no files contain \"" << line << "\"\n";
                continue;
            }
            std::cout << "  " << hits.size() << " file(s):\n";
            for (const auto& f : hits)
            {
                std::cout << "    " << f << '\n';
            }
        }
    }

    // Index the same directory with 1/2/4/8 threads and report speedup.
    void run_benchmark(const std::filesystem::path& dir)
    {
        const std::vector<std::size_t> configs{1, 2, 4, 8};
        std::cout << "Benchmarking index of " << dir << "\n\n";
        std::cout << "threads |   time (ms) | files | speedup\n";
        std::cout << "--------+-------------+-------+--------\n";
        double baseline_ms = 0.0;
        for (std::size_t t : configs)
        {
            custom::file_indexer indexer(t);
            const double ms = index_and_time(indexer, dir);
            if (t == configs.front())
            {
                baseline_ms = ms;
            }
            std::printf("%7zu | %11.1f | %5zu | %6.2fx\n",
                        t, ms, indexer.indexed_files(), baseline_ms / ms);
        }
    }
}

void print_usage(const char* prog)
{
    std::cerr << "Usage:\n"
              << "  " << prog << " <directory> [thread_count]   index and search interactively\n"
              << "  " << prog << " <directory> --bench          benchmark 1/2/4/8 threads\n"
              << "  " << prog << " <directory> --save <db>       index, then save to a SQLite file\n"
              << "  " << prog << " --load <db>                   open a saved index and search it\n";
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return 1;
    }

    const std::string first = argv[1];

    // --load <db>: no indexing at all; query the SQLite file directly.
    if (first == "--load")
    {
        if (argc < 3)
        {
            std::cerr << "error: --load needs a database path\n";
            return 1;
        }
        try
        {
            custom::sqlite_index_store store(argv[2]);
            std::cout << "Loaded index from " << argv[2] << " (no re-indexing).\n";
            run_query_loop(store);
        }
        catch (const std::exception& e)
        {
            std::cerr << "error: " << e.what() << '\n';
            return 1;
        }
        return 0;
    }

    const std::filesystem::path dir = first;
    if (!std::filesystem::is_directory(dir))
    {
        std::cerr << "error: " << dir << " is not a directory\n";
        return 1;
    }

    const std::string second = (argc >= 3) ? argv[2] : "";

    if (second == "--bench")
    {
        run_benchmark(dir);
        return 0;
    }

    // Index in memory using the parallel pipeline (shared by every mode below).
    std::size_t threads = std::thread::hardware_concurrency();
    if (threads == 0)
    {
        threads = 1; // hardware_concurrency() may report 0; match the pool's clamp.
    }
    if (second == "--save")
    {
        if (argc < 4)
        {
            std::cerr << "error: --save needs a database path\n";
            return 1;
        }
    }
    else if (!second.empty())
    {
        threads = static_cast<std::size_t>(std::max(1, std::atoi(second.c_str())));
    }

    custom::file_indexer indexer(threads);
    double ms = 0.0;
    try
    {
        ms = index_and_time(indexer, dir);
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
    std::cout << "Indexed " << indexer.indexed_files() << " files, "
              << indexer.unique_words() << " unique words in "
              << ms << " ms using " << threads << " threads.\n";

    if (second == "--save")
    {
        try
        {
            custom::sqlite_index_store::save(indexer.index(), argv[3]);
            std::cout << "Saved index to " << argv[3] << ". Reopen it with --load "
                      << argv[3] << ".\n";
        }
        catch (const std::exception& e)
        {
            std::cerr << "error: " << e.what() << '\n';
            return 1;
        }
        return 0;
    }

    run_query_loop(indexer);
    return 0;
}
