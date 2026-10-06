#pragma once
#include "file_indexer.h"
#include <charconv>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <./components.h>

namespace UI
{
    std::size_t to_size_t(const std::string& string);
    void main_flow(custom::file_indexer& indexer);
    std::vector<custom::File_Record*> process_command(std::vector<std::string> tokens, custom::file_indexer& indexer);
    int process_type(const std::string& key);
    bool range_search(const std::string& key);
}