#pragma once
#include <vector>
#include <array>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <exception>
#include <utility>
#include <algorithm>
#include <variant>
#include <concepts>
#include <memory>
#include <type_traits>
#include <stdexcept>
namespace custom
{
	template<typename T>
	concept Heapable =std::movable<T> && std::destructible<T> && requires(const T& a, const T& b) {
		{ a < b } -> std::convertible_to<bool>;
		{ a == b } -> std::convertible_to<bool>;
	};

	template<Heapable T>
	class heap_element_wrapper
	{
	public:
		static constexpr bool is_direct = sizeof(T) <= 64 || (sizeof(T) <= 128 && std::is_trivially_copyable_v<T>);
		using wrapped_type = std::conditional_t<is_direct, T, std::unique_ptr<T>>;
		//constructors
		heap_element_wrapper() = delete;
		heap_element_wrapper(const T& value)
			: m_data{copy_data(value)} {}
		heap_element_wrapper(T&& value)
			: m_data{move_data(std::move(value))} {}
		~heap_element_wrapper() = default;
		heap_element_wrapper(heap_element_wrapper&& other) noexcept = default;
		heap_element_wrapper& operator=(heap_element_wrapper&& other) noexcept = default;
		//custom copy constructor and copy assigment for unique pointer
		heap_element_wrapper(const heap_element_wrapper& other)
			: m_data{copy_wrapped(other)}
		{
		}
		heap_element_wrapper& operator=(const heap_element_wrapper& other)
		{
			if (this == &other)
				return *this;
			if constexpr (is_direct)
			{
				m_data = other.m_data;
			}
			else
			{
				this->m_data = std::make_unique<T>(*(other.m_data));
			}
			return *this;
		}
		//access the wrapped data
		T& value()
		{
			if constexpr (is_direct)
				return m_data;
			else
				return *m_data;
		}
		const T& value() const
		{
			if constexpr (is_direct)
				return m_data;
			else
				return *m_data;
		}
		//overloaded comparison
		friend bool operator<(const heap_element_wrapper& a, const heap_element_wrapper& b)
		{
			return a.value() < b.value();
		}
		friend bool operator>(const heap_element_wrapper& a, const heap_element_wrapper& b)
		{
			return b < a;
		}
		friend bool operator<=(const heap_element_wrapper& a, const heap_element_wrapper& b)
		{
			return !(b < a);
		}
		friend bool operator>=(const heap_element_wrapper& a, const heap_element_wrapper& b)
		{
			return !(a < b);
		}
		friend bool operator==(const heap_element_wrapper& a, const heap_element_wrapper& b)
		{
			return a.value() == b.value();
		}
		friend bool operator!=(const heap_element_wrapper& a, const heap_element_wrapper& b)
		{
			return !(a == b);
		}
	private:
		
		static wrapped_type copy_data(const T& value)
		{
			if constexpr (is_direct)
				return value;
			else
				return std::make_unique<T>(value);
		}
		static wrapped_type move_data(T&& value)
		{
			if constexpr (is_direct)
				return std::move(value);
			else
				return std::make_unique<T>(std::move(value));
		}
		static wrapped_type copy_wrapped(const heap_element_wrapper& other)
		{
			if constexpr (is_direct)
				return other.m_data;
			else
				return std::make_unique<T>(*other.m_data);
		}

		wrapped_type m_data;
	};

	enum class heap_type
	{
		MAX,
		MIN,
	};

	template<Heapable T, heap_type TYPE = heap_type::MAX, std::size_t stack_cap = 1000>
	class binary_heap
	{
	public:
		//Declare alias
		using value_type = T;
		using size_type = std::size_t;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;
		using Element = heap_element_wrapper<value_type>;
		using Array_Storage = std::array<std::optional<Element>, stack_cap>;
		using Vector_Storage = std::vector<Element>;
		//Constructor
		binary_heap() : m_storage{ Array_Storage{} }, m_size{ 0 }, m_is_array{ 1 } {}
		explicit binary_heap(T value) : m_storage{ Array_Storage{} }, m_size{ 0 }, m_is_array{ 1 }
		{
			this->push(std::move(value));
		}
		template<typename Iter>
		binary_heap(Iter begin, Iter end, size_type size) : m_size{ size }
		{
			if (size <= stack_cap)
			{
				m_storage = Array_Storage{};
				auto& arr = std::get<Array_Storage>(m_storage);
				m_is_array = true;
				for (size_type i{ 0 }; begin != end; ++begin, ++i)
				{
					arr[i] = Element(*begin);
				}
				heapify_storage();
			}
			else
			{
				m_storage = Vector_Storage{};
				auto& vec = std::get<Vector_Storage>(m_storage);
				vec.reserve(size);
				m_is_array = false;
				for (auto it = begin; it != end; ++it)
				{
					vec.emplace_back(*it);
				}
				heapify_storage();
			}
		}
		binary_heap(const std::vector<T>& vec) : binary_heap(vec.begin(), vec.end(), vec.size()) {}
		template<size_type S>
		binary_heap(const std::array<T,S>& arr) : binary_heap(arr.begin(), arr.end(), S) {}
		binary_heap(const std::initializer_list<T>& list) : binary_heap(list.begin(), list.end(), list.size()) {}
		~binary_heap() = default;
		//Push function
		void push(T value)
		{
			if (m_is_array && m_size == stack_cap)
			{
				promote_to_vector();
				m_is_array = false;
			}
			if (m_is_array)
			{
				auto& arr = std::get<Array_Storage>(m_storage);
				arr[m_size].emplace(std::move(value));
			}
			else
			{
				auto& vec = std::get<Vector_Storage>(m_storage);
				vec.emplace_back(std::move(value));
			}
			heapify_up(m_size);
			++m_size;
		}
		//Get value function
		value_type pop_root()
		{
			if (m_size == 0) throw std::out_of_range("Heap is empty");
			if (m_is_array)
			{
				auto& arr = std::get<Array_Storage>(m_storage);
				std::swap(arr[0], arr[m_size - 1]);
				--m_size;
				if (m_size > 0) heapify_down(0);
				value_type pop_val = std::move(arr[m_size].value().value());
				arr[m_size].reset();
				return pop_val;
			}
			else
			{
				auto& vec = std::get<Vector_Storage>(m_storage);
				std::swap(vec[0], vec[m_size - 1]);
				--m_size;
				if (m_size > 0) heapify_down(0);
				value_type pop_val = std::move(vec[m_size].value());
				vec.pop_back();
				return pop_val;
			}
		}
		const_reference top() const
		{
			if (m_size == 0) throw std::out_of_range("Heap is empty!!!");
			if (m_is_array) return std::get<Array_Storage>(m_storage)[0].value().value();
			else return std::get<Vector_Storage>(m_storage)[0].value();
		}
	private:
		void promote_to_vector()
		{
			auto& arr = std::get<Array_Storage>(m_storage);
			Vector_Storage v;
			v.reserve(stack_cap * 2);
			for (auto& i : arr)
			{
				v.push_back(std::move(*i));
			}
			m_storage = std::move(v);
			std::cout << "Noti: Storage is now moving to vector!!!";
		}
		bool compare_element(const Element& a, const Element& b) const
		{
			if constexpr (TYPE == heap_type::MAX)
			{
				return a > b;
			}
			else
			{
				return a < b;
			}
		}
		void heapify_up(size_type index)
		{
			if (m_is_array)
			{
				auto& arr = std::get<Array_Storage>(m_storage);
				auto value = std::move(arr[index]);
				while (index > 0 && compare_element(*value, *arr[(index - 1) / 2]))
				{
					arr[index] = std::move(arr[(index - 1) / 2]);
					index = (index - 1) / 2;
				}
				arr[index] = std::move(value);
			}
			else
			{
				auto& vec = std::get<Vector_Storage>(m_storage);
				auto value = std::move(vec[index]);
				while (index > 0 && compare_element(value, vec[(index - 1) / 2]))
				{
					vec[index] = std::move(vec[(index - 1) / 2]);
					index = (index - 1) / 2;
				}
				vec[index] = std::move(value);
			}
		}
		void heapify_down(size_type index)
		{
			if (m_is_array)
			{
				auto& arr = std::get<Array_Storage>(m_storage);
				auto value = std::move(arr[index]);
				while (true)
				{
					size_type l_child{ index * 2 + 1 };
					size_type r_child{ index * 2 + 2 };
					if (l_child >= m_size)
					{
						break;
					}
					size_type select_child = (r_child < m_size && compare_element(*arr[r_child], *arr[l_child]) ? r_child : l_child);
					if (compare_element(*arr[select_child], *value))
					{
						arr[index] = std::move(arr[select_child]);
						index = select_child;
					}
					else
					{
						break;
					}
				}
				arr[index] = std::move(value);
			}
			else
			{
				auto& vec = std::get<Vector_Storage>(m_storage);
				auto value = std::move(vec[index]);
				while (true)
				{
					size_type l_child{ index * 2 + 1 };
					size_type r_child{ index * 2 + 2 };
					if (l_child >= m_size)
					{
						break;
					}
					size_type select_child = (r_child < m_size && compare_element(vec[r_child], vec[l_child]) ? r_child : l_child);
					if (compare_element(vec[select_child], value))
					{
						vec[index] = std::move(vec[select_child]);
						index = select_child;
					}
					else
					{
						break;
					}
				}
				vec[index] = std::move(value);
			}
		}
		void heapify_storage()
		{
			if (m_size <= 1)
				return;
			for (size_type i = (m_size - 2) / 2 + 1; i > 0; --i)
			{
				heapify_down(i - 1);
			}
		}
		std::variant<Array_Storage, Vector_Storage> m_storage;
		size_type m_size;
		bool m_is_array;
	};
}