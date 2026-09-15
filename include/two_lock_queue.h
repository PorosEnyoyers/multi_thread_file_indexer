#pragma once

#include <optional>
#include <utility>
#include <mutex>
#include <atomic>
#include <concepts>
#include <cstddef>

namespace custom
{
    // Michael & Scott two-lock queue.
    //
    // The head and the tail are protected by *separate* mutexes, so one producer
    // and one consumer can make progress at the same time (higher throughput
    // under contention than a single-mutex queue).
    //
    // The one field that a producer (push) and a consumer (try_pop) can touch
    // concurrently is the `next` pointer of the boundary node. It is therefore a
    // std::atomic<node*> with release/acquire ordering, so publishing a new node
    // (push) happens-before it becomes visible to a popper. This is what makes the
    // queue data-race free under ThreadSanitizer.
    //
    // This queue is intentionally non-blocking (try_pop only). Correct blocking
    // consumption on a two-lock queue is subtle (the notifier cannot cheaply hold
    // the consumer's mutex), so the blocking work queue is ts_queue instead.
    template <std::movable T>
    class two_lock_queue
    {
    public:
        using value_type = T;
        using size_type = std::size_t;

        two_lock_queue()
        {
            // Start with a single dummy node shared by head and tail.
            m_head = m_tail = new node();
        }
        two_lock_queue(const two_lock_queue&) = delete;
        two_lock_queue(two_lock_queue&&) = delete;
        two_lock_queue& operator=(const two_lock_queue&) = delete;
        two_lock_queue& operator=(two_lock_queue&&) = delete;
        ~two_lock_queue()
        {
            node* n = m_head;
            while (n != nullptr)
            {
                node* next = n->m_next.load(std::memory_order_relaxed);
                delete n;
                n = next;
            }
        }

        void push(T value)
        {
            node* new_node = new node(std::move(value));
            std::lock_guard<std::mutex> lk{m_tail_lk};
            // Publish the fully-constructed node with a release store.
            m_tail->m_next.store(new_node, std::memory_order_release);
            m_tail = new_node;
            m_size.fetch_add(1, std::memory_order_relaxed);
        }

        // Non-blocking. Returns false when the queue is empty.
        bool try_pop(value_type& pipe_out)
        {
            std::lock_guard<std::mutex> lk{m_head_lk};
            node* new_head = m_head->m_next.load(std::memory_order_acquire);
            if (new_head == nullptr)
            {
                return false; // only the dummy node is present
            }
            pipe_out = std::move(new_head->m_value.value());
            delete m_head;         // free the old dummy
            m_head = new_head;     // its value has been moved out; it is the new dummy
            m_size.fetch_sub(1, std::memory_order_relaxed);
            return true;
        }

        size_type size() const
        {
            return m_size.load(std::memory_order_relaxed);
        }

    private:
        struct node
        {
            std::optional<T> m_value;
            std::atomic<node*> m_next{nullptr};

            node() = default;
            explicit node(T value) : m_value{std::move(value)} {}
        };

        std::mutex m_head_lk;                 // guards m_head
        std::mutex m_tail_lk;                 // guards m_tail
        node* m_head;                         // dummy node at the front
        node* m_tail;                         // last node
        std::atomic<size_type> m_size{0};
    };
}
