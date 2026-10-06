#include "../include/User_Interface.h"

namespace UI
{
    bool range_search(const std::string& key)
    {
        if (key == "-range")
            return true;

        return false;
    }
    std::size_t to_size_t(const std::string& string)
    {
        std::size_t value{};
        std::from_chars_result res = std::from_chars(string.data(), string.data()+string.size(), value);
        if(res.ec != std::errc() || res.ptr != string.data()+string.size())
        {
            std::cout << "Invalid argument!!!";
            return 0;
        }
        else {
            return value;
        }
    }
    void main_flow(custom::file_indexer& indexer)
    {
        while(true)
        {
            std::cout << "Use command arguments to get the file record.\nExample: -size 10\n-path /usr/project.txt\n-size -range 50 100\nEnter your command to get files data: ";
            std::string input;
            std::getline(std::cin ,input);
            if(input == "-9999") return;
            std::istringstream stream{input};
            std::vector<std::string> tokens;
            std::string token;
            while(stream >> token)
            {
                tokens.push_back(token);
            }
            std::vector<custom::File_Record*> result = process_command(std::move(tokens), indexer);
            if(result.empty())
            {
                std::cout << "Can't find the files matching arguments!!!\n";
            }
            else {
                for(auto i : result)
                {
                    std::cout << i->path << (i->is_dir? " is a directory" : " is a file")<< ". Size: " << i->size << ". Last Mod: " << std::chrono::system_clock::to_time_t(i->mod_time) << "\n";
                }
            }
        }
    }
    std::vector<custom::File_Record*> process_command(std::vector<std::string> tokens, custom::file_indexer& indexer)
    {
        if(tokens.size() > 4 || tokens.size() < 2)
        {
            std::cout << "Fatal invalid commands!!!\n";
            return {};
        }
        int search_type = process_type(tokens[0]);
        bool is_range_search = range_search(tokens[1]);
        if(is_range_search && tokens.size() < 4)
        {
            std::cout << "Missing arguments for range search!!!\n";
            return {};
        }
        switch (search_type)
        {
            case (0):
                return indexer.find_path(tokens[1]);
            case (1):
                if(is_range_search)
                {
                    return indexer.find_size(to_size_t(tokens[2]), to_size_t(tokens[3]));
                }
                else
                {
                    return indexer.find_size(to_size_t(tokens[1]));
                }
            case (2):
                return indexer.find_mod_time(tokens[1]);
            default:
                {
                    std::cout << "IDK how it reach this brand";
                    return {};
                }
        }
    }
    int process_type(const std::string& key)
    {
        if(key == "-path")
        {
            return 0;
        }
        else if(key == "-size")
        {
            return 1;
        }
        else if(key == "-mod_time")
        {
            return 2;
        }
        else {
            std::cout << "Invalid operation!!!";
            return -1;
        }
    }
}