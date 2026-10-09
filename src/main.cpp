#include "../include/file_indexer.h"
#include "../include/User_Interface.h"
int main()
{
    custom::file_indexer indexer{};
    std::string starting_dir;
    std::cout << "Thread pool created.\n" << indexer;
    std::cout <<"\n\nEnter the directory to index: ";
    std::getline(std::cin, starting_dir);
    if(indexer.start(starting_dir) == -1)
    {
        return 0;
    }
    indexer.loading();
    UI::main_flow(indexer);
    return 0;
}