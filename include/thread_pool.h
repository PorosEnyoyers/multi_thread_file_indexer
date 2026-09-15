#pragma once

#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

#include "./thread_guard.h"
#include "./thread_safe_queue.h"

namespace custom
{
    // Fixed-size pool of worker threads pulling tasks from a shared queue.
    // submit() returns a std::future so callers can wait for results and
    // propagate exceptions. Workers are owned by thread_guard, so they are
    // joined automatically (RAII) when the pool is destroyed.
    class thread_pool
    {
    public:
        explicit thread_pool(std::size_t thread_count = std::thread::hardware_concurrency())
        {
            if (thread_count == 0)
            {
                thread_count = 1;
            }
            m_workers.reserve(thread_count);
            for (std::size_t i = 0; i < thread_count; ++i)
            {
                m_workers.emplace_back([this] { worker_loop(); });
            }
        }

        thread_pool(const thread_pool&) = delete;
        thread_pool(thread_pool&&) = delete;
        thread_pool& operator=(const thread_pool&) = delete;
        thread_pool& operator=(thread_pool&&) = delete;

        ~thread_pool()
        {
            // Tell workers to drain remaining tasks and exit. m_workers is declared
            // after m_tasks, so it is destroyed first: each thread_guard joins its
            // worker, which has already left its loop thanks to shutdown().
            m_tasks.shutdown();
        }

        // Enqueue a callable; returns a future for its result.
        template<typename Func, typename... Args>
        auto submit(Func&& func, Args&&... args)
            -> std::future<std::invoke_result_t<Func, Args...>>
        {
            using result_t = std::invoke_result_t<Func, Args...>;
            auto task = std::make_shared<std::packaged_task<result_t()>>(
                std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
            std::future<result_t> result = task->get_future();
            m_tasks.push([task] { (*task)(); });
            return result;
        }

        std::size_t size() const noexcept { return m_workers.size(); }

    private:
        void worker_loop()
        {
            std::function<void()> task;
            while (m_tasks.wait_pop(task))
            {
                task();
            }
        }

        ts_queue<std::function<void()>> m_tasks; // declared first -> destroyed last
        std::vector<thread_guard> m_workers;     // destroyed first -> joined after shutdown()
    };
}
