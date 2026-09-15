# multi_thread_file_indexer

A small command-line tool that indexes the text content of every file in a
directory tree using a pool of worker threads, then answers word queries in
constant time. Think of it as a tiny local search engine: build the index once,
then look up which files contain a word instantly.

Written in modern C++20 to practice concurrent programming from the ground up:
a thread pool, thread-safe queues, an RAII thread wrapper, a sharded concurrent
index, and a producer/consumer indexing pipeline.

## What it does

```
$ multi_thread_file_indexer <directory> [thread_count]
Indexed 1240 files, 58211 unique words in 92.3 ms using 8 threads.

Type a word to search (empty line to quit):
> mutex
  3 file(s):
    include/inverted_index.h
    include/thread_safe_queue.h
    include/two_lock_queue.h
```

## Persistence (`--save` / `--load`)

By default the index lives only in RAM and is gone when the program exits.
To keep it, use the hybrid flow: build the index in memory with the fast parallel
pipeline, then dump it to a SQLite file. Later runs load that file and query it
directly, with no re-indexing.

```bash
# Build the index in parallel, then persist it in one transaction:
multi_thread_file_indexer <directory> --save index.db

# Reopen the saved index and search it (no re-indexing):
multi_thread_file_indexer --load index.db
```

The database uses a **normalized** schema so no string is ever stored twice:

```sql
terms(term_id PK, word UNIQUE)            -- every distinct word, once
docs (doc_id  PK, path UNIQUE)            -- every distinct file path, once
postings(term_id, doc_id,                 -- "this word is in this file"
         PRIMARY KEY(term_id, doc_id)) WITHOUT ROWID
```

`postings` stores compact integer pairs instead of repeating the word/path
strings, and its composite primary key doubles as the clustered index (fast
term lookups, automatic de-duplication). A query is a three-table join:

```sql
SELECT d.path
FROM postings p
JOIN terms t ON t.term_id = p.term_id
JOIN docs  d ON d.doc_id  = p.doc_id
WHERE t.word = ?
ORDER BY d.path;
```

Write-side choices that matter: the whole dump runs inside a **single
transaction** (per-row commits would fsync thousands of times), with
`journal_mode=WAL` and `synchronous=NORMAL` for bulk loading, and prepared
statements reused across every insert.

The schema is created in code at runtime (and `--save` overwrites the file)
because the tool owns the database as a disposable, single-writer artifact: the
index is rebuilt from the source files whenever you want, so there is nothing to
migrate. A shared or long-lived database with real data would instead be
schema-first with versioned migrations. (`sql/schema.sql` documents the same
DDL for browsing the file by hand.)

## Architecture

```
                 index_directory() on the main thread (producer)
                 walks the tree with std::filesystem and submits
                 one task per file
                                   │
                                   ▼
        ┌──────────────  ts_queue<task>  ──────────────┐   (single-mutex,
        │                                               │    blocking, with
        ▼               ▼               ▼               ▼    graceful shutdown)
     worker 0        worker 1        worker 2   ...  worker N   (thread_pool,
        │               │               │               │       RAII-joined)
        └───────┬───────┴───────┬───────┴───────┬───────┘
                ▼               ▼               ▼
        read file → tokenize → add(word, path) into inverted_index
                (16 shards, each with its own mutex → parallel writes)
```

| Component | File | Responsibility |
|-----------|------|----------------|
| `thread_guard` | `include/thread_guard.h` | RAII wrapper that joins a `std::thread` on destruction |
| `ts_queue` | `include/thread_safe_queue.h` | Single-mutex blocking work queue with `shutdown()` |
| `two_lock_queue` | `include/two_lock_queue.h` | Michael & Scott two-lock queue (non-blocking, atomic linkage) |
| `thread_pool` | `include/thread_pool.h` | Fixed worker pool, `submit()` returns a `std::future` |
| `inverted_index` | `include/inverted_index.h` | Sharded, thread-safe `word -> set<file>` map |
| `file_indexer` | `include/file_indexer.h` | Producer/consumer pipeline tying it together |
| `sqlite_index_store` | `include/sqlite_index_store.h` | Persists the index to a SQLite file and queries it back |
| `normalize_word` | `include/text_util.h` | Shared tokenization used at index and query time |

## Design decisions

- **Thread pool uses `ts_queue`, not `two_lock_queue`.** Workers need to *block*
  when idle and to be *woken to exit* at shutdown. Correct blocking on a two-lock
  queue is subtle (the producer can't cheaply hold the consumer's mutex to signal
  a condition variable without losing wakeups), so blocking consumption uses the
  single-mutex `ts_queue`, whose predicate is always modified under its own mutex.
- **`two_lock_queue` is kept as a race-free, non-blocking alternative.** Its
  boundary `next` pointer is a `std::atomic<node*>` with release/acquire ordering,
  so a producer and a consumer can run concurrently with no data race. It is
  verified under ThreadSanitizer (see below).
- **The index is sharded (16 buckets, one mutex each).** Workers indexing
  different words usually touch different shards, so they don't serialize on a
  single global lock — this is what makes the write path scale with cores.
- **Workers are RAII-joined.** `thread_pool` owns its workers as `thread_guard`s
  declared *after* the task queue, so on destruction the queue is shut down first,
  workers drain remaining tasks, then each `thread_guard` joins automatically.
- **Each file's words are de-duplicated locally before touching the index**, so a
  word that appears 500 times in one file takes the shard lock once, not 500 times.

## Build

Requires a C++20 compiler and CMake ≥ 3.16.

```bash
cmake -S . -B build
cmake --build build -j
```

No CMake? A single-file compile works too (link SQLite):

```bash
clang++ -std=c++20 -O2 -Iinclude src/main.cpp -lsqlite3 -o multi_thread_file_indexer
```

Requires `libsqlite3` (preinstalled on macOS; `apt install libsqlite3-dev` on
Debian/Ubuntu). The unit tests do not depend on SQLite.

## Test

```bash
cd build && ctest --output-on-failure
# or run the binary directly:
./build/unit_tests
```

The tests cover FIFO ordering, shutdown behaviour, concurrent producer/consumer
on `two_lock_queue`, the thread pool's futures, and concurrent index writes.

### ThreadSanitizer

The concurrent code is verified data-race free:

```bash
cmake -S . -B build-tsan -DENABLE_TSAN=ON
cmake --build build-tsan -j
./build-tsan/unit_tests        # no TSan warnings
```

## Benchmark

```bash
./build/multi_thread_file_indexer <large_directory> --bench
```

Example run (Apple clang, 8-core machine, this repository as input, 49 files):

```
threads |   time (ms) | files | speedup
--------+-------------+-------+--------
      1 |        46.0 |    49 |   1.00x
      2 |        15.5 |    49 |   2.97x
      4 |        11.2 |    49 |   4.09x
      8 |        10.7 |    49 |   4.30x
```

Speedup flattens past the core count and once the workload becomes I/O-bound —
expected behaviour for a read-heavy pipeline.

## License

MIT. See [LICENSE](LICENSE).
