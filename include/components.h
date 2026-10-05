#pragma once

#include <atomic>
#include <memory>
#include <utility>
#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <string_view>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <string_view>
#include <system_error>
#include <ostream>
#include <vector>
#include "/home/khoip/projects/Custom_Container_Library/include/Red_Black_Tree.h"

namespace custom
{
    struct File_Record
    {
        std::string path;
        bool is_dir;
        std::size_t size;
        std::chrono::system_clock::time_point mod_time;
    };
    struct File_Record_Size
    {
        using value_type = std::size_t;
        value_type key;
        std::vector<File_Record*> files;
        bool operator<(const File_Record_Size& other) const
        {
            return this->key < other.key;
        }
        bool operator>(const File_Record_Size& other) const
        {
            return other < *this;
        }
        bool operator==(const File_Record_Size& other) const
        {
            return this->key == other.key;
        }
        bool operator<=(const File_Record_Size& other) const
        {
            return *this < other || *this == other;
        }
        bool operator>=(const File_Record_Size& other) const
        {
            return other <= *this;
        }
    };
    struct File_Record_Mod_Time
    {
        using value_type = std::chrono::system_clock::time_point;
        value_type key;
        std::vector<File_Record*> files;
        bool operator<(const File_Record_Mod_Time& other) const
        {
            return this->key < other.key;
        }
        bool operator>(const File_Record_Mod_Time& other) const
        {
            return other < *this;
        }
        bool operator==(const File_Record_Mod_Time& other) const
        {
            return this->key == other.key;
        }
        bool operator<=(const File_Record_Mod_Time& other) const
        {
            return *this < other || *this == other;
        }
        bool operator>=(const File_Record_Mod_Time& other) const
        {
            return other <= *this;
        }
    };
    struct thread_safe_map
    {
        std::unordered_map<std::string, std::unique_ptr<File_Record>> storage;
        std::mutex lk;
    };
    template<typename T>
    struct thread_safe_tree
    {
        custom::RB_Tree<T> tree;
        std::mutex lk;
    };
    enum class failed_op
    {
        opendir,
        lstat,
        readdir,
        process_path,
        process_size,
        process_mod,
        exception_thrown,
    };
    static std::vector<std::string> failed_op_strings{"opendir", "lstat","readdir","process_path","process_size","process_mod","exception thrown"};
    struct failed_log
    {
        std::string path;
        int error_code;
        failed_op operation;
        std::string_view print_op(failed_op op) const
        {
            switch(op)
            {
                case(failed_op::opendir):
                    return failed_op_strings[0];
                case(failed_op::lstat):
                    return failed_op_strings[1];
                case(failed_op::readdir):
                    return failed_op_strings[2];
                case(failed_op::process_path):
                    return failed_op_strings[3];
                case(failed_op::process_size):
                    return failed_op_strings[4];
                case(failed_op::process_mod):
                    return failed_op_strings[5];
                default:
                    return failed_op_strings[6];
            }
        }
        friend std::ostream& operator<<( std::ostream& out, const failed_log& log)
        {
            out << log.path << " cannot be processed_" << std::error_code(log.error_code, std::generic_category()).message() << "_Failed at operation: " << log.print_op(log.operation) <<'\n';
            return out;
        }
    };
    struct log_container
    {
        std::vector<failed_log> logs;
        std::mutex lk;
        void add(std::string path, int error_code, failed_op operation)
        {
            std::lock_guard<std::mutex> lk (this->lk);
            logs.push_back({std::move(path), error_code, operation});
        }
        friend std::ostream& operator<<( std::ostream& out, const log_container& log)
        {
            if(log.logs.size() == 0)
            {
                std::cout << "No exception throwns!!!\n";
                return out;
            }
            for(auto i : log.logs)
            {
                out << i;
            }
            return out;
        }
    };
    class outstanding_work_guard
    {
        public:
        outstanding_work_guard(std::atomic_int64_t& num)
        : outstanding_work{num}
        {
        }
        ~outstanding_work_guard()
        {
            --outstanding_work;
        }
        private:
        std::atomic_int64_t& outstanding_work;
    };
}