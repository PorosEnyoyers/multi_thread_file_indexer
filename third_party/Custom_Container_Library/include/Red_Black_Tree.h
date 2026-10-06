#pragma once

#include <memory>
#include <stdexcept>
#include <utility>
#include <type_traits>
#include <concepts>
#include <initializer_list>
#include <cstdint>
#include <array>
#include <vector>
#include <exception>
#include <iostream>
#include <string>
#include <cstddef>

namespace custom
{
    struct Hook
    {
    public:
        Hook* h_left = nullptr;
        Hook* h_right = nullptr;

        bool is_black() const
        {
            return h_parent & GET_COLOR_TAG_BLACK;
        }
        bool is_red() const
        {
            return !is_black();
        }
        Hook* get_p_ptr()
        {
            return reinterpret_cast<Hook*>(h_parent & GET_PTR_TAG_RED);
        }
        const Hook* get_p_ptr() const
        {
            return reinterpret_cast<Hook*>(h_parent & GET_PTR_TAG_RED);
        }
        void set_p_ptr(Hook* new_p)
        {
            uintptr_t color_tag = h_parent & GET_COLOR_TAG_BLACK;
            h_parent = reinterpret_cast<uintptr_t>(new_p) | color_tag;
        }
        void tag_black()
        {
            h_parent = h_parent | GET_COLOR_TAG_BLACK;
        }
        void tag_red()
        {
            h_parent = h_parent & GET_PTR_TAG_RED;
        }
        bool is_root() const
        {
            return this->get_p_ptr()->get_p_ptr() == this && this->is_black();
        }
        bool is_sentinel() const
        {
            return this->get_p_ptr()->get_p_ptr() == this && this->is_red();
        }
        bool is_child() const
        {
            return this->get_p_ptr()->get_p_ptr() != this;
        }
        bool is_left() const
        {
            return(this->is_child() && this == this->get_p_ptr()->h_left);
        }
        bool is_right() const
        {
            return(this->is_child() && this == this->get_p_ptr()->h_right);
        }
        //[[Danger]]: The color property of the tree will be preserved. Nodes might not retain its old color after swap.
        void swap_position(Hook* other, Hook* null_hook)
        {
            if (this == other)
            {
                return;
            }
            //Case 1: this_node is other_node's parent
            if(this == other->get_p_ptr())
            {
                Hook* this_parent = this->get_p_ptr();
                bool other_is_left(other->is_left());
                //Segment: realligned parents relationships
                if(this_parent->is_sentinel())//Deal with case this_node is root
                {
                    this_parent->set_p_ptr(other);
                }
                else //Set this_node's parent left ptr or right ptr to the other_node
                {
                    if(this->is_left())
                    {
                        this_parent->h_left = other;
                    }
                    else
                    {
                        this_parent->h_right = other;
                    }
                }
                //[[Danger]]: the color of both node is swapped if they are different color
                std::swap(this->h_parent, other->h_parent);
                this->set_p_ptr(other);
                other->set_p_ptr(this_parent);
                //Segment: realligned children relationships
                std::swap(this->h_left, other->h_left);
                std::swap(this->h_right, other->h_right);
                if(other_is_left)
                {
                    other->h_left = this;
                    //Make sure the node is not nullptr before dereference it.
                    if (other->h_right != null_hook)
                    {
                        other->h_right->set_p_ptr(other);
                    }
                }
                else 
                {
                    other->h_right = this;
                    //Make sure the node is not nullptr before dereference it.
                    if(other->h_left != null_hook)
                    {
                        other->h_left->set_p_ptr(other);
                    }
                }
                //Make sure the node is not nullptr before dereference it.
                if(this->h_left != null_hook)
                {
                    this->h_left->set_p_ptr(this);
                }
                 //Make sure the node is not nullptr before dereference it.
                if(this->h_right != null_hook)
                {
                    this->h_right->set_p_ptr(this);
                }
            }
            //case 2: other_node is this_node's parent
            else if (this->get_p_ptr() == other)
            {
				Hook* other_parent = other->get_p_ptr();
				bool this_is_left = this->is_left();
				if (other_parent->is_sentinel())
				{
					other_parent->set_p_ptr(this);
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
                //[[Danger]]: the color of both node is swapped if they are different color
				std::swap(this->h_parent, other->h_parent);
				other->set_p_ptr(this);
				this->set_p_ptr(other_parent);
				//deal with children relationship
				std::swap(this->h_left, other->h_left);
				std::swap(this->h_right, other->h_right);
				if (this_is_left)
				{
					this->h_left = other;
					if (this->h_right != null_hook)
					{
						this->h_right->set_p_ptr(this);
					}
				}
				else
				{
					this->h_right = other;
					if (this->h_left != null_hook)
					{
						this->h_left->set_p_ptr(this);
					}
				}
				if (other->h_left != null_hook)
				{
					other->h_left->set_p_ptr(other);
				}
				if (other->h_right != null_hook)
				{
					other->h_right->set_p_ptr(other);
				}
            }
            //case 3: this_node and other_node are not parent_child of each other
            else
            {
                Hook* this_parent = this->get_p_ptr();
                Hook* other_parent = other->get_p_ptr();
                //Segment: Deal with parents
                if(this_parent == other_parent) //Case this_node and other_node are siblings
                {
                    std::swap(this_parent->h_left, this_parent->h_right);
                }
                else 
                {
                    if (this_parent->is_sentinel())
                    {
                        this_parent->set_p_ptr(other);
                    }
                    else if(this->is_left())
                    {
                        this_parent->h_left = other;
                    }
                    else
                    {
                        this_parent->h_right = other;
                    }
                    if(other_parent->is_sentinel())
                    {
                        other_parent->set_p_ptr(this);
                    }
                    else if(other->is_left())
                    {
                        other_parent->h_left = this;
                    }
                    else 
                    {
                        other_parent->h_right = this;
                    }
                }
                //[[Danger]]: the color of both node is swapped if they are different color
                std::swap(this->h_parent, other->h_parent);
                //Segment: Deal with children
                std::swap(this->h_left, other->h_left);
                std::swap(this->h_right, other->h_right);
                if (other->h_left != null_hook)
				{
					other->h_left->set_p_ptr(other);
				}
				if (other->h_right != null_hook)
				{
					other->h_right->set_p_ptr(other);
				}
				if (this->h_left != null_hook)
				{
					this->h_left->set_p_ptr(this);
				}
				if (this->h_right != null_hook)
				{
					this->h_right->set_p_ptr(this);
				}
            }
        }
        //A newly created node will always be red
        Hook() : h_left{nullptr}, h_right{nullptr}, h_parent{0}
        {
        }
        Hook(const Hook& other) = delete;
        Hook(Hook&& other) = delete;
        Hook& operator=(const Hook& other) = delete;
        Hook& operator=(Hook&& other) = delete;
        ~Hook() = default;
    private:
        //A pointer on a modern computer will have at least 2 unused least significant bit which we will use to indicate if an node is red or black, (if tagged is black, non tagged is red)
        std::uintptr_t h_parent = 0;
        static constexpr uintptr_t GET_COLOR_TAG_BLACK = 0x1;
        static constexpr uintptr_t GET_PTR_TAG_RED = ~0x1;
    };

    template<std::movable T>
    struct RB_Node : Hook
    {
        T n_data;

        RB_Node() = delete;
        RB_Node(T data, Hook* parent_ptr = nullptr) : Hook{}, n_data{std::move(data)}
        {
            this->set_p_ptr(parent_ptr);
        }
        RB_Node(const RB_Node& other) = delete;
        RB_Node& operator=(const RB_Node& other) = delete;
        RB_Node(RB_Node&& other) = delete;
        RB_Node& operator=(RB_Node&& other) = delete;
        ~RB_Node() = default;
    };

    template <typename Tree>
    class rb_tree_iterator;

    template<typename T>
    requires std::movable<T> && std::totally_ordered<T>
    class RB_Tree
    {
    public:
        //Alias
        using size_type = std::size_t;
        using value_type = T;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;
        using iterator = rb_tree_iterator<RB_Tree<T>>;
        using const_iterator = rb_tree_iterator<const RB_Tree<T>>;
        //Rules of five
        RB_Tree() : m_sentinel{}, m_size{0}
        {
            m_sentinel.set_p_ptr(&m_sentinel);
            m_sentinel.h_left = m_sentinel.h_right = &m_sentinel;
        }
        template<typename Iter>
        RB_Tree(Iter begin, Iter end) : RB_Tree()
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
        RB_Tree(const RB_Tree& other) = delete;
        RB_Tree(RB_Tree&& other) = delete;
        RB_Tree& operator=(const RB_Tree& other) = delete;
        RB_Tree& operator=(RB_Tree&& other) = delete;
        ~RB_Tree()
        {
            this->destroy_all_node();
        }
        friend std::ostream& operator<< (std::ostream& out, RB_Tree<value_type>& tree)
        {
            if (tree.m_size <= 0)
            {
                out << "Tree is empty!!!";
            }
            else
            {
                iterator iter{tree.rbegin()};
                while (!iter.is_sentinel())
                {
                    std::string prefix = "";
                    for (size_type i{ 0 }; i < iter.get_level(); ++i)
                    {
                        prefix = prefix + "     ";
                    }
                    if (iter.is_root())
                    {
                        prefix += "[Root: ";
                    }
                    else
                    {
                        (iter.is_left() ? prefix += "L__[" : prefix += "TTT[");
                    }
                    out << prefix << (iter.is_red() ? "red: " : "blk: ") << *iter << "]\n";
                    --iter;
                }
            }
            return out;
        }
        //Insert Functions
        void insert(value_type value)
        {
            iterator curr = this->root();
            iterator parent = this->end();
            if (m_size == 0)
            {
                RB_Node<value_type>* new_node = new RB_Node<value_type>(std::move(value), &m_sentinel);
                new_node->tag_black();
                m_sentinel.h_left = m_sentinel.h_right = static_cast<Hook*>(new_node);
                new_node->set_p_ptr(&m_sentinel);
                m_sentinel.set_p_ptr(static_cast<Hook*>(new_node));
                new_node->h_left = new_node->h_right = &m_sentinel;
                m_size = 1;
                return;
            }
            bool inserted(false);
            while(!inserted)
            {
                if(*curr == value)
                {
                    return;
                }
                if(value < *curr)
                {
                    if(curr.get_hook()->h_left != &m_sentinel)
                    {
                        curr.go_left();
                    }
                    else
                    {
                        RB_Node<value_type>* new_node_ptr = new RB_Node<value_type>(std::move(value), curr.get_hook());
                        new_node_ptr->h_left = new_node_ptr->h_right = &m_sentinel;
                        curr.get_hook()->h_left = static_cast<Hook*>(new_node_ptr);
                        inserted = true;
                        parent = curr;
                        curr.go_left();
                        if(*curr < *(this->min()))
                        {
                            m_sentinel.h_left = curr.get_hook();
                        }
                    }
                }
                else
                {
                    if(curr.get_hook()->h_right != &m_sentinel)
                    {
                        curr.go_right();
                    }
                    else
                    {
                        RB_Node<value_type>* new_node_ptr = new RB_Node<value_type>(std::move(value), curr.get_hook());
                        new_node_ptr->h_left = new_node_ptr->h_right = &m_sentinel;
                        curr.get_hook()->h_right = static_cast<Hook*>(new_node_ptr);
                        inserted = true;
                        parent = curr;
                        curr.go_right();
                        if(*curr > *(this->max()))
                        {
                            m_sentinel.h_right = curr.get_hook();
                        }
                    }
                }
            }
            if(parent.is_red())
            {
               this->process_violation(curr, parent);
            }
            ++m_size;
        }
        template<typename Iter>
        void insert(Iter begin, Iter end)
        {
            while (begin != end)
            {
                this->insert(std::move(*begin));
                ++begin;
            }
        }
        void insert(std::vector<value_type> vect)
        {
            insert(vect.begin(), vect.end());
        }
        template<size_type s>
        void insert(std::array<value_type, s> arr)
        {
            insert(arr.begin(), arr.end());
        }
        void insert(std::initializer_list<value_type> list)
        {
            insert(list.begin(), list.end());
        }
        //Find functions will return the sentinel if not found and return an interator at the node location if found
        iterator find(const value_type& key)&
        {
            iterator end = this->end();
            if(m_size == 0)
            {
                return end;
            }
            iterator curr = this->root();
            while(curr != end)
            {
                if (key == *curr)
                {
                    break;
                }
                (key < *curr ? curr.go_left() : curr.go_right());
            }
            return curr;
        }
        [[nodiscard]] std::vector<iterator> find_range(const value_type& lower, const value_type& upper)&
        {
            if(m_size == 0 || lower > upper)
            {
                return std::vector<iterator>();
            }
            iterator curr = this->root();
            iterator low_bound = this->end();
            std::vector<iterator> result;
            while (curr != this->end())
            {
                if(*curr < lower)
                {
                    curr.go_right();
                }
                else 
                {
                    low_bound = curr;
                    curr.go_left();
                }
            }
            if(low_bound == this->end() || *low_bound > upper)
            {
                return result;
            }
            iterator max_node = this->max();
            while(true)
            {
                result.push_back(low_bound);
                if(*low_bound >= upper || low_bound == max_node)
                {
                    break;
                }
                ++low_bound;
            }
            return result;
        }
        const_iterator find(const value_type& key)const &
        {
            const_iterator end = this->end();
            if(m_size == 0)
            {
                return end;
            }
            const_iterator curr = this->root();
            while(curr != end)
            {
                if (key == *curr)
                {
                    break;
                }
                (key < *curr ? curr.go_left() : curr.go_right());
            }
            return curr;
        }
        //Find and delete a node function
        bool remove(const value_type& key)
        {
            auto curr = this->find(key);
            if(curr.is_sentinel())
            {
                return false;
            }
            if(curr.is_leaf())
            {
                this->remove_leaf_node(curr);
            }
            else 
            {
                this->remove_internal_node(curr);
            }
            --m_size;
            if (m_size == 0)
            {
                m_sentinel.h_left = m_sentinel.h_right = &m_sentinel;
                m_sentinel.set_p_ptr(&m_sentinel);
            }
            else 
            {
                update_min_max();
            }
            return true;
        }
        //Utility Functions
        const Hook* get_sentinel_ptr() const
        {
            return &(this->m_sentinel);
        }
        size_type size() const
        {
            return m_size;
        }
        //Constructing iterator functions
        [[nodiscard]] iterator root()&
        {
            return iterator(this,m_sentinel.get_p_ptr());
        }
        [[nodiscard]] iterator begin()&
        {
            return iterator(this, m_sentinel.h_left);
        }
        [[nodiscard]] iterator end()&
        {
            return iterator(this, &m_sentinel);
        }
        [[nodiscard]] iterator rbegin()&
        {
            return iterator(this, m_sentinel.h_right);
        }
        [[nodiscard]] iterator rend()&
        {
            return iterator(this, &m_sentinel);
        }
        [[nodiscard]] iterator min()&
        {
            return iterator(this, m_sentinel.h_left);
        }
        [[nodiscard]] iterator max()&
        {
            return iterator(this, m_sentinel.h_right);
        }
        [[nodiscard]]const_iterator root() const &
        {
            return const_iterator(this,m_sentinel.get_p_ptr());
        }
        [[nodiscard]]const_iterator begin() const &
        {
            return const_iterator(this, m_sentinel.h_left);
        }
        [[nodiscard]]const_iterator end() const &
        {
            return const_iterator(this, &m_sentinel);
        }
        [[nodiscard]]const_iterator rbegin() const &
        {
            return const_iterator(this, m_sentinel.h_right);
        }
        [[nodiscard]]const_iterator rend() const &
        {
            return const_iterator(this, &m_sentinel);
        }
        [[nodiscard]]const_iterator min() const &
        {
            return const_iterator(this, m_sentinel.h_left);
        }
        [[nodiscard]]const_iterator max() const &
        {
            return const_iterator(this, m_sentinel.h_right);
        }
    private:
        //Update min max
        void update_min_max()
        {
            Hook* root = m_sentinel.get_p_ptr();
            while(root->h_left != &m_sentinel)
            {
                root = root->h_left;
            }
            m_sentinel.h_left = root;
            root = m_sentinel.get_p_ptr();
            while(root->h_right != &m_sentinel)
            {
                root = root->h_right;
            }
            m_sentinel.h_right = root;
        }
        //Rotate functions
        void ll_rotation(Hook* child_to_parent, Hook* parent_to_r_child)
        {
            Hook* grandpa = parent_to_r_child->get_p_ptr();
            if(grandpa == &m_sentinel)
            {
                grandpa->set_p_ptr(child_to_parent);
            }
            else
            {
                (parent_to_r_child->is_left() ? grandpa->h_left = child_to_parent : grandpa->h_right = child_to_parent);
            }
            child_to_parent->set_p_ptr(grandpa);
            Hook* curr_r_child = child_to_parent->h_right;
            parent_to_r_child->h_left = curr_r_child;
            if (curr_r_child != &m_sentinel)
            {
                curr_r_child->set_p_ptr(parent_to_r_child);
            }
            child_to_parent->h_right = parent_to_r_child;
            parent_to_r_child->set_p_ptr(child_to_parent);
        }
        void rr_rotation(Hook* child_to_parent, Hook* parent_to_l_child)
        {
            Hook* grandpa = parent_to_l_child->get_p_ptr();
            if (grandpa == & m_sentinel)
            {
                grandpa->set_p_ptr(child_to_parent);
            }
            else
            {
                (parent_to_l_child->is_right() ? grandpa->h_right = child_to_parent : grandpa->h_left = child_to_parent);
            }
            child_to_parent->set_p_ptr(grandpa);
            Hook* curr_l_child = child_to_parent->h_left;
            parent_to_l_child->h_right = curr_l_child;
            if(curr_l_child != &m_sentinel)
            {
                curr_l_child->set_p_ptr(parent_to_l_child);
            }
            child_to_parent->h_left = parent_to_l_child;
            parent_to_l_child->set_p_ptr(child_to_parent);
        }
        void lr_rotation(Hook* child_to_grand_parent, Hook* parent_to_l_child)
        {
            rr_rotation(child_to_grand_parent, parent_to_l_child);
            parent_to_l_child = parent_to_l_child->get_p_ptr();
            parent_to_l_child = parent_to_l_child->get_p_ptr();
            ll_rotation(child_to_grand_parent, parent_to_l_child);
        }
        void rl_rotation(Hook* child_to_grand_parent, Hook* parent_to_r_child)
        {
            ll_rotation(child_to_grand_parent, parent_to_r_child);
            parent_to_r_child = parent_to_r_child->get_p_ptr();
            parent_to_r_child = parent_to_r_child->get_p_ptr();
            rr_rotation(child_to_grand_parent,parent_to_r_child);
        }
        void process_violation(iterator curr, iterator parent)
        {
            while(curr.is_red() && parent.is_red())
            {
                iterator uncle = parent.get_sib();
                if((!uncle.is_sentinel()) && uncle.is_red())//if uncle of currnode is red we recolor
                {
                    curr.go_parent();
                    parent.go_parent();
                    curr.tag_black();
                    uncle.tag_black();
                    parent.tag_red();
                    m_sentinel.get_p_ptr()->tag_black();//Tag root black regardless
                    m_sentinel.tag_red();
                    curr.go_parent();
                    parent.go_parent();
                }
                else//if uncle of curr node is black or null we rotate
                {
                    this->rotate_for_red_red_violation(curr, parent);
                }
            }
        }
        void rotate_for_red_red_violation(iterator curr, iterator parent)
        {
            if(parent.is_left())
            {
                if(curr.is_left())
                {
                    curr.go_parent();
                    parent.go_parent();
                    ll_rotation(curr.get_hook(), parent.get_hook());
                    curr.tag_black();
                    parent.tag_red();
                }
                else
                {
                    lr_rotation(curr.get_hook(), parent.get_hook());
                    curr.tag_black();
                    curr.go_right();
                    curr.tag_red();
                }
            }
            else
            {
                if(curr.is_right())
                {
                    curr.go_parent();
                    parent.go_parent();
                    rr_rotation(curr.get_hook(), parent.get_hook());
                    curr.tag_black();
                    parent.tag_red();
                }
                else
                {
                    rl_rotation(curr.get_hook(), parent.get_hook());
                    curr.tag_black();
                    curr.go_left();
                    curr.tag_red();
                }
            }
        }
        void destroy_all_node()
        {
            iterator curr{this->root()};
            if (m_size == 0) {return;}
            //Get a leaf node by traversing most left then if its not a leaf node, move right, repeat till reaching a leaf node
            while (curr.is_internal())
            {
                while(!curr.left_null())
                {
                    curr.go_left();
                }
                if(curr.is_internal())
                {
                    curr.go_right();
                }
            }
            while (!curr.is_sentinel())
            {
                auto temp = curr;
                if(curr.is_left())
                {
                    curr.go_parent();
                    while(!curr.right_null())
                    {
                        curr.go_right();
                        while(!curr.left_null())
                        {
                            curr.go_left();
                        }
                    }
                }
                else
                {
                    curr.go_parent();
                }
                delete temp.get_node_ptr();
                --m_size;
            }
        }
        void remove_leaf_node(iterator node)
        {
            if(node.is_red())
            {
                if (node.is_left())
                {
                    node.get_p_ptr()->h_left = &m_sentinel;
                    delete node.get_node_ptr();
                }
                else 
                {
                    node.get_p_ptr()->h_right = &m_sentinel;
                    delete node.get_node_ptr();
                }
            }
            else
            {
                this->process_double_black(node);
                if (node.is_left())
                {
                    node.get_p_ptr()->h_left = &m_sentinel;
                    delete node.get_node_ptr();
                }
                else 
                {
                    node.get_p_ptr()->h_right = &m_sentinel;
                    delete node.get_node_ptr();
                }
            }
        }
        void process_double_black(iterator node)
        {
            while(!node.is_root())
            {
                iterator sib = node.get_sib();
                iterator parent {this, node.get_p_ptr()};
                if (sib.is_black())
                {
                    iterator nephew = this->get_red_child_of(sib);
                    if(!nephew.is_sentinel())
                    {
                        this->make_nephew_far(nephew, sib, node);
                        bool parent_is_red {parent.is_red()};
                        if (node.is_left())
                        {
                            rr_rotation(sib.get_hook(), parent.get_hook());
                            nephew.tag_black();
                            parent.tag_black();
                        }
                        else
                        {
                            ll_rotation(sib.get_hook(),parent.get_hook());
                            nephew.tag_black();
                            parent.tag_black();
                        }
                        if(parent_is_red)
                        {
                            sib.tag_red();
                        }
                        else
                        {
                            sib.tag_black();
                        }
                        break;
                    }
                    else
                    {
                        if(parent.is_red())
                        {
                            parent.tag_black();
                            sib.tag_red();
                            break;
                        }
                        else
                        {
                            sib.tag_red();
                            node = parent;
                        }
                    }
                }
                else//sib is red so parent is always black
                {
                    if(node.is_left())
                    {
                        rr_rotation(sib.get_hook(),parent.get_hook());
                        sib.tag_black();
                        parent.tag_red();
                    }
                    else 
                    {
                        ll_rotation(sib.get_hook(), parent.get_hook());
                        sib.tag_black();
                        parent.tag_red();   
                    }
                }
            }
        }
        iterator get_red_child_of(iterator node) // return sentinel iterator if sib has no red child
        {
            if(!node.left_null() && node.get_hook()->h_left->is_red())
            {
                return node.go_left();
            }
            else if (!node.right_null() && node.get_hook()->h_right->is_red())
            {
                return node.go_right();
            }
            else
            {
                return this->end();
            }
        }
        void make_nephew_far(iterator& nephew, iterator& sib, iterator& node)
        {
            if ((node.is_left() && nephew.is_right())||(node.is_right() && nephew.is_left()))
            {
                return;
            }
            else 
            {
                if(nephew.is_left() && node.is_left())
                {
                    iterator other_nephew = nephew.get_sib();
                    if(!other_nephew.is_sentinel() && other_nephew.is_red())
                    {
                        nephew = other_nephew;
                        return;
                    }
                    else 
                    {
                        ll_rotation(nephew.get_hook(), sib.get_hook());
                        std::swap(nephew, sib);
                    }
                }
                else if(nephew.is_right() && node.is_right())
                {
                    iterator other_nephew = nephew.get_sib();
                    if(!other_nephew.is_sentinel() && other_nephew.is_red())
                    {
                        nephew = other_nephew;
                        return;
                    }
                    else 
                    {
                        rr_rotation(nephew.get_hook(), sib.get_hook());
                        std::swap(nephew,sib);
                    }
                }
            }
        }
        void remove_internal_node(iterator node)
        {
            if(node.left_null() || node.right_null())
            {
                this->remove_internal_node_with_one_child(node);
            }
            else
            {
                iterator successor = node;
                ++successor;
                if(!successor.right_null()) //sucessor has a right child
                {
                    node.get_hook()->swap_position(successor.get_hook(), &m_sentinel);
                    remove_internal_node_with_one_child(node);
                }
                else if(successor.is_red()) //sucessor is leaf and red
                {
                    node.get_hook()->swap_position(successor.get_hook(),&m_sentinel);
                    if(node.is_left())
                    {
                        node.get_p_ptr()->h_left = &m_sentinel;
                        delete node.get_node_ptr();
                    }
                    else 
                    {
                        node.get_p_ptr()->h_right = &m_sentinel;
                        delete node.get_node_ptr();
                    }
                }
                else //successor is leaf and black 
                {
                    node.get_hook()->swap_position(successor.get_hook(), &m_sentinel);
                    process_double_black(node);
                    if(node.is_left())
                    {
                        node.get_p_ptr()->h_left = &m_sentinel;
                        delete node.get_node_ptr();
                    }
                    else 
                    {
                        node.get_p_ptr()->h_right = &m_sentinel;
                        delete node.get_node_ptr();
                    }
                }
            }
        }
        void remove_internal_node_with_one_child(iterator node)
        {
            iterator temp = node;
            (!temp.left_null()? temp.go_left() : temp.go_right());
            node.get_hook()->swap_position(temp.get_hook(),&m_sentinel);
            temp.tag_black();
            temp.get_hook()->h_left = temp.get_hook()->h_right = &m_sentinel;
            delete node.get_node_ptr();
        }
        Hook m_sentinel;
        size_type m_size;
        friend rb_tree_iterator<RB_Tree<T>>;
        friend rb_tree_iterator<const RB_Tree<T>>;
    };
    template <typename Tree>
    class rb_tree_iterator
    {
    public:
        //Alias
        using container_type = std::remove_const_t<Tree>;
        using value_type = typename container_type::value_type;
        using reference = std::conditional_t<std::is_const_v<Tree>, const value_type&, value_type&>;
        using pointer = std::conditional_t<std::is_const_v<Tree>, const value_type*, value_type*>;
        using size_type = std::size_t;
        using self_type = rb_tree_iterator<Tree>;
        using hook_pointer = std::conditional_t<std::is_const_v<Tree>, const Hook*, Hook*>;
        //Rule of five
        rb_tree_iterator() = delete;
        rb_tree_iterator(Tree* tree, hook_pointer hook)
        : i_tree{tree}, i_hook{hook}
        {}
        rb_tree_iterator(const rb_tree_iterator& other) = default;
        rb_tree_iterator(rb_tree_iterator&& other) = default;
        rb_tree_iterator& operator=(const rb_tree_iterator& other) = default;
        rb_tree_iterator& operator=(rb_tree_iterator&& other) = default;
        ~rb_tree_iterator() = default;

        bool operator==(const self_type& other) const {return i_hook == other.i_hook;}
        bool operator!=(const self_type& other) const {return i_hook != other.i_hook;}
        [[nodiscard]] RB_Node<value_type>* get_node_ptr()
            requires (!std::is_const_v<Tree>)
        {
            return static_cast<RB_Node<value_type>*>(i_hook);
        }
        [[nodiscard]] const RB_Node<value_type>* get_node_ptr() const
        {
            return static_cast<const RB_Node<value_type>*>(i_hook);
        }
        [[nodiscard]] reference operator*()
            requires (!std::is_const_v<Tree>)
        {
            return this->get_node_ptr()->n_data;
        }
        [[nodiscard]] const value_type& operator*() const
        {
            return this->get_node_ptr()->n_data;
        }
        bool is_root() const
        {
            return i_hook->is_root();
        }
        bool is_sentinel() const
        {
            return this->i_hook == i_tree->get_sentinel_ptr();
        }
        bool is_child() const
        {
            return i_hook->is_child();
        }
        bool is_left() const
        {
            return i_hook->is_left();
        }
        bool is_right() const
        {
            return i_hook->is_right();
        }
        bool is_red() const
        {
            return i_hook->is_red();
        }
        bool is_black() const
        {
            return i_hook->is_black();
        }
        bool is_leaf() const
        {
            return this->left_null() && this->right_null();
        }
        bool is_internal() const
        {
            return !is_leaf();
        }
        // [[nodiscard]]Hook* get_hook() &
        // {
        //     return this->i_hook;
        // }
        // [[nodiscard]]Hook* get_p_ptr() &
        // {
        //     return this->i_hook->get_p_ptr();
        // }
        //Traversing functions
        self_type& go_left()&
        {
            i_hook = i_hook->h_left;
            return *this;
        }
        self_type& go_right()&
        {
            i_hook = i_hook->h_right;
            return *this;
        }
        self_type& go_parent()&
        {
            i_hook = i_hook->get_p_ptr();
            return *this;
        }
        self_type get_sib()&
        {
            return (i_hook->is_left() ? self_type(i_tree, i_hook->get_p_ptr()->h_right) : self_type(i_tree,i_hook->get_p_ptr()->h_left) );
        }
        bool left_null() const
        {
            return this->i_hook->h_left == i_tree->get_sentinel_ptr();
        }
        bool right_null() const
        {
            return this->i_hook->h_right == i_tree->get_sentinel_ptr();
        }
        self_type& operator++()&
        {
            //++ call on iterator end() will loop back to the smallest element inside the tree
            if(this->is_sentinel())
            {
                this->go_left();
            }
            else 
            {
                auto temp = *this;
                if(!temp.right_null())
                {
                    temp.go_right();
                    while(!temp.left_null())
                    {
                        temp.go_left();
                    }
                }
                else
                {
                    while((!temp.is_root()) && temp.is_right())
                    {
                        temp.go_parent();
                    }
                    temp.go_parent();
                }
                this->i_hook = temp.get_hook();
            }
            return *this;
        }
        self_type& operator--()&
        {
            //if -- is called on iterator end(), it will go to the largest element
            if(this->is_sentinel())
            {
                this->go_right();
            }
            else
            {
                auto temp = *this;
                if(!temp.left_null())
                {
                    temp.go_left();
                    while(!temp.right_null())
                    {
                        temp.go_right();
                    }
                }
                else
                {
                    while((!temp.is_root()) && temp.is_left())
                    {
                        temp.go_parent();
                    }
                    temp.go_parent();
                }
                this->i_hook = temp.get_hook();
            }
            return *this;
        }
        size_type get_level()&
        {
            size_type level{0};
            hook_pointer temp = this->i_hook;
            while (temp != &(i_tree->m_sentinel))
            {
                temp = temp->get_p_ptr();
                ++level;
            }
            return level;
        }
    private:
        Tree* i_tree;
        hook_pointer i_hook;
        void tag_black()&
        {
            i_hook->tag_black();
        }
        void tag_red()&
        {
            i_hook->tag_red();
        }
        [[nodiscard]]hook_pointer get_hook() &
        {
            return this->i_hook;
        }
        [[nodiscard]]hook_pointer get_p_ptr() &
        {
            return this->i_hook->get_p_ptr();
        }
        friend RB_Tree<value_type>;
    };
}