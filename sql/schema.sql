-- Schema for the persistent inverted index (see include/sqlite_index_store.h).
--
-- NOTE: the application creates these tables automatically when you run
--   multi_thread_file_indexer <dir> --save index.db
-- This file is only needed to inspect/rebuild the schema by hand (e.g. in
-- DataGrip). Running --save on an existing file OVERWRITES it, so tables you
-- create here manually will be replaced by the app.

-- Recommended pragmas for bulk loading.
PRAGMA journal_mode = WAL;
PRAGMA synchronous  = NORMAL;

-- Every distinct word, stored exactly once.
CREATE TABLE IF NOT EXISTS terms (
    term_id INTEGER PRIMARY KEY,
    word    TEXT UNIQUE NOT NULL
);

-- Every distinct file path, stored exactly once.
CREATE TABLE IF NOT EXISTS docs (
    doc_id INTEGER PRIMARY KEY,
    path   TEXT UNIQUE NOT NULL
);

-- "this word appears in this file". Compact integer pairs; the composite
-- primary key is also the clustered index and rejects duplicate postings.
CREATE TABLE IF NOT EXISTS postings (
    term_id INTEGER NOT NULL,
    doc_id  INTEGER NOT NULL,
    PRIMARY KEY (term_id, doc_id)
) WITHOUT ROWID;

-- ---------------------------------------------------------------------------
-- Example queries
-- ---------------------------------------------------------------------------

-- Files that contain a given word:
-- SELECT d.path
-- FROM postings p
-- JOIN terms t ON t.term_id = p.term_id
-- JOIN docs  d ON d.doc_id  = p.doc_id
-- WHERE t.word = 'mutex'
-- ORDER BY d.path;

-- Row counts per table:
-- SELECT 'terms' AS tbl, COUNT(*) FROM terms
-- UNION ALL SELECT 'docs',     COUNT(*) FROM docs
-- UNION ALL SELECT 'postings', COUNT(*) FROM postings;

-- Top 10 most widespread words (appear in the most files):
-- SELECT t.word, COUNT(*) AS files
-- FROM postings p
-- JOIN terms t ON t.term_id = p.term_id
-- GROUP BY p.term_id
-- ORDER BY files DESC
-- LIMIT 10;
