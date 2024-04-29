#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H

#include <binary_search_tree.h>

template<typename tkey, typename tvalue>
class red_black_tree final:

    public binary_search_tree<tkey, tvalue>
{

public:

    enum class node_color
    {
        RED,
        BLACK
    };

private:

    struct node final:
        binary_search_tree<tkey, tvalue>::node
    {

    public:

        node_color _color;

    public:

        explicit node(
                tkey const &key,
                tvalue const &value) :
                    binary_search_tree<tkey, tvalue>::node(key, value),
                    _color(node_color::RED)
        {

        }

        explicit node(
                tkey const &key,
                tvalue const &&value) :
                    binary_search_tree<tkey, tvalue>::node(key, std::move(value)),
                    _color(node_color::RED)
        {

        }
    };

public:

    struct iterator_data final:
        public binary_search_tree<tkey, tvalue>::iterator_data
    {

    private:

        node_color _color;

    public:

        node_color get_color() const
        {
            if (this->is_state_initialized())
            {
                return _color;
            }
            else
            {
                std::logic_error("uninitialized iterator data");
            }
        }

    public:

        explicit iterator_data(
            unsigned int depth,
            tkey const &key,
            tvalue const &value,
            node_color color);
    };

private:

    friend class balancer;

    class balancer
    {

    public:

        void balance_after_insertion_method(
                std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path,
                red_black_tree<tkey, tvalue> const *rb_tree)
        {
            std::stack<typename binary_search_tree<tkey, tvalue>::node **> temp_path = path;

            while (!temp_path.empty())
            {
                typename binary_search_tree<tkey, tvalue>::node *base_node = *(temp_path.top());
                auto *node = static_cast<red_black_tree<tkey, tvalue>::node*>(base_node);

                std::string color = node->_color == red_black_tree<tkey, tvalue>::node_color::RED ? "RED" : "BLACK";

                std::cout << "Key: " << node->key << ", Value: " << node->value << " Color: " << color << std::endl;
                temp_path.pop();
            }

            if (path.empty())
            {
                return;
            }

            red_black_tree<tkey, tvalue>::node* son_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));

            if (path.size() == 1)
            {
                son_node->_color = node_color::BLACK;
                return;
            }

            path.pop();

            red_black_tree<tkey, tvalue>::node* parent_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
            red_black_tree<tkey, tvalue>::node** parent_node_ptr = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(path.top());

            path.pop();

            if (parent_node->_color == node_color::BLACK)
            {
                return;
            }

            red_black_tree<tkey, tvalue>::node* grandparent_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
            red_black_tree<tkey, tvalue>::node** grandparent_node_ptr = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(path.top());

            path.pop();

            red_black_tree<tkey, tvalue>::node *uncle_node;

            if (grandparent_node->left_subtree == parent_node)
            {
                uncle_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node*>(grandparent_node->right_subtree);

                if ((uncle_node == nullptr) || (uncle_node->_color == node_color::BLACK))
                {
                    if (parent_node->left_subtree == son_node)
                    {
                        rb_tree->small_right_rotation(
                                *reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(parent_node_ptr));

                        parent_node->_color = node_color::BLACK;
                        grandparent_node->_color = node_color::RED;
                    }
                    else
                    {
                        if (parent_node == rb_tree->_root)
                        {
                            rb_tree->small_left_rotation(
                                    *reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>((rb_tree->_root)));
                        }
                        else
                        {
                            rb_tree->small_left_rotation(
                                    *reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(parent_node_ptr));
                        }

                        parent_node->_color = node_color::BLACK;
                        grandparent_node->_color = node_color::RED;

                        if (grandparent_node == rb_tree->_root)
                        {
                            rb_tree->small_right_rotation(
                                    *reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>((rb_tree->_root)));
                        }
                        else
                        {
                            rb_tree->small_right_rotation(
                                    *reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(grandparent_node_ptr));
                        }
                    }
                }
                else
                {
                    grandparent_node->_color = node_color::RED;
                    parent_node->_color = node_color::BLACK;
                    uncle_node->_color = node_color::BLACK;
                    balance_after_insertion_method(path, rb_tree);
                }
            }
            else
            {
                uncle_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node*>(grandparent_node->left_subtree);

                if ((uncle_node == nullptr) || (uncle_node->_color == node_color::BLACK))
                {
                    if (parent_node->left_subtree == son_node)
                    {
                        rb_tree->small_left_rotation(
                                *reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(parent_node_ptr));

                        parent_node->_color = node_color::BLACK;
                        grandparent_node->_color = node_color::RED;
                    }
                    else
                    {
                        if (parent_node == rb_tree->_root)
                        {
                            rb_tree->small_right_rotation(
                                    *reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>((rb_tree->_root)));
                        }
                        else
                        {
                            rb_tree->small_right_rotation(
                                    *reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(parent_node_ptr));
                        }

                        parent_node->_color = node_color::BLACK;
                        grandparent_node->_color = node_color::RED;

                        if (grandparent_node == rb_tree->_root)
                        {
                            rb_tree->small_left_rotation(
                                    *reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>((rb_tree->_root)));
                        }
                        else
                        {
                            rb_tree->small_left_rotation(
                                    *reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(grandparent_node_ptr));
                        }
                    }
                    balance_after_insertion_method(path, rb_tree);
                }
                else
                {
                    grandparent_node->_color = node_color::RED;
                    parent_node->_color = node_color::BLACK;
                    uncle_node->_color = node_color::BLACK;
                    balance_after_insertion_method(path, rb_tree);
                }
            }
        }

//        void balance_after_disposal_method(
//                std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path,
//                typename binary_search_tree<tkey, tvalue>::node* node_to_dispose)
//        {
//            if (path.empty())
//            {
//                return;
//            }
//
//            if (node_to_dispose->left_subtree != nullptr && node_to_dispose->right_subtree != nullptr)
//            {
//                if (get_color(node_to_dispose) == node_color::RED)
//                {
//                    return;
//                }
//            }
//            else if (node_to_dispose->left_subtree != nullptr || node_to_dispose->right_subtree != nullptr)
//            {
//
//            }
//            else
//            {
//
//            }
//        }

    };

    class insertion_template_method final:
        public binary_search_tree<tkey, tvalue>::insertion_template_method,
        public balancer
    {

    public:

        explicit insertion_template_method(
            red_black_tree<tkey, tvalue> *tree,
            typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy);


    private:

        void balance(std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
            this->balance_after_insertion_method(path, dynamic_cast<red_black_tree<tkey, tvalue> const *>(this->_tree));
        }
    };

    class disposal_template_method final:
        public binary_search_tree<tkey, tvalue>::disposal_template_method
    {

    public:

        explicit disposal_template_method(
            red_black_tree<tkey, tvalue> *tree,
            typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy);

    private:

        void balance(
                std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {

        }

    };

public:

    explicit red_black_tree(
            std::function<int(tkey const &, tkey const &)> comparer,
            allocator* allocator = nullptr,
            logger *logger = nullptr,
            typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy =
                binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy::throw_an_exception,
            typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy =
                binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy::throw_an_exception);

public:

    ~red_black_tree() noexcept final;

    red_black_tree(red_black_tree<tkey, tvalue> &other);

    red_black_tree<tkey, tvalue> &operator=(red_black_tree<tkey, tvalue> const &other);

    red_black_tree(red_black_tree<tkey, tvalue> &&other) noexcept;

    red_black_tree<tkey, tvalue> &operator=(red_black_tree<tkey, tvalue> &&other) noexcept;


private:

    static void set_color(
            typename binary_search_tree<tkey,tvalue>::node* node,
            node_color color) noexcept
    {
        dynamic_cast<red_black_tree<tkey, tvalue>::node*>(node)->_color = color;
    }

    static typename red_black_tree<tkey, tvalue>::node_color get_color(
            typename binary_search_tree<tkey,tvalue>::node* node) noexcept
    {
        auto* rb_tree_node = dynamic_cast<red_black_tree<tkey, tvalue>::node*>(node);

        if (rb_tree_node != nullptr)
        {
            return rb_tree_node->_color;
        }

        return node_color::BLACK;
    }
};

template<typename tkey, typename tvalue>
red_black_tree<tkey, tvalue>::iterator_data::iterator_data(
    unsigned int depth,
    tkey const &key,
    tvalue const &value,
    typename red_black_tree<tkey, tvalue>::node_color color):
        binary_search_tree<tkey, tvalue>::iterator_data(depth, key, value)
{
    this->_color = color;
}

template<typename tkey, typename tvalue>
red_black_tree<tkey, tvalue>::insertion_template_method::insertion_template_method(
    red_black_tree<tkey, tvalue> *tree,
    typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy):
        binary_search_tree<tkey, tvalue>::insertion_template_method(tree, insertion_strategy)
{

}

template<typename tkey,typename tvalue>
red_black_tree<tkey, tvalue>::disposal_template_method::disposal_template_method(
    red_black_tree<tkey, tvalue> *tree,
    typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy) :
        binary_search_tree<tkey, tvalue>::disposal_template_method(tree, disposal_strategy)
{

}

template<typename tkey,typename tvalue>
red_black_tree<tkey, tvalue>::red_black_tree(
        std::function<int(tkey const &, tkey const &)> comparer,
        allocator* allocator,
        logger *logger,
        typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy,
        typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy) :

        binary_search_tree<tkey, tvalue>(new red_black_tree<tkey, tvalue>::insertion_template_method(this, insertion_strategy),
                                         new typename binary_search_tree<tkey, tvalue>::obtaining_template_method(dynamic_cast<binary_search_tree<tkey, tvalue> *>(this)),
                                         new red_black_tree<tkey, tvalue>::disposal_template_method(this, disposal_strategy), comparer, allocator, logger)
{

}


template<typename tkey,typename tvalue>
red_black_tree<tkey, tvalue>::~red_black_tree() noexcept
{

}

template<typename tkey,typename tvalue>
red_black_tree<tkey, tvalue>::red_black_tree(red_black_tree<tkey, tvalue> &other)
{

}

template<typename tkey,typename tvalue>
red_black_tree<tkey, tvalue> &red_black_tree<tkey, tvalue>::operator=(
    red_black_tree<tkey, tvalue> const &other)
{
    if (this != &other)
    {
        clear(this->_root);
        this->_allocator = other._allocator;
        this->_logger = other._logger;
        this->_keys_comparer = other._keys_comparer;
        this->_root = copy(other._root);
    }

    return *this;
}

template<typename tkey,typename tvalue>
red_black_tree<tkey, tvalue>::red_black_tree(red_black_tree<tkey, tvalue> &&other) noexcept :
        red_black_tree<tkey, tvalue>(other._keys_comparer, other.get_allocator(), other.get_logger())
{
    this->_insertion_template = other._insertion_template;
    other._insertion_template = nullptr;

    this->_obtaining_template = other._obtaining_template;
    other._obtaining_template = nullptr;

    this->_disposal_template = other._disposal_template;
    other._disposal_template = nullptr;

    this->_root = other._root;
    other._root = nullptr;

}

template<typename tkey,typename tvalue>
red_black_tree<tkey, tvalue> &red_black_tree<tkey, tvalue>::operator=(red_black_tree<tkey, tvalue> &&other) noexcept
{
    if (this != &other)
    {
        clear(this->_root);

        this->_allocator = other._allocator;
        other._allocator = nullptr;

        this->_logger = other._logger;
        other._logger = nullptr;

        this->_keys_comparer = other._keys_comparer;
        other._keys_comparer = nullptr;

        this->_root = other._root;
        other._root = nullptr;
    }

    return *this;
}

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H
