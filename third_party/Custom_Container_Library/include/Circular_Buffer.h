#pragma once

#include <utility>
#include <initializer_list>
#include <array>
#include <vector>
#include <cassert>
#include <exception>
#include <iostream>
#include <concepts>
namespace custom
{
	template <typename Buffer>
	class circular_buffer_iterator;

	template <typename T, std::size_t N>
		requires(N > 0)
	class circular_buffer
	{
	public:
		//Declare alias
		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;
		using iterator = circular_buffer_iterator <circular_buffer<T, N>>;
		using const_iterator = circular_buffer_iterator<const circular_buffer<T, N>>;
		//Constructor
		constexpr circular_buffer() = default;
		template <typename Iter>
		constexpr circular_buffer(Iter begin, Iter end, size_type size) : m_size{ size }
		{
			if (size == 0)
			{
				return;
			}
			assert(size <= N && "Container size is too small!!! Loss of datas prevented!!!");
			std::copy(begin, end, m_data.begin());
			m_tail = size - 1;
		}
		constexpr circular_buffer(const std::vector<T>& vect) : circular_buffer(vect.begin(), vect.end(), vect.size()) {}
		template <size_type S>
			requires(S <= N)
		constexpr circular_buffer(const std::array<T, S>& arr) : circular_buffer(arr.begin(), arr.end(), S) {}
		constexpr circular_buffer(const std::initializer_list<T>& list) : circular_buffer(list.begin(), list.end(), list.size()) {}
		constexpr circular_buffer(const_reference value) : m_size{ N }, m_tail{ N - 1 }
		{
			std::fill(m_data.begin(), m_data.end(), value);
		}
		~circular_buffer() = default;
		
		//Functions for metadata
		constexpr size_type size() const noexcept
		{
			return m_size;
		}
		constexpr size_type capacity() const noexcept
		{
			return N;
		}
		constexpr bool empty() const noexcept
		{
			return m_size == 0;
		}
		constexpr bool full() const noexcept
		{
			return m_size == N;
		}
		constexpr void clear() noexcept
		{
			m_size = 0; m_head = 0; m_tail = 0;
		}
		//Access members in the containers
		constexpr reference operator[](size_type const pos)
		{
			return m_data[(m_head + pos) % N];
		}
		constexpr const_reference operator[] (size_type const pos) const
		{
			return m_data[(m_head + pos) % N];
		}
		constexpr reference at(size_type const pos)
		{
			if (pos < m_size)
				return m_data[(m_head + pos) % N];
			throw std::out_of_range("Index is out of range");
		}
		constexpr const_reference at(size_type const pos) const
		{
			if (pos < m_size)
				return m_data[(m_head + pos) % N];

			throw std::out_of_range("Index is out of range");
		}
		constexpr reference front()
		{
			if (m_size > 0) return m_data[m_head];
			throw std::logic_error("Buffer is empty");
		}
		constexpr const_reference front() const
		{
			if (m_size > 0) return m_data[m_head];
			throw std::logic_error("Buffer is empty");
		}
		constexpr reference back()
		{
			if (m_size > 0) return m_data[m_tail];
			throw std::logic_error("Buffer is empty");
		}
		constexpr const_reference back() const
		{
			if (m_size > 0) return m_data[m_tail];
			throw std::logic_error("Buffer is empty");
		}
		//Inserting elements
		constexpr void push_back(value_type value)&
		{
			if (!(this->empty()))
			{
				m_tail = (m_tail + 1) % N;
			}
			m_data[m_tail] = std::move(value);
			if (m_size < N)
			{
				++m_size;
				return;
			}
			m_head = (m_head + 1) % N;
		}
		//Pop the oldest data
		constexpr value_type pop_front()&
		{
			if (empty()) throw std::logic_error("Buffer is empty");

			size_type index = m_head;
			m_head = (m_head + 1) % N;
			--m_size;
			if (m_size == 0)
			{
				m_tail = m_head;
			}
			return m_data[index];
		}
		//Iterator access and creation
		iterator begin()
		{
			return iterator(this, 0);
		}
		iterator end()
		{
			return iterator(this, m_size);
		}
		const_iterator begin() const
		{
			return const_iterator(this, 0);
		}
		const_iterator end() const
		{
			return const_iterator(this, m_size);
		}
		//Printing out the container
		friend std::ostream& operator<<(std::ostream& out, const circular_buffer<T, N>& arr)
			requires requires(std::ostream& os, const T& value) { os << value; }
		{
			if (arr.size() == 0)
			{
				out << "Container is empty!!!";
				return out;
			}
			for (auto i{ arr.begin() }; i != arr.end(); ++i)
			{
				out << *i << " ";
			}
			return out;
		}
	private:
		std::array<value_type, N> m_data{};
		size_type m_head{ 0 };
		size_type m_tail{ 0 };
		size_type m_size{ 0 };
		friend circular_buffer_iterator<circular_buffer<T, N>>;
		friend circular_buffer_iterator<const circular_buffer<T, N>>;
	};

	template <typename Buffer>
	class circular_buffer_iterator
	{
	public:
		using self_type = circular_buffer_iterator<Buffer>;
		using container_type = std::remove_const_t<Buffer>;
		using value_type = typename container_type::value_type;
		using reference = std::conditional_t<std::is_const_v<Buffer>, const value_type&, value_type&>;
		using pointer = std::conditional_t<std::is_const_v<Buffer>, const value_type*, value_type*>;
		using iterator_category = std::random_access_iterator_tag;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;

		//constructor
		circular_buffer_iterator(Buffer* buffer, size_type const index) : m_buffer{ buffer }, m_index{ index } {}

		//helper functions
		bool compatible(self_type const& other) const
		{
			return m_buffer->m_data.data() == other.m_buffer->m_data.data();
		}
		bool in_bounds() const
		{
			return m_buffer && m_index < m_buffer->size();
		}
		//Random-access functionality
		self_type& operator++()
		{
			if (m_index >= m_buffer->size())
				throw std::out_of_range("Iterator connot be incremented past the end of range!!!");
			++m_index;
			return *this;
		}
		self_type operator++(int)
		{
			self_type temp = *this;
			++*this;
			return temp;
		}
		self_type& operator--()
		{
			if (m_index <= 0)
				throw std::out_of_range("Iterator cannot be decrement before the beginning of the range!!!");
			--m_index;
			return *this;
		}
		self_type operator--(int)
		{
			self_type temp = *this;
			--*this;
			return temp;
		}
		self_type& operator+=(difference_type const offset)
		{
			difference_type next = static_cast<difference_type>(m_index) + offset;

			if (next < 0 || next > m_buffer->size())
				throw std::out_of_range("Iterator are moving pass the allowed range!!!");

			m_index = static_cast<size_type>(next);
			return *this;
		}
		self_type& operator-=(difference_type const offset)
		{
			return *this += -offset;
		}
		difference_type operator-(self_type const& other) const
		{
			return m_index - other.m_index;
		}
		self_type operator+(difference_type offset) const
		{
			self_type temp = *this;
			return temp += offset;
		}
		self_type operator-(difference_type offset) const
		{
			self_type temp = *this;
			return temp -= offset;
		}

		//comparison functionality
		bool operator==(self_type const& other) const
		{
			return this->compatible(other) && m_index == other.m_index;
		}
		bool operator!=(self_type const& other) const
		{
			return !(*this == other);
		}
		bool operator<(self_type const& other) const
		{
			return m_index < other.m_index;
		}
		bool operator>(self_type const& other) const
		{
			return other < *this;
		}
		bool operator<=(self_type const& other) const
		{
			return !(other < *this);
		}
		bool operator>=(self_type const& other) const
		{
			return !(*this < other);
		}
		//Access and dereference functionality
		reference operator*() const
		{
			if (m_buffer->empty() || !(this->in_bounds()))
				throw std::logic_error("Cannot dereferentiate the iterator!!!");
			return (*m_buffer)[m_index];
		}
		pointer operator->() const
		{
			return &operator*();
		}
		value_type& operator[](difference_type const offset)
		{
			return *((*this + offset));
		}
		value_type const& operator[](difference_type const offset) const
		{
			return *((*this + offset));
		}
	private:
		Buffer* m_buffer;
		size_type m_index{ 0 };
	}; 
}