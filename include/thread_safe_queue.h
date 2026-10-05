#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <concepts>

namespace custom
{
    template <typename T>
    concept movable = std::movable<T>;

    template<movable T, typename Queue = std::queue<T>>
    class ts_queue
    {
    public:
        using value_type = typename Queue::value_type;
        using reference = value_type&;
        using const_reference = const value_type&;
        using size_type = typename Queue::size_type;
        using pointer = value_type*;
        using const_pointer = const value_type*;

        ts_queue() = default;
        ts_queue( const ts_queue& other) = delete;
        ts_queue( ts_queue&& other) = delete;
        ts_queue& operator= (const ts_queue& other) = delete;
        ts_queue& operator= (ts_queue&& other) = delete;
        ~ts_queue() = default;
        template<typename... Args>
        void emplace(Args&&... args)
        {
            std::lock_guard<std::mutex> lk{m_mut};
            m_queue.emplace(std::forward<Args>(args)...);
            m_cv.notify_one();
        }
        template<typename... Args>
        void push(Args... value)
        { 
            std::lock_guard<std::mutex> lk{m_mut};
            (m_queue.push(std::move(value)),...);
            m_cv.notify_all();
        }
        bool try_pop(value_type& pipe_out)
        {
            std::lock_guard<std::mutex> lk{m_mut};
            if(m_queue.empty())
            {
                return false;
            }
            pipe_out = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }
        void wait_pop(value_type& pipe_out)
        {
            std::unique_lock<std::mutex> lk{m_mut};
            m_cv.wait(lk,[this](){return !(this->m_queue.empty());});
            pipe_out = std::move(m_queue.front());
            m_queue.pop();
        }
    private:
        std::condition_variable m_cv;
        std::mutex m_mut;
        Queue m_queue;
    };
}