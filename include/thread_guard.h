#pragma once
#include <thread>
#include <utility>
#include <concepts>

namespace custom
{
    template<typename Func, typename... Args>
    concept Function = requires(Func func, Args... args)
    {
        func(args...);
    };
    template<typename Func_ptr, typename Obj_ptr, typename... Args>
    concept Object_Func = requires(Func_ptr func, Obj_ptr obj, Args... args) {
        (obj->*func)(args...);
    };
    class thread_guard
    {
    public:
        //Default constructor
        thread_guard() noexcept = default;
        //Constructor that takes a function to create a thread_guard
        template<typename Func, typename... Args>
        requires Function<Func, Args...>
        explicit thread_guard(Func&& func, Args&&... args)
        : m_t{std::forward<Func>(func),std::forward<Args>(args)...}
        {
        }
        //Constructor that takes a member function pointer and object pointer to create a thread_guard
        template<typename Func_ptr, typename Obj_ptr, typename... Args>
        requires Object_Func<Func_ptr, Obj_ptr, Args...>
        explicit thread_guard(Func_ptr func, Obj_ptr obj, Args&&... args)
        : m_t{func, obj, std::forward<Args>(args)...}
        {
        }
        //Constructor that takes a thread and takes ownership of it
        explicit thread_guard(std::thread&& thread) noexcept
        : m_t{std::move(thread)}
        {
        }
        //Copy constructor and copy assignment
        thread_guard(const thread_guard& other_thread) = delete;
        thread_guard& operator=(const thread_guard& other_thread) = delete;
        //Move constructor and move assignment
        thread_guard(thread_guard&& other) noexcept
        : m_t{std::move(other.m_t)}
        {}
        thread_guard& operator=(thread_guard&& other)
        {
            if(this == &other)
                return *this;
            if(this->joinable())
                m_t.join();
            m_t = std::move(other.m_t);
            return *this;
        }
        //Destructor
        ~thread_guard()
        {
            if(this->joinable())
                m_t.join();
        }
        //Utility functions
        std::thread& get_thread() noexcept
        {
            return m_t;
        }
        const std::thread& get_thread() const noexcept
        {
            return m_t;
        }
        std::thread::id get_id() const noexcept
        {
            return m_t.get_id();
        }
        void swap(thread_guard& other) noexcept
        {
            std::swap(this->m_t, other.m_t);
        }
        bool joinable() const noexcept
        {
            return m_t.joinable();
        }
        void join()
        {
            m_t.join();
        }
        void detach()
        {
            m_t.detach();
        }
    private:
        std::thread m_t;
    };
}