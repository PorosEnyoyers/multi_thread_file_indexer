#pragma once

#include <optional>
#include <utility>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <concepts>

namespace custom
{
    template<typename T>
    struct node
    {
        std::optional<T> m_value;
        std::unique_ptr<node> m_next;

        node() : m_value {std::nullopt}, m_next{nullptr}{}
        node& operator=(const T& a) = delete;
        node& operator=(T&& a) = delete;
        ~node() = default;
        void initialized_value(T value)
        {
            m_value = std::move(value);
        }
        void set_next(std::unique_ptr<node<T>> ptr)
        {
            m_next = std::move(ptr);
        }
    };

    template <std::movable T>
    class two_lock_queue
    {
    public:
        using value_type = T;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using size_type = std::size_t;
        using node = node<value_type>;
        using queue = two_lock_queue<T>;
        void push(T value)
        {
            std::unique_ptr<node> new_node = std::make_unique<node>();
            std::lock_guard<std::mutex> lk{m_tail_lk};
            m_tail_ptr->initialized_value(std::move(value));
            m_tail_ptr->set_next(std::move(new_node));
            m_tail_ptr = m_tail_ptr->m_next.get();
            ++m_size;
        }
        two_lock_queue()
        {
            m_head_ptr = std::make_unique<node>();
            m_tail_ptr = m_head_ptr.get();
            m_size = 0;
        }
        two_lock_queue(const queue& other) = delete;
        two_lock_queue(queue&& other) = delete;
        two_lock_queue& operator=(const queue& other) = delete;
        two_lock_queue& operator=(queue&& other) = delete;
        ~two_lock_queue() = default;

    private:
        std::mutex m_head_lk;
        std::mutex m_tail_lk;
        std::condition_variable m_cv;
        std::unique_ptr<node> m_head_ptr;
        node* m_tail_ptr;
        size_type m_size;
    };
}