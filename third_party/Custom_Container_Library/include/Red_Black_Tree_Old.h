#pragma once
#include <iostream>
#include <array>
#include <vector>
#include <initializer_list>
#include <utility>
#include <cstdint>
#include <concepts>

namespace custom
{
	struct Hook
	{
		Hook* h_left = nullptr;
		Hook* h_right = nullptr;

		bool is_red() const
		{
			return h_parent & GET_COLOR;
		}
		bool is_black() const
		{
			return !is_red();
		}
		Hook* get_parent_ptr()
		{
			return reinterpret_cast<Hook*>(h_parent & GET_PTR);
		}
		void set_parent_ptr(Hook* new_par)
		{
			uintptr_t color_tag = h_parent & GET_COLOR;
			h_parent = reinterpret_cast<uintptr_t>(new_par) | color_tag;
		}
		void tag_red()
		{
			h_parent = h_parent | GET_COLOR;
		}
		void tag_black()
		{
			h_parent = h_parent & GET_PTR;
		}
		bool is_left()
		{
			return (!this->is_root()) && this == this->get_parent_ptr()->h_left;
		}
		bool is_right()
		{
			return (!this->is_root()) && this == this->get_parent_ptr()->h_right;
		}
		bool is_sentinel()
		{
			return (this->get_parent_ptr()->get_parent_ptr() == this && this->is_red());
		}
		bool is_root()
		{
			return this->get_parent_ptr()->get_parent_ptr() == this && this->is_black();
		}
		void swap_position(Hook* other)
		{
			if (this == other->get_parent_ptr())//this node is other node parent
			{
				Hook* this_parent = this->get_parent_ptr();
				bool other_is_left(other->is_left());
				//deal with parent relationship
				if (this_parent->is_sentinel())//this is root an sentinel parent point to root
				{
					this_parent->set_parent_ptr(other);
				}
				else
				{
					if (this->is_left())
					{
						this_parent->h_left = other;
					}
					else
					{
						this_parent->h_right = other;
					}
				}
				std::swap(this->h_parent, other->h_parent); //cause the parent tag hold the node color this preserve the color of the node.
				this->set_parent_ptr(other);
				other->set_parent_ptr(this_parent);
				//deal with children relationship
				std::swap(this->h_left, other->h_left);
				std::swap(this->h_right, other->h_right);
				if (other_is_left)
				{
					other->h_left = this;
					if (other->h_right)
					{
						other->h_right->set_parent_ptr(other);
					}
				}
				else
				{
					other->h_right = this;
					if (other->h_left)
					{
						other->h_left->set_parent_ptr(other);
					}
				}
				if (this->h_left)
				{
					this->h_left->set_parent_ptr(this);
				}
				if (this->h_right)
				{
					this->h_right->set_parent_ptr(this);
				}
			}
			else if (this->get_parent_ptr() == other)//other node is this node parent
			{
				Hook* other_parent = other->get_parent_ptr();
				bool this_is_left = this->is_left();
				if (other_parent->is_sentinel())
				{
					other_parent->set_parent_ptr(this);
				}
				else
				{
					if (other->is_left())
					{
						other_parent->h_left = this;
					}
					else
					{
						other_parent->h_right = this;
					}
				}
				std::swap(this->h_parent, other->h_parent); //cause the parent tag hold the node color, this preserve the color of the node.
				other->set_parent_ptr(this);
				this->set_parent_ptr(other_parent);
				//deal with children relationship
				std::swap(this->h_left, other->h_left);
				std::swap(this->h_right, other->h_right);
				if (this_is_left)
				{
					this->h_left = other;
					if (this->h_right)
					{
						this->h_right->set_parent_ptr(this);
					}
				}
				else
				{
					this->h_right = other;
					if (this->h_left)
					{
						this->h_left->set_parent_ptr(this);
					}
				}
				if (other->h_left)
				{
					other->h_left->set_parent_ptr(other);
				}
				if (other->h_right)
				{
					other->h_right->set_parent_ptr(other);
				}
			}
			else//this and other is not have parent children relationship.
			{
				Hook* this_parent = this->get_parent_ptr();
				Hook* other_parent = other->get_parent_ptr();
				//deal with parent relationship
                if(this_parent == other_parent)
                {
                    std::swap(this_parent->h_left, this_parent->h_right);
                }
                else
                {
				    if (this_parent->is_sentinel())
				    {
					    this_parent->set_parent_ptr(other);
				    }
				    else if (this->is_left())
				    {
					    this_parent->h_left = other;
				    }
				    else
				    {
					    this_parent->h_right = other;
				    }
				    if (other_parent->is_sentinel())//if other is root
				    {
				    	other_parent->set_parent_ptr(this);
				    }
				    else if (other->is_left())
				    {
				    	other_parent->h_left = this;
				    }
				    else
				    {
				    	other_parent->h_right = this;
				    }
                }
				std::swap(this->h_parent, other->h_parent);
				//deal with children relationship
				std::swap(this->h_left, other->h_left);
				std::swap(this->h_right, other->h_right);
				if (other->h_left)
				{
					other->h_left->set_parent_ptr(other);
				}
				if (other->h_right)
				{
					other->h_right->set_parent_ptr(other);
				}
				if (this->h_left)
				{
					this->h_left->set_parent_ptr(this);
				}
				if (this->h_right)
				{
					this->h_right->set_parent_ptr(this);
				}
			}
		}
		Hook() : h_left{ nullptr }, h_right{ nullptr }, h_parent{ 0 }
		{
			this->tag_red();
		}
	private:
		uintptr_t h_parent = 0;
		static constexpr uintptr_t GET_COLOR = 0x1;
		static constexpr uintptr_t GET_PTR = ~0x1;
	};
	template<std::movable T>
	struct RB_Node
	{
		T n_data;
		Hook n_hook;

		RB_Node() = delete;

		RB_Node(T data, Hook* parent_ptr = nullptr) : n_data{ std::move(data) }, n_hook{}
		{
			n_hook.set_parent_ptr(parent_ptr);
			n_hook.tag_red();
		}
	};
	template<typename T>
    requires std::movable<T> && std::totally_ordered<T>
	class RB_Tree
	{
	public:
		using size_type = size_t;
		using value_type = T;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using reference = value_type&;
		using const_reference = const value_type&;

		struct Iterator
		{
			Hook* i_ptr;

			Iterator(Hook* ptr) : i_ptr{ ptr } {}
			bool operator==(const Iterator& other) const { return i_ptr == other.i_ptr; }
			bool operator!=(const Iterator& other) const { return i_ptr != other.i_ptr; }
			[[nodiscard]] constexpr RB_Node<value_type>* get_node_ptr()&
			{
				auto* temp = reinterpret_cast<char*>(this->i_ptr) - offsetof(RB_Node<value_type>, n_hook);
				return reinterpret_cast<RB_Node<value_type>*>(temp);
			}
			[[nodiscard]] constexpr reference operator*()
			{
				return this->get_node_ptr()->n_data;
			}
			bool is_root() const&
			{
				return this->i_ptr->is_root();
			}
			bool is_sentinel() const&
			{
				return this->i_ptr->is_sentinel();
			}
			bool is_left() const&
			{
				return this->i_ptr->is_left();
			}
			bool is_right() const&
			{
				return this->i_ptr->is_right();
			}
			Iterator& go_left()&
			{
				this->i_ptr = this->i_ptr->h_left;
				return *this;
			}
			Iterator& go_right()&
			{
				this->i_ptr = this->i_ptr->h_right;
				return *this;
			}
			Iterator& go_parent()&
			{
				this->i_ptr = this->i_ptr->get_parent_ptr();
				return *this;
			}
			bool is_leaf()const&
			{
				if (this->i_ptr->h_left || this->i_ptr->h_right)
				{
					return false;
				}
				return true;
			}
			bool is_internal()const&
			{
				return !is_leaf();
			}
			Iterator& operator++()&
			{
				//if ++ is call on iterator end(), it will loop back to the smallest element.
				if (this->is_sentinel())
				{
					this->go_left();
				}
				else
				{
					Iterator temp = *this;
					if (temp.i_ptr->h_right)
					{
						temp.go_right();
						while (temp.i_ptr->h_left)
						{
							temp.go_left();
						}
					}
					else
					{
						while ((!temp.is_root()) && temp.is_right())
						{
							temp.go_parent();
						}
						temp.go_parent();
					}
					this->i_ptr = temp.i_ptr;
				}
				return *this;
			}
			Iterator& operator--()&
			{
				//if -- is call on iterator end(), it will go to the largest element.
				if (this->is_sentinel())
				{
					this->go_right();
				}
				else
				{
					Iterator temp = *this;
					if (temp.i_ptr->h_left)
					{
						temp.go_left();
						while (temp.i_ptr->h_right)
						{
							temp.go_right();
						}
					}
					else
					{
						while ((!temp.is_root()) && temp.is_left())
						{
							temp.go_parent();
						}
						temp.go_parent();
					}
					this->i_ptr = temp.i_ptr;
				}
				return *this;
			}
			bool is_red()
			{
				return this->i_ptr && this->i_ptr->is_red();
			}
			bool is_black()
			{
				return !(this->is_red());
			}
			Iterator get_sib()
			{
				Iterator temp{ nullptr };
				temp.i_ptr = (this->i_ptr == this->i_ptr->get_parent_ptr()->h_left ? this->i_ptr->get_parent_ptr()->h_right : this->i_ptr->get_parent_ptr()->h_left);
				return temp;
			}
			size_type get_level()&
			{
				size_type level{ 0 };
				Hook* temp = this->i_ptr;
				while (!(temp->is_sentinel()))
				{
					temp = temp->get_parent_ptr();
					++level;
				}
				return level;
			}
		};
		[[nodiscard]] constexpr Iterator root()&
		{
			return Iterator(m_sentinel.get_parent_ptr());
		}
		[[nodiscard]] constexpr Iterator begin()&
		{
			return Iterator(m_sentinel.h_left);
		}
		[[nodiscard]] constexpr Iterator end()&
		{
			return Iterator(&m_sentinel);
		}
		[[nodiscard]] constexpr Iterator rbegin()&
		{
			return Iterator(m_sentinel.h_right);
		}
		[[nodiscard]] constexpr Iterator rend()&
		{
			return Iterator(&m_sentinel);
		}
		[[nodiscard]] constexpr Iterator min()&
		{
			return Iterator(m_sentinel.h_left);
		}
		[[nodiscard]] constexpr Iterator max()&
		{
			return Iterator(m_sentinel.h_right);
		}
		//Return an iterator that hold the sentinel if value not found
		Iterator find(const value_type& key)&
		{
			if (m_size == 0)
			{
				return this->end();
			}
			Iterator curr_node = this->root();
			while (curr_node.i_ptr)
			{
				if (key == *curr_node)
				{
					break;
				}
				(key < *curr_node ? curr_node.go_left() : curr_node.go_right());
			}
			if (curr_node.i_ptr)
			{
				return curr_node;
			}
			else
			{
				curr_node.i_ptr = &m_sentinel;
				return curr_node;
			}
		}
		void ll_rotation(Iterator child_to_parent, Iterator parent_to_r_child)
		{
			Hook* grand_par_hook = parent_to_r_child.i_ptr->get_parent_ptr();
			if (grand_par_hook == &m_sentinel)
			{
				grand_par_hook->set_parent_ptr(child_to_parent.i_ptr);
			}
			else
			{
				(parent_to_r_child.is_left() ? grand_par_hook->h_left = child_to_parent.i_ptr : grand_par_hook->h_right = child_to_parent.i_ptr);
			}
			child_to_parent.i_ptr->set_parent_ptr(grand_par_hook);
			Hook* curr_r_child = child_to_parent.i_ptr->h_right;
			parent_to_r_child.i_ptr->h_left = curr_r_child;
			if (curr_r_child)
			{
				curr_r_child->set_parent_ptr(parent_to_r_child.i_ptr);
			}
			child_to_parent.i_ptr->h_right = parent_to_r_child.i_ptr;
			parent_to_r_child.i_ptr->set_parent_ptr(child_to_parent.i_ptr);
		}
		void rr_rotation(Iterator child_to_parent, Iterator parent_to_l_child)
		{
			Hook* grand_par_hook = parent_to_l_child.i_ptr->get_parent_ptr();
			if (grand_par_hook == &m_sentinel)
			{
				grand_par_hook->set_parent_ptr(child_to_parent.i_ptr);
			}
			else
			{
				(parent_to_l_child.is_right() ? grand_par_hook->h_right = child_to_parent.i_ptr : grand_par_hook->h_left = child_to_parent.i_ptr);
			}
			child_to_parent.i_ptr->set_parent_ptr(grand_par_hook);
			Hook* curr_l_child = child_to_parent.i_ptr->h_left;
			parent_to_l_child.i_ptr->h_right = curr_l_child;
			if (curr_l_child)
			{
				curr_l_child->set_parent_ptr(parent_to_l_child.i_ptr);
			}
			child_to_parent.i_ptr->h_left = parent_to_l_child.i_ptr;
			parent_to_l_child.i_ptr->set_parent_ptr(child_to_parent.i_ptr);
		}
		void lr_rotation(Iterator child_to_grand_parent, Iterator parent_to_l_child)
		{
			rr_rotation(child_to_grand_parent, parent_to_l_child);
			parent_to_l_child.go_parent();
			parent_to_l_child.go_parent();
			ll_rotation(child_to_grand_parent, parent_to_l_child);
		}
		void rl_rotation(Iterator child_to_grand_parent, Iterator parent_to_r_child)
		{
			ll_rotation(child_to_grand_parent, parent_to_r_child);
			parent_to_r_child.go_parent();
			parent_to_r_child.go_parent();
			rr_rotation(child_to_grand_parent, parent_to_r_child);
		}
		void rotate_for_red_red_violation(Iterator curr_node, Iterator parent_node)
		{
			if (parent_node.is_left())
			{
				if (curr_node.is_left())
				{
					curr_node.go_parent();
					parent_node.go_parent();
					ll_rotation(curr_node, parent_node);
					curr_node.i_ptr->tag_black();
					parent_node.i_ptr->tag_red();
				}
				else
				{
					lr_rotation(curr_node, parent_node);
					curr_node.i_ptr->tag_black();
					curr_node.go_right();
					curr_node.i_ptr->tag_red();
				}
			}
			else
			{
				if (curr_node.is_right())
				{
					curr_node.go_parent();
					parent_node.go_parent();
					rr_rotation(curr_node, parent_node);
					curr_node.i_ptr->tag_black();
					parent_node.i_ptr->tag_red();
				}
				else
				{
					rl_rotation(curr_node, parent_node);
					curr_node.i_ptr->tag_black();
					curr_node.go_left();
					curr_node.i_ptr->tag_red();
				}
			}
		}
		void process_violation(Iterator curr_node, Iterator parent_node)
		{
			while (curr_node.is_red() && parent_node.is_red())
			{
				Iterator sib = parent_node.get_sib();
				if (sib.is_red())//if uncle of current node is red we recolor
				{
					//Advance curr_node and parent node up from 1st red node to recolor
					curr_node.go_parent();
					parent_node.go_parent();
					curr_node.i_ptr->tag_black();
					sib.i_ptr->tag_black();
					parent_node.i_ptr->tag_red();
					m_sentinel.get_parent_ptr()->tag_black(); //repeatedly tag root black since it cheaper than a condition
					m_sentinel.tag_red();
					curr_node.go_parent();
					parent_node.go_parent();
				}
				else //if uncle of currnet node is nullptr or black we rotate
				{
					rotate_for_red_red_violation(curr_node, parent_node);
				}
			}
		}
		void insert(value_type key)
		{
			Iterator curr_node = this->root();
			Iterator parent_node{ nullptr };
			if (m_size == 0)
			{
				RB_Node<value_type>* new_node_ptr = new RB_Node<value_type>(std::move(key), &m_sentinel);
				new_node_ptr->n_hook.tag_black();
				m_sentinel.h_left = m_sentinel.h_right = (&(new_node_ptr->n_hook));
				new_node_ptr->n_hook.set_parent_ptr(&m_sentinel);
				m_sentinel.set_parent_ptr(&(new_node_ptr->n_hook));
				m_size = 1;
				return;
			}
			bool inserted(false);
			while (!inserted)
			{
				if (*curr_node == key)
				{
					return;
				}
				if (key < *curr_node)
				{
					if (curr_node.i_ptr->h_left)
					{
						curr_node.go_left();
					}
					else
					{
						RB_Node<value_type>* new_node_ptr = new RB_Node<value_type>(std::move(key), curr_node.i_ptr);
						curr_node.i_ptr->h_left = &(new_node_ptr->n_hook);
						inserted = true;
						parent_node = curr_node;
						curr_node.go_left();
						if (*curr_node < *(this->min()))
						{
							m_sentinel.h_left = curr_node.i_ptr;
						}
					}
				}
				else
				{
					if (curr_node.i_ptr->h_right)
					{
						curr_node.go_right();
					}
					else
					{
						RB_Node<value_type>* new_node_ptr = new RB_Node<value_type>(std::move(key), curr_node.i_ptr);
						curr_node.i_ptr->h_right = &(new_node_ptr->n_hook);
						inserted = true;
						parent_node = curr_node;
						curr_node.go_right();
						if (*curr_node > *(this->max()))
						{
							m_sentinel.h_right = curr_node.i_ptr;
						}
					}
				}
			}
			if (parent_node.i_ptr->is_red())
			{
				process_violation(curr_node, parent_node);
			}
			++m_size;
		}
		void remove(const value_type& key)
		{
			Iterator curr_node = this->find(key);
			if (curr_node.i_ptr == &m_sentinel)
			{
				return;
			}
			if (curr_node.is_leaf())
			{
				remove_leaf_node(curr_node);
			}
			else
			{
				remove_internal_node(curr_node);
			}
			--m_size;
			if (m_size == 0)
			{
				m_sentinel.h_left = m_sentinel.h_right = &m_sentinel;
				m_sentinel.set_parent_ptr(&m_sentinel);
			}
			else
			{
				Hook* root = m_sentinel.get_parent_ptr();
				m_sentinel.h_left = find_min(root);
				m_sentinel.h_right = find_max(root);
			}
		}
		//Default constructor will create a hook on the stack and act as a sentinel
		RB_Tree() : m_sentinel{}, m_size{ 0 }
        {
            m_sentinel.set_parent_ptr(&m_sentinel);
            m_sentinel.h_left = m_sentinel.h_right = &m_sentinel;
        }
		template<typename Iter>
		RB_Tree(Iter begin, Iter end) : m_sentinel{}, m_size{ 0 }
		{
			while (begin != end)
			{
				this->insert(std::move(*begin));
				++begin;
			}
		}
		RB_Tree(std::vector<value_type> vect) : RB_Tree(vect.begin(), vect.end()) {}
		template<size_type s>
		RB_Tree(std::array<value_type, s> arr) : RB_Tree(arr.begin(), arr.end()) {}
		RB_Tree(std::initializer_list<value_type> list) : RB_Tree(list.begin(), list.end()) {}
		~RB_Tree() { destroy_all_node(); std::cout << "\nDestructor called!!!"; }
		friend std::ostream& operator<< (std::ostream& out, RB_Tree<value_type>& tree)
		{
			if (tree.m_size <= 0)
			{
				out << "Tree is empty!!!";
			}
			else
			{
				Iterator iter{ tree.rbegin() };
				while (!iter.is_sentinel())
				{
					std::string prefix = "";
					for (size_type i{ 0 }; i < iter.get_level(); ++i)
					{
						prefix = prefix + "      ";
					}
					if (iter.is_root())
					{
						prefix += "[Root: ";
					}
					else
					{
						(iter.is_left() ? prefix += "L___[" : prefix += "TTTT[");
					}
					out << prefix << (iter.i_ptr->is_red() ? "red:" : "blk:") << *iter << "]\n";
					--iter;
				}
			}
			return out;
		}
	private:
		Hook m_sentinel;
		size_type m_size;
		void destroy_all_node()
		{
			Iterator curr_node{ this->root() };
			if (m_size == 0) { return; }
			//Get a leaf node by traversing most left then if its not a leaf node, move right and repeat
			while (curr_node.is_internal())
			{
				while (curr_node.i_ptr->h_left)
				{
					curr_node.go_left();
				}
				if (curr_node.is_internal())
				{
					curr_node.go_right();
				}
			}
			while (m_size > 0 &&!curr_node.is_sentinel())
			{
				Iterator temp = curr_node;
				if (curr_node.is_left())
				{
					curr_node.go_parent();
					while (curr_node.i_ptr->h_right)
					{
						curr_node.go_right();
						while (curr_node.i_ptr->h_left)
						{
							curr_node.go_left();
						}
					}
				}
				else
				{
					curr_node.go_parent();
				}
				delete temp.get_node_ptr();
				--m_size;
			}
		}
		Iterator get_red_child_of(Iterator curr_node) // return i_ptr == nullptr if sib has no red child
		{
			if (curr_node.i_ptr->h_left && curr_node.i_ptr->h_left->is_red())
			{
				return curr_node.go_left();
			}
			else if (curr_node.i_ptr->h_right && curr_node.i_ptr->h_right->is_red())
			{
				return curr_node.go_right();
			}
			else
			{
				curr_node.i_ptr = nullptr;
				return curr_node;
			}
		}
		void make_nephew_far(Iterator& nephew, Iterator& sib, Iterator& double_black_node)
		{
			if ((double_black_node.is_left() && nephew.is_right()) || (double_black_node.is_right() && nephew.is_left()))
			{
				return;
			}
			else
			{
				if (nephew.is_left() && double_black_node.is_left())
				{
					Iterator other_nephew = nephew.get_sib();
					if (other_nephew.i_ptr && other_nephew.is_red())
					{
						nephew = other_nephew;
						return;
					}
					else
					{
						ll_rotation(nephew, sib);
						std::swap(nephew.i_ptr, sib.i_ptr);
					}
				}
				else if(nephew.is_right() && double_black_node.is_right())
				{
					Iterator other_nephew = nephew.get_sib();
					if (other_nephew.i_ptr && other_nephew.is_red())
					{
						nephew = other_nephew;
						return;
					}
					else
					{
						rr_rotation(nephew, sib);
						std::swap(nephew.i_ptr, sib.i_ptr);
					}
				}
			}
		}
		void process_double_black(Iterator double_black_node)
		{
			while (true)
			{
				if (double_black_node.is_root())
				{
					break;
				}
				Iterator sib = double_black_node.get_sib();
				Iterator parent = double_black_node.i_ptr->get_parent_ptr();
				if (sib.is_black())
				{
					Iterator nephew = get_red_child_of(sib);
					if (nephew.i_ptr)//red child exist
					{
						make_nephew_far(nephew, sib, double_black_node);
						bool parent_is_red{ parent.is_red() };
						if (double_black_node.is_left())
						{
							rr_rotation(sib, parent);
							nephew.i_ptr->tag_black();
							parent.i_ptr->tag_black();
						}
						else
						{
							ll_rotation(sib, parent);
							nephew.i_ptr->tag_black();
							parent.i_ptr->tag_black();
						}
						if (parent_is_red)
						{
							sib.i_ptr->tag_red();
						}
						else
						{
							sib.i_ptr->tag_black();
						}
						break;
					}
					else//no red child
					{
						if (parent.i_ptr->is_red())
						{
							parent.i_ptr->tag_black();
							sib.i_ptr->tag_red();
							break;
						}
						else
						{
							sib.i_ptr->tag_red();
							double_black_node = parent;
						}
					}
				}
				else//sib is red so parent is default black
				{
					if (double_black_node.is_left())
					{
						rr_rotation(sib, parent);
						sib.i_ptr->tag_black();
						parent.i_ptr->tag_red();
					}
					else
					{
						ll_rotation(sib, parent);
						sib.i_ptr->tag_black();
						parent.i_ptr->tag_red();
					}
				}
			}
		}
		void remove_internal_node_with_one_child(Iterator node_to_delete)
		{
			Iterator temp = node_to_delete;
			(temp.i_ptr->h_left ? temp.go_left() : temp.go_right());
			node_to_delete.i_ptr->swap_position(temp.i_ptr);
			temp.i_ptr->tag_black();
			temp.i_ptr->h_left = nullptr;
			temp.i_ptr->h_right = nullptr;
			delete node_to_delete.get_node_ptr();
		}
		void remove_internal_node(Iterator node_to_delete)
		{
			if (node_to_delete.i_ptr->h_left == nullptr || node_to_delete.i_ptr->h_right == nullptr)
			{
				remove_internal_node_with_one_child(node_to_delete);
			}
			else
			{
				Iterator successor = node_to_delete;
				++successor;
				if (successor.i_ptr->h_right) //successor has one child which mean its black and it child is on the right and is red
				{
					bool node_is_red{ node_to_delete.is_red() };
					node_to_delete.i_ptr->swap_position(successor.i_ptr);
					if (node_is_red)
					{
						successor.i_ptr->tag_red();
					}
					else
					{
						successor.i_ptr->tag_black();

					}
					remove_internal_node_with_one_child(node_to_delete);
				}
				else if(successor.is_red()) //successor is leaf and is red
				{
					bool node_is_red{ node_to_delete.is_red() };
					node_to_delete.i_ptr->swap_position(successor.i_ptr);
					if (node_to_delete.is_left())
					{
						node_to_delete.i_ptr->get_parent_ptr()->h_left = nullptr;
						delete node_to_delete.get_node_ptr();
					}
					else
					{
						node_to_delete.i_ptr->get_parent_ptr()->h_right = nullptr;
						delete node_to_delete.get_node_ptr();
					}
					if (node_is_red)
					{
						successor.i_ptr->tag_red();
					}
					else
					{
						successor.i_ptr->tag_black();
					}
				}
				else//successor is leaf and black
				{
					bool node_is_red{ node_to_delete.is_red() };
					node_to_delete.i_ptr->swap_position(successor.i_ptr);
					if (node_is_red)
					{
						successor.i_ptr->tag_red();
					}
					else
					{
						successor.i_ptr->tag_black();
					}
					process_double_black(node_to_delete);
					if (node_to_delete.is_left())
					{
						node_to_delete.i_ptr->get_parent_ptr()->h_left = nullptr;
						delete node_to_delete.get_node_ptr();
					}
					else
					{
						node_to_delete.i_ptr->get_parent_ptr()->h_right = nullptr;
						delete node_to_delete.get_node_ptr();
					}
				}
			}
		}
		void remove_leaf_node(Iterator node_to_delete)
		{
			if (node_to_delete.i_ptr->is_red())
			{
				if (node_to_delete.is_left())
				{
					node_to_delete.i_ptr->get_parent_ptr()->h_left = nullptr;
					delete node_to_delete.get_node_ptr();
				}
				else
				{
					node_to_delete.i_ptr->get_parent_ptr()->h_right = nullptr;
					delete node_to_delete.get_node_ptr();
				}
			}
			else
			{
				process_double_black(node_to_delete);
				if (node_to_delete.is_left())
				{
					node_to_delete.i_ptr->get_parent_ptr()->h_left = nullptr;
					delete node_to_delete.get_node_ptr();
				}
				else
				{
					node_to_delete.i_ptr->get_parent_ptr()->h_right = nullptr;
					delete node_to_delete.get_node_ptr();
				}
			}
		}
		Hook* find_min(Hook* node)
		{
			while (node->h_left)
			{
				node = node->h_left;
			}
			return node;
		}
		Hook* find_max(Hook* node)
		{
			while (node->h_right)
			{
				node = node->h_right;
			}
			return node;
		}
	};
}