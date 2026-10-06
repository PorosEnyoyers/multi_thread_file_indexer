#pragma once

#include "./components.h"
#include "./thread_pool.h"
#include <chrono>
#include <dirent.h>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include <cerrno>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <atomic>
namespace custom
{
    class file_indexer
    {
        public:
        file_indexer()
        : path_storage{}, size_storage{}, mod_storage{}, log_storage{}, outstanding_work{0}, _pool{}
        {
            log_storage.logs.reserve(100);
        }
        ~file_indexer() = default;
        int start(std::string path)
        {
            struct stat file_data;
            if(lstat(path.c_str(), &file_data) == - 1)
            {
                std::cout << "Can't open starting Path!!! Terminating program!!!";
                return -1;
            }
            custom::File_Record record{};
            record.path = path;
            record.is_dir = S_ISDIR(file_data.st_mode);
            if(!record.is_dir)
            {
                std::cout << "Summit path is not a directory!!! Terminating program!!!";
                return -1;
            }
            record.size = static_cast<std::size_t>(file_data.st_size);
            record.mod_time = std::chrono::system_clock::time_point{std::chrono::seconds{file_data.st_mtim.tv_sec}};
            std::unique_ptr<File_Record> temp = std::make_unique<File_Record>(std::move(record));
            File_Record* temp_ptr = temp.get();
            path_storage.storage[temp->path] = std::move(temp);
            File_Record_Size size_file {temp_ptr->size, {}};
            size_file.files.push_back(temp_ptr);
            size_storage.tree.insert(size_file);
            File_Record_Mod_Time mod_file{temp_ptr->mod_time, {}};
            mod_file.files.push_back(temp_ptr);
            mod_storage.tree.insert(mod_file);
            ++num_file_processed;
            ++outstanding_work;
            try
            {
                auto discard = this->_pool.submit(&custom::file_indexer::process_dir, this, std::move(path));
            }
            catch(...)
            {
                --outstanding_work;
                throw std::logic_error("Can't submit directory to scan files!!! 0 file scanned");
            }
            return 0;
        }
        void loading()
        {
            while (outstanding_work != 0)
            {
                std::cout << "\033[2J\033[H" << std::flush;
                std::cout << "Indexing in progress!!! Please Wait!!!\n";
                std::cout << "Tasks in queue: " << outstanding_work << "\n";
                std::cout << "Files Indexed: " << num_file_processed << "\n";

                std::this_thread::sleep_for(std::chrono::milliseconds{100});
            }
            _pool.shutdown();
                std::cout << "Tasks in queue: " << outstanding_work << "\n";
                std::cout << "Files Indexed: " << num_file_processed << "\n";
                std::cout << log_storage;
        }
        std::vector<File_Record*> find_path(const std::string& key)
        {
            auto result = path_storage.storage[key].get();
            if (result)
            {
                return std::vector<File_Record*>{result};
            }
            else 
            {
                return {};
            }

        }
        std::vector<File_Record*> find_size(const std::size_t& key)
        {
            auto result = size_storage.tree.find(custom::File_Record_Size{key, {}});
            if(!result.is_sentinel())
            {
                return result.get_node_ptr()->n_data.files;
            }
            return {};
        }
        std::vector<File_Record*> find_size(const std::size_t& lower, const std::size_t& upper)
        {
            std::vector<File_Record*> res{};
            if(lower > upper || (upper - lower) >= (1024 * 1024) || (lower == 0 && upper == 0))
            {
                return res;
            }
            auto iters = size_storage.tree.find_range({lower,{}}, {upper,{}});
            for(auto& i:iters)
            {
                std::vector<File_Record*>& temp = i.get_node_ptr()->n_data.files;
                res.insert(res.end(), temp.begin(), temp.end());
            }
            return res;
        }
        std::vector<File_Record*> find_mod_time(const std::string& key)
        {
            std::chrono::system_clock::time_point upper = std::chrono::system_clock::now();
            std::chrono::system_clock::time_point lower {};
            if(key == "today")
            {
                lower = upper - std::chrono::hours(24);
            }
            else if(key == "this_week")
            {
                lower = upper - std::chrono::days(7);
            }
            else if(key == "this_month")
            {
                lower = upper -std::chrono::days(30);
            }
            else if(key == "this_year")
            {
                lower = upper - std::chrono::days(365);
            }
            else 
            {
                return {};
            }
            auto iter = this->mod_storage.tree.find_range({lower, {}}, {upper,{}});
            std::vector<File_Record*> res{};
            for(auto& i:iter)
            {
                std::vector<File_Record*>& temp = i.get_node_ptr()->n_data.files;
                res.insert(res.end(), temp.begin(), temp.end());
            }
            return res;
        }
        private:
        // void loading()
        // {
        //     while (outstanding_work != 0)
        //     {
        //         std::cout << "\033[2J\033[H" << std::flush;
        //         std::cout << "Indexing in progress!!! Please Wait!!!\n";
        //         std::cout << "Tasks in queue: " << outstanding_work << "\n";
        //         std::cout << "Files Indexed: " << num_file_processed << "\n";

        //         std::this_thread::sleep_for(std::chrono::milliseconds{100});
        //     }
        //     _pool.shutdown();
        // }
        void process_dir(std::string path)
        {
            std::unique_ptr<DIR, int(*)(DIR*)> dir = {opendir(path.c_str()), &closedir};
            custom::outstanding_work_guard work_guard{this->outstanding_work};
            if(dir == nullptr)
            {
                int err = errno;
                log_storage.add(path, err, failed_op::opendir);
                return;
            }
            std::vector<File_Record> file_batch;
            file_batch.reserve(101);
            dirent* file_ptr;
            std::string file_path;
            while(true)
            {
                errno = 0;
                file_ptr = readdir(dir.get());
                if(file_ptr == nullptr) break;
                std::string_view temp (file_ptr->d_name);
                if(temp == "." || temp == "..")
                {
                    continue;
                }
                file_path = path + "/" + file_ptr->d_name;
                struct stat file_data;
                if(lstat(file_path.c_str(), &file_data) == -1)
                {
                    int err = errno;
                    log_storage.add(file_path, err,failed_op::lstat);
                    continue;
                }
                File_Record processed_file;
                if(S_ISDIR(file_data.st_mode))
                {
                    processed_file.is_dir = true;
                    ++outstanding_work;
                    try
                    {
                        auto discard = _pool.submit(&file_indexer::process_dir,this,file_path);
                    }
                    catch(...)
                    {
                        --outstanding_work;
                        log_storage.add(file_path, 0,failed_op::exception_thrown);
                    }
                }
                else {
                    processed_file.is_dir = false;
                }
                processed_file.path = std::move(file_path);
                processed_file.size = static_cast<std::size_t>(file_data.st_size);
                processed_file.mod_time = std::chrono::system_clock::time_point{std::chrono::seconds(file_data.st_mtim.tv_sec)};
                file_batch.push_back(std::move(processed_file));
                if(file_batch.size() >= 100)
                {
                    ++outstanding_work;
                    try
                    {
                        auto discard = _pool.submit(&file_indexer::process_path,this,std::move(file_batch),path);
                    }
                    catch(...)
                    {
                        --outstanding_work;
                        log_storage.add(path, 0,failed_op::exception_thrown);
                    }
                    file_batch = std::vector<File_Record>{};
                    file_batch.reserve(101);
                }
            }
            int err = errno;
            if(!file_batch.empty())
            {
                ++outstanding_work;
                try
                {
                auto discard = _pool.submit(&file_indexer::process_path,this,std::move(file_batch),path);
                }
                catch(...)
                {
                    --outstanding_work;
                    log_storage.add(path, 0,failed_op::exception_thrown);
                }
            }
            if(err != 0)
            {
                log_storage.add(path, err, failed_op::readdir);
                return;
            }
        }
        void process_path(std::vector<File_Record> records,std::string path)
        {
            custom::outstanding_work_guard work_guard{this->outstanding_work};
            std::vector<File_Record*> rec_ptrs;
            rec_ptrs.reserve(100);
            try
            {
                std::lock_guard<std::mutex> lk{path_storage.lk};
                for(File_Record i : records)
                {
                    std::unique_ptr<File_Record> temp = std::make_unique<File_Record>(std::move(i));
                    rec_ptrs.push_back(temp.get());
                    path_storage.storage[(*temp).path] = std::move(temp);
                    ++num_file_processed;
                }
            }
            catch (...)
            {
                log_storage.add(std::move(path),0,failed_op::process_path);
                return;
            }
            ++outstanding_work;
            try 
            {
                auto discard = _pool.submit(&file_indexer::process_size,this,rec_ptrs);
            }
            catch (...) 
            {
                --outstanding_work;
                log_storage.add(path, 0, failed_op::process_path);
            }
            ++outstanding_work;
            try 
            {
                auto discard = _pool.submit(&file_indexer::process_mod,this,std::move(rec_ptrs));
            } catch (...) 
            {
                --outstanding_work;
                log_storage.add(std::move(path), 0, failed_op::exception_thrown);
            }
        }
        void process_size(std::vector<File_Record*> records)
        {
            custom::outstanding_work_guard work_guard{this->outstanding_work};
            File_Record_Size temp;
            std::string curr_file;
            try 
            {
                std::lock_guard<std::mutex> lk(size_storage.lk);
                 for(File_Record* i : records)
                {
                    curr_file = i->path;
                    temp = File_Record_Size{i->size, std::vector<File_Record*>()};
                    auto iter = size_storage.tree.find(temp);
                    if(iter == size_storage.tree.end())
                    {
                        temp.files.push_back(i);
                        size_storage.tree.insert(std::move(temp));
                    }
                    else 
                    {
                        (*iter).files.push_back(i);
                    }
                }
            }
            catch(...)
            {
                log_storage.add(std::move(curr_file), 0, failed_op::process_size);
            }
        }
        void process_mod(std::vector<File_Record*> records)
        {
            custom::outstanding_work_guard work_guard{this->outstanding_work};
            File_Record_Mod_Time temp;
            std::string curr_file;
            try 
            {            
                std::lock_guard<std::mutex> lk(mod_storage.lk);
                 for(File_Record* i : records)
                {
                    curr_file = i->path;
                    temp = File_Record_Mod_Time{i->mod_time, std::vector<File_Record*>()};
                    auto iter = mod_storage.tree.find(temp);
                    if(iter == mod_storage.tree.end())
                    {
                        temp.files.push_back(i);
                        mod_storage.tree.insert(std::move(temp));
                    }
                    else 
                    {
                        (*iter).files.push_back(i);
                    }
                }
            }
            catch(...)
            {
                log_storage.add(std::move(curr_file), 0, failed_op::process_mod);
            }
        }
        custom::thread_safe_map path_storage;
        custom::thread_safe_tree<File_Record_Size> size_storage;
        custom::thread_safe_tree<File_Record_Mod_Time> mod_storage;
        custom::log_container log_storage;
        std::atomic_int64_t outstanding_work {0};
        std::atomic_int64_t num_file_processed {0};
        custom::thread_pool _pool;
    };
}
