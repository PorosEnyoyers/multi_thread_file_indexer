#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <concepts>
#include <cstddef>

namespace custom
{
    template <typename T>
    concept movable = std::movable<T>;

    // Single-mutex, thread-safe queue used as the work queue of the thread pool.
    // Supports a graceful shutdown so blocked consumers can exit instead of
    // waiting forever (this is what lets the pool join its workers cleanly).
    template<movable T, typename Queue = std::queue<T>>
    class ts_queue
    {
    public:
        using value_type = typename Queue::value_type;

        ts_queue() = default;
        ts_queue(const ts_queue&) = delete;
        ts_queue(ts_queue&&) = delete;
        ts_queue& operator=(const ts_queue&) = delete;
        ts_queue& operator=(ts_queue&&) = delete;
        ~ts_queue() = default;

        void push(T value)
        {
            {
                std::lock_guard<std::mutex> lk{m_mut};
                m_queue.push(std::move(value));
            }
            m_cv.notify_one();
        }

        template<typename... Args>
        void emplace(Args&&... args)
        {
            {
                std::lock_guard<std::mutex> lk{m_mut};
                m_queue.emplace(std::forward<Args>(args)...);
            }
            m_cv.notify_one();
        }

        // Non-blocking. Returns false when the queue is empty.
        bool try_pop(value_type& pipe_out)
        {
            std::lock_guard<std::mutex> lk{m_mut};
            if (m_queue.empty())
            {
                return false;
            }
            pipe_out = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }

        // Blocks until an item is available OR shutdown() is called.
        // Returns false only when woken by shutdown with an empty queue.
        // The predicate is modified under m_mut (both in push and shutdown),
        // so there is no lost-wakeup window.
        bool wait_pop(value_type& pipe_out)
        {
            std::unique_lock<std::mutex> lk{m_mut};
            m_cv.wait(lk, [this] { return !m_queue.empty() || m_done; });
            if (m_queue.empty())
            {
                return false; // woken by shutdown
            }
            pipe_out = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }

        // Wakes every waiter so they can observe m_done and exit.
        void shutdown()
        {
            {
                std::lock_guard<std::mutex> lk{m_mut};
                m_done = true;
            }
            m_cv.notify_all();
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lk{m_mut};
            return m_queue.empty();
        }

        std::size_t size() const
        {
            std::lock_guard<std::mutex> lk{m_mut};
            return m_queue.size();
        }

    private:
        mutable std::mutex m_mut;
        std::condition_variable m_cv;
        Queue m_queue;
        bool m_done = false;
    };
}
