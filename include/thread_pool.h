#pragma once
#include <concepts>
#include <future>
#include "./thread_guard.h"
#include "./two_lock_queue.h"
#include <type_traits>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <tuple>
namespace custom
{
    class Task
    {
        struct task_base
        {
            virtual void execute() = 0;
            virtual ~task_base() = default;
        };
        template<typename Func, typename... Args>
        requires Function<Func, Args...>
        struct task_function : task_base
        {
            Func m_func;
            std::tuple<Args...> m_args;
            task_function(Func&& func, Args&&... args)
            : m_func{std::forward<Func>(func)}, m_args{std::forward<Args>(args)...}
            {}
            void execute() override
            {
                std::apply(m_func, std::move(m_args));
            }
        };
        template<typename Func_ptr, typename Obj_ptr, typename... Args>
        requires Object_Func<Func_ptr, Obj_ptr, Args...>
        struct task_object_function : task_base
        {
            Func_ptr m_func;
            std::tuple<Obj_ptr, Args...> m_args;
            task_object_function(Func_ptr func, Obj_ptr obj, Args&&... args)
            : m_func{func}, m_args{obj, std::forward<Args>(args)...}
            {}
            void execute() override
            {
                std::apply(m_func, std::move(m_args));
            }
        };
        template<std::invocable F>
        struct task_functor : task_base
        {
            F m_functor;
            task_functor(F&& functor)
            : m_functor{std::forward<F>(functor)}
            {}
            void execute() override
            {
                m_functor();
            }
        };

    public:
        Task() = default;
        //Overengineered due to the Task should only take std::packaged_task
        template<typename Func, typename... Args>
        requires Function<Func, Args...>
        Task(Func&& func, Args&&... args)
        : m_task(new task_function<Func, Args...>(std::forward<Func>(func), std::forward<Args>(args)...))
        {}
        //Overengineered due to the Task should only take std::packaged_task
        template<typename Func_ptr, typename Obj_ptr, typename... Args>
        requires Object_Func<Func_ptr, Obj_ptr, Args...>
        Task(Func_ptr func, Obj_ptr obj, Args&&... args)
        : m_task(new task_object_function<Func_ptr,Obj_ptr,Args...>(func,obj,std::forward<Args>(args)...))
        {}

        template<std::invocable F>
        requires (!std::same_as<std::remove_cvref_t<F>, Task>) //This is due to imbiguous matching with move constructor since Task satisfy std::invocable constraints.
        Task(F&& functor)
        : m_task(new task_functor<F>(std::forward<F>(functor)))
        {}

        void operator()()
        {
            m_task->execute();
        }

        Task(Task&& other)
        : m_task(std::move(other.m_task))
        {}
        Task& operator=(Task&& other)
        {
            m_task = std::move(other.m_task);
            return *this;
        }
        Task(const Task& other) = delete;
        Task& operator=(const Task& other) = delete;
        ~Task() = default;
    private:
        std::unique_ptr<task_base> m_task;
    };
    class thread_pool
    {
    public:
        thread_pool()
        : m_done{false}
        {
            unsigned  thread_count = std::thread::hardware_concurrency();
            if(thread_count == 0)
            {
                thread_count = 1;
            }
            m_threads.reserve(thread_count);
            try
            {
                for(unsigned i = 0; i < thread_count; ++i)
                {
                    m_threads.push_back(thread_guard{std::thread(&thread_pool::worker_thread, this)});
                }
            }
            catch(...)
            {
                m_done = true;
                 m_task_queue.notify_all();
                throw std::logic_error("Failed to create thread!!!");
            }
        }
        thread_pool(const thread_pool& other) = delete;
        thread_pool& operator=(const thread_pool& other) = delete;
        thread_pool(thread_pool&& other) = delete;
        thread_pool& operator=(thread_pool&& other) = delete;
        ~thread_pool()
        {
            m_done = true;
            m_task_queue.notify_all();
        }
        template<typename Invoke, typename... Args>
        requires std::invocable<Invoke, Args...>
        [[nodiscard]] auto submit(Invoke&& invoke, Args&&... args) & -> std::future<std::invoke_result_t<Invoke, Args...>>
        {
            using Result_Type = typename std::invoke_result_t<Invoke, Args...>;
            //Capture functions and argument into lambda
            auto functor = [invoke = std::forward<Invoke>(invoke), args = std::make_tuple(std::forward<Args>(args)...)] () mutable {
               return std::apply(invoke, std::move(args));
            };
            std::packaged_task<Result_Type()> task(std::move(functor));
            std::future<Result_Type> result = task.get_future();
            m_task_queue.push(Task(std::move(task)));
            return result;
        }
        void shutdown()&
        {
            m_done = true;
            m_task_queue.notify_all();
        }
    private:
        std::atomic_bool m_done;
        custom::two_lock_queue<Task> m_task_queue;
        std::vector<custom::thread_guard> m_threads;
        void worker_thread() &
        {
            while(true)
            {
                Task task;
                if(!m_task_queue.wait_pop_or_stop(task, m_done))
                    return;
                 task();
            }   
        }
    };
}   