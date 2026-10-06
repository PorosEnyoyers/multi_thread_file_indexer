#pragma once
#include <queue>
#include <utility>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <bitset>
#include <bit>

namespace custom
{
	template <typename T, typename Queue = std::queue<T>, std::size_t max_prio = 10>
		requires (max_prio < 64)
	class Prio_Queue
	{
	public:
		using value_type = typename Queue::value_type;
		using reference = value_type&;
		using const_reference = const value_type&;
		using size_type = typename Queue::size_type;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		static constexpr size_type default_prio = max_prio / 2;

		//Default constructor
		Prio_Queue() : m_size{ 0 }, m_array(max_prio, Queue()), m_flag{ }
		{
		}
		~Prio_Queue() = default;
		Prio_Queue(Prio_Queue&& prio_queue) noexcept
			:m_array(std::move(prio_queue.m_array)), m_size(prio_queue.m_size), m_flag(prio_queue.m_flag)
		{
			prio_queue.m_size = 0;
			prio_queue.m_flag.reset();
		}
		Prio_Queue(const Prio_Queue& prio_queue)
			:m_array(prio_queue.m_array), m_size(prio_queue.m_size), m_flag(prio_queue.m_flag)
		{
		}
		[[nodiscard]] Prio_Queue& operator=(Prio_Queue&& prio_queue) & noexcept
		{
			m_array = std::move(prio_queue.m_array);
			m_size = prio_queue.m_size;
			m_flag = prio_queue.m_flag;
			prio_queue.m_size = 0;
			prio_queue.m_flag.reset();
			return *this;
		}
		[[nodiscard]] Prio_Queue& operator=(const Prio_Queue& prio_queue) & 
		{
			m_array = prio_queue.m_array;
			m_size = prio_queue.m_size;
			m_flag = prio_queue.m_flag;
			return *this;
		}
		[[nodiscard]] size_type get_size() const
		{
			return m_size;
		}
		constexpr bool is_empty() const
		{
			return m_size == 0;
		}
		[[nodiscard]] constexpr reference front()&
		{
			if (this->is_empty())
			{
				throw std::logic_error("All queues are empty!!! Can't call front()!!!");
			}
			return m_array[get_highest_prio_non_empty()].front();
		}
		[[nodiscard]] constexpr reference last()&
		{
			if (this->is_empty())
			{
				throw std::logic_error("All queues are empty!!! Can't call last()!!!");
			}
			return m_array[get_lowest_prio_non_empty()].back();
		}
		[[nodiscard]] constexpr const_reference front() const &
		{
			if (this->is_empty())
			{
				throw std::logic_error("All queues are empty!!! Can't call front()!!!");
			}
			return m_array[get_highest_prio_non_empty()].front();
		}
		[[nodiscard]] constexpr const_reference last() const &
		{
			if (this->is_empty())
			{
				throw std::logic_error("All queues are empty!!! Can't call last()!!!");
			}
			return m_array[get_lowest_prio_non_empty()].back();
		}
		[[nodiscard]] size_type size_of_queue(size_type prio) const
		{
			return m_array[prio].size();
		}
		[[nodiscard]] bool is_queue_empty(size_type prio) const
		{
			return (m_flag.test(prio) ? false : true);
		}
		void enqueue(value_type data)
		{
			enqueue(std::move(data), default_prio);
		}
		void enqueue(value_type data, size_type prio)
		{
			m_array[prio].push(std::move(data));
			++m_size;
			m_flag.set(prio);
		}
		[[nodiscard]] value_type dequeue()&
		{
			return dequeue(this->get_highest_prio_non_empty());
		}
		[[nodiscard]] value_type dequeue(size_type prio)&
		{
			if (m_size == 0)
			{
				return -69420;
			}
			if (!m_flag.test(prio))
			{
				throw std::logic_error("The requested queue to dequeue is empty, can't call dequeu!!!");
			}
			value_type temp = std::move(m_array[prio].front());
			m_array[prio].pop();
			--m_size;
			if (m_array[prio].empty())
			{
				m_flag.reset(prio);
			}
			return temp;
		}
	private:
		std::vector<Queue> m_array;
		size_type m_size;
		std::bitset<max_prio> m_flag;

		/**
		We cast the m_flag to unsigned long integer since std::countr_zero and std::bit_width only accept integers.
		Index 0(least significant bit) is the highest prio while index 9(most significant bit) is the lowest prio
		**/
		size_type get_highest_prio_non_empty() const
		{
			return static_cast<size_type>(std::countr_zero(m_flag.to_ullong()));
		}
		size_type get_lowest_prio_non_empty() const
		{
			return static_cast<size_type>(std::bit_width(m_flag.to_ullong())) - 1;
		}
	};
}