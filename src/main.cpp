#include "../include/file_indexer.h"

int main()
{
    custom::file_indexer indexer{};
    indexer.start("/");
    indexer.loading();
    return 0;
}