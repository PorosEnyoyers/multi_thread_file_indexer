#include "../include/file_indexer.h"
#include "../include/User_Interface.h"
int main()
{
    custom::file_indexer indexer{};
    if(indexer.start("/usr") == -1)
    {
        return 0;
    }
    indexer.loading();
    UI::main_flow(indexer);
    return 0;
}