#pragma once

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <sqlite3.h>

#include "./inverted_index.h"
#include "./text_util.h"

namespace custom
{
    // Persists an inverted_index into a single SQLite file and queries it back.
    //
    // Schema (normalized, so no string is stored twice):
    //
    //   terms(term_id PK, word UNIQUE)      -- each distinct word, once
    //   docs (doc_id  PK, path UNIQUE)      -- each distinct file path, once
    //   postings(term_id, doc_id)           -- "this word appears in this file"
    //            PRIMARY KEY(term_id, doc_id) WITHOUT ROWID
    //
    // The postings table stores compact integer pairs instead of repeating the
    // word/path strings. Its composite primary key is also the clustered index,
    // so looking up all docs for a term is a fast range scan and duplicate
    // postings are rejected automatically.
    //
    // Usage:
    //   sqlite_index_store::save(indexer.index(), "index.db");   // write
    //   sqlite_index_store store("index.db");                    // open (read)
    //   auto files = store.query("mutex");                       // JOIN query
    class sqlite_index_store
    {
    public:
        // Write the whole in-memory index to db_path in a single transaction.
        // A single transaction is essential: committing per-row would fsync the
        // disk thousands of times and be orders of magnitude slower.
        static void save(const inverted_index& index, const std::string& db_path)
        {
            std::remove(db_path.c_str()); // overwrite: save is idempotent

            sqlite3* db = nullptr;
            if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK)
            {
                const std::string msg = db ? sqlite3_errmsg(db) : "cannot open db";
                sqlite3_close(db);
                throw std::runtime_error("save: " + msg);
            }

            try
            {
                // Bulk-load pragmas: durable enough, much faster than the default.
                exec(db, "PRAGMA journal_mode=WAL;");
                exec(db, "PRAGMA synchronous=NORMAL;");
                exec(db,
                     "CREATE TABLE terms("
                     "  term_id INTEGER PRIMARY KEY,"
                     "  word    TEXT UNIQUE NOT NULL);"
                     "CREATE TABLE docs("
                     "  doc_id INTEGER PRIMARY KEY,"
                     "  path   TEXT UNIQUE NOT NULL);"
                     "CREATE TABLE postings("
                     "  term_id INTEGER NOT NULL,"
                     "  doc_id  INTEGER NOT NULL,"
                     "  PRIMARY KEY(term_id, doc_id)) WITHOUT ROWID;");

                exec(db, "BEGIN;");

                sqlite3_stmt* ins_term = prepare(db, "INSERT INTO terms(word) VALUES(?);");
                sqlite3_stmt* ins_doc = prepare(db, "INSERT INTO docs(path) VALUES(?);");
                sqlite3_stmt* ins_post = prepare(db, "INSERT INTO postings(term_id, doc_id) VALUES(?, ?);");

                // Cache path -> doc_id so each file becomes exactly one docs row,
                // even though it is referenced by many words.
                std::unordered_map<std::string, std::int64_t> doc_ids;

                index.for_each([&](const std::string& word,
                                   const std::unordered_set<std::string>& paths) {
                    bind_text(ins_term, 1, word);
                    step_insert(db, ins_term);
                    const std::int64_t term_id = sqlite3_last_insert_rowid(db);

                    for (const std::string& path : paths)
                    {
                        std::int64_t doc_id;
                        auto it = doc_ids.find(path);
                        if (it != doc_ids.end())
                        {
                            doc_id = it->second;
                        }
                        else
                        {
                            bind_text(ins_doc, 1, path);
                            step_insert(db, ins_doc);
                            doc_id = sqlite3_last_insert_rowid(db);
                            doc_ids.emplace(path, doc_id);
                        }

                        sqlite3_bind_int64(ins_post, 1, term_id);
                        sqlite3_bind_int64(ins_post, 2, doc_id);
                        step_insert(db, ins_post);
                    }
                });

                sqlite3_finalize(ins_term);
                sqlite3_finalize(ins_doc);
                sqlite3_finalize(ins_post);

                exec(db, "COMMIT;");
            }
            catch (...)
            {
                // close_v2: safely tears down the connection even if prepared
                // statements from the failing txn are still unfinalized (a plain
                // sqlite3_close would return SQLITE_BUSY and leak the handle).
                // WAL rolls back the open txn.
                sqlite3_close_v2(db);
                throw;
            }

            sqlite3_close(db);
        }

        // Open an existing index database for read-only querying.
        explicit sqlite_index_store(const std::string& db_path)
        {
            if (sqlite3_open_v2(db_path.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
            {
                const std::string msg = m_db ? sqlite3_errmsg(m_db) : "cannot open db";
                sqlite3_close(m_db);
                throw std::runtime_error("open: " + msg);
            }
            try
            {
                // prepare can fail (e.g. --load on a db without our tables); the
                // destructor won't run for a half-built object, so close m_db here.
                m_query = prepare(m_db,
                                  "SELECT d.path "
                                  "FROM postings p "
                                  "JOIN terms t ON t.term_id = p.term_id "
                                  "JOIN docs  d ON d.doc_id  = p.doc_id "
                                  "WHERE t.word = ? "
                                  "ORDER BY d.path;");
            }
            catch (...)
            {
                sqlite3_close_v2(m_db);
                throw;
            }
        }

        sqlite_index_store(const sqlite_index_store&) = delete;
        sqlite_index_store& operator=(const sqlite_index_store&) = delete;

        ~sqlite_index_store()
        {
            if (m_query) sqlite3_finalize(m_query);
            if (m_db) sqlite3_close(m_db);
        }

        // Return the sorted files containing `word` by running the JOIN above.
        std::vector<std::string> query(const std::string& word)
        {
            const std::string w = normalize_word(word);
            sqlite3_reset(m_query);
            sqlite3_bind_text(m_query, 1, w.c_str(), -1, SQLITE_TRANSIENT);

            std::vector<std::string> result;
            while (sqlite3_step(m_query) == SQLITE_ROW)
            {
                const unsigned char* path = sqlite3_column_text(m_query, 0);
                if (path)
                {
                    result.emplace_back(reinterpret_cast<const char*>(path));
                }
            }
            return result;
        }

    private:
        static void exec(sqlite3* db, const char* sql)
        {
            char* err = nullptr;
            if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK)
            {
                const std::string msg = err ? err : "unknown error";
                sqlite3_free(err);
                throw std::runtime_error("exec failed: " + msg);
            }
        }

        static sqlite3_stmt* prepare(sqlite3* db, const char* sql)
        {
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
            }
            return stmt;
        }

        static void bind_text(sqlite3_stmt* stmt, int col, const std::string& value)
        {
            // SQLITE_TRANSIENT: SQLite copies the string, so it may outlive `value`.
            sqlite3_bind_text(stmt, col, value.c_str(), -1, SQLITE_TRANSIENT);
        }

        static void step_insert(sqlite3* db, sqlite3_stmt* stmt)
        {
            if (sqlite3_step(stmt) != SQLITE_DONE)
            {
                throw std::runtime_error(std::string("insert failed: ") + sqlite3_errmsg(db));
            }
            sqlite3_reset(stmt); // ready to be re-bound and re-used
        }

        sqlite3* m_db = nullptr;
        sqlite3_stmt* m_query = nullptr;
    };
}
