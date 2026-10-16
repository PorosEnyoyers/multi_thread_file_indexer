#include <gtest/gtest.h>
#include "file_indexer.h"
#include <filesystem>
#include <fstream>
#include <string>

TEST(FileIndexer, InvalidDirectoryReturnsMinusOne) {
    custom::file_indexer indexer{};
    EXPECT_EQ(indexer.start("/this/path/does/not/exist"), -1);
}

TEST(FileIndexer, IndexesLargeNestedDirectory) {
    namespace fs = std::filesystem;
    fs::path root = fs::temp_directory_path() / "indexer_big_test";
    fs::remove_all(root);

    // 20 folders x 10 subfolders x 5 files = 1000 files
    for (int i = 0; i < 20; ++i) {
        for (int j = 0; j < 10; ++j) {
            fs::path d = root / ("dir" + std::to_string(i)) / ("sub" + std::to_string(j));
            fs::create_directories(d);
            for (int k = 0; k < 5; ++k) {
                std::ofstream f(d / ("file" + std::to_string(k) + ".txt"));
                if (k != 0) f << "word" << i << " word" << j << " shared common text\n";
                // k == 0 stays empty on purpose: empty files are a classic edge case
            }
        }
    }

    {
        custom::file_indexer indexer{};
        EXPECT_NE(indexer.start(root.string()), -1);
        indexer.loading();   // see note below
    }   // indexer destroyed here, before the directory is deleted

    fs::remove_all(root);
}