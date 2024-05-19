#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H

#include <binary_search_tree.h>
#include <logger_guardant.h>

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

public:

    struct node final:
            binary_search_tree<tkey, tvalue>::node
    {

    public:

        node_color color;

    public:

        explicit node(
                tkey const &key,
                tvalue const &value):
                binary_search_tree<tkey, tvalue>::node(key, value),
                color(node_color::RED)
        {

        }

        explicit node(
                tkey const &key,
                tvalue &&value):
                binary_search_tree<tkey, tvalue>::node(key, std::move(value)),
                color(node_color::RED)
        {

        }


    };

public:

    struct iterator_data final:
            public binary_search_tree<tkey, tvalue>::iterator_data
    {

    public:

        node_color _color;

    public:

        node_color get_color() const
        {
            if (this->_is_state_initialized)
            {
                return _color;
            }

            throw std::logic_error("tried to read color from uninitialized iterator data");
        }

    public:

        explicit iterator_data(
                unsigned int depth,
                tkey const &key,
                tvalue const &value,
                node_color color) :
                binary_search_tree<tkey, tvalue>::iterator_data(depth, key, value),
                _color(color)
        {

        }
    };

private:

    friend class balancer;

    class balancer
    {

    public:

        void balance_method_after_insert(std::stack<typename binary_search_tree<tkey, tvalue>::node**> &path,
                                         red_black_tree<tkey, tvalue> const *rb_tree)
        {
            std::cout << "balance after insert method has started" << std::endl;

            if (path.empty())
            {
                std::cout << "stack is empty" << std::endl;
                return;
            }

            auto *son_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));

            if (son_node == rb_tree->_root)
            {
                son_node->color = node_color::BLACK;
                std::cout << "disposed node is root" << std::endl;
                return;
            }

            path.pop();

            auto *parent_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node*>(*(path.top()));
            auto **parent_node_ptr = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node**>(path.top());

            if (parent_node->color == node_color::BLACK)
            {
                son_node->color = node_color::RED;
                std::string color_n = (son_node->color == node_color::RED) ? "RED" : "BLACK";
                std::cout << "parent node has black color, " << "son color: " << color_n << std::endl;
                return;
            }

            path.pop();

            auto *grandparent_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node*>(*(path.top()));
            auto **grandparent_node_ptr = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node**>(path.top());

            red_black_tree<tkey, tvalue>::node* uncle_node;

            if (grandparent_node->right_subtree == parent_node)
            {
                std::cout << "uncle is left subtree" << std::endl;

                uncle_node = reinterpret_cast<red_black_tree<tkey, tvalue>::node*>(grandparent_node->left_subtree);

                if (uncle_node && get_color(uncle_node) == node_color::RED)
                {
                    std::cout << "uncle node is left subtree and red color" << std::endl;

                    uncle_node->color = node_color::BLACK;
                    parent_node->color = node_color::BLACK;
                    grandparent_node->color = node_color::RED;

                    balance_method_after_insert(path, rb_tree);
                }
                else
                {
                    std::cout << "uncle node is left subtree and black color" << std::endl;

                    if (parent_node->left_subtree == son_node)
                    {
                        std::cout << "son node is left subtree: small right rotate" << std::endl;
                        rb_tree->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_node_ptr));
                    }
                    else
                    {
                        std::cout << "son node is right rotate: small left rotate" << std::endl;
                        rb_tree->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(grandparent_node_ptr));
                    }

                    balance_method_after_insert(path, rb_tree);
                }
            }
            else
            {
                std::cout << "uncle is right subtree" << std::endl;

                uncle_node = reinterpret_cast<red_black_tree<tkey, tvalue>::node*>(grandparent_node->right_subtree);

                std::string color = (get_color(uncle_node) == node_color::RED) ? "RED" : "BLACK";
                std::cout << color << std::endl;

                if (uncle_node && get_color(uncle_node) == node_color::RED)
                {
                    std::cout << "uncle node is right subtree and red color" << std::endl;

                    parent_node->color = node_color::BLACK;
                    uncle_node->color = node_color::BLACK;
                    grandparent_node->color = node_color::RED;

                    balance_method_after_insert(path, rb_tree);
                }
                else
                {
                    std::cout << "uncle node is right subtree and black color" << std::endl;

                    if (parent_node->left_subtree == son_node)
                    {
                        std::cout << "son node is left subtree" << std::endl;

                        rb_tree->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(grandparent_node_ptr));
                    }
                    else
                    {
                        std::cout << "son node is right subtree" << std::endl;

                        rb_tree->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_node_ptr));
                    }
                }
            }
        }

        void balance_method_after_dispose(std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path, red_black_tree<tkey, tvalue> const *tree)
        {
            std::cout << "start balance method after dispose" << std::endl;

            if (path.empty())
            {
                std::cout << "stack is empty" << std::endl;
                return;
            }

            red_black_tree<tkey, tvalue>::node *parent_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
            red_black_tree<tkey, tvalue>::node **parent_node_ptr = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(path.top());

            bool left_subtree = false;

            if (parent_node == nullptr || parent_node->color == node_color::RED)
            {
                std::cout << "only one node or don't need a balance" << std::endl;
                return;
            }

            red_black_tree<tkey, tvalue>::node *brother_node;
            red_black_tree<tkey, tvalue>::node **brother_node_ptr;

            if (parent_node->left_subtree != nullptr)
            {
                brother_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(parent_node->left_subtree);
                brother_node_ptr = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(&(parent_node->left_subtree));
                left_subtree = false;
            }
            else
            {
                brother_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(parent_node->right_subtree);
                brother_node_ptr = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(&(parent_node->right_subtree));
                left_subtree = true;
            }

            if (brother_node->color == node_color::BLACK)
            {
                red_black_tree<tkey, tvalue>::node *left_child_to_brother = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(brother_node->left_subtree);
                red_black_tree<tkey, tvalue>::node *right_child_to_brother = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(brother_node->right_subtree);

                if (left_child_to_brother != nullptr && left_child_to_brother->color == node_color::RED ||
                    right_child_to_brother != nullptr && right_child_to_brother->color == node_color::RED)
                {
                    if (left_subtree)
                    {
                        if (right_child_to_brother != nullptr && right_child_to_brother->color == node_color::RED)
                        {
                            brother_node->color = parent_node->color;
                            parent_node->color = node_color::BLACK;
                            right_child_to_brother->color = node_color::BLACK;

                            tree->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_node_ptr));
                            return;
                        }
                        else if (left_child_to_brother != nullptr && left_child_to_brother->color == node_color::RED &&
                                 right_child_to_brother && right_child_to_brother->color == node_color::BLACK)
                        {
                            left_child_to_brother->color = node_color::BLACK;
                            brother_node->color = node_color::RED;

                            tree->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(brother_node_ptr));
                            balance_method_after_dispose(path, tree);
                        }
                    }
                    else
                    {
                        if (left_child_to_brother != nullptr && left_child_to_brother->color == node_color::RED)
                        {
                            brother_node->color = parent_node->color;
                            parent_node->color = node_color::BLACK;
                            left_child_to_brother->color = node_color::BLACK;
                            tree->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_node_ptr));
                            return;
                        }
                        else if (right_child_to_brother != nullptr && right_child_to_brother->color == node_color::RED &&
                                 left_child_to_brother && left_child_to_brother->color == node_color::BLACK)
                        {
                            right_child_to_brother->color = node_color::BLACK;
                            brother_node->color = node_color::RED;
                            tree->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(brother_node_ptr));
                            balance_method_after_dispose(path, tree);
                        }
                    }
                }
                else
                {
                    brother_node->color = node_color::RED;

                    if (parent_node->color == node_color::RED)
                    {
                        parent_node->color = node_color::BLACK;
                        return;
                    }
                    else
                    {
                        parent_node->color = node_color::BLACK;
                        if (left_subtree)
                        {
                            path.pop();
                            red_black_tree<tkey, tvalue>::node *grandparent_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
                            if (grandparent_node && grandparent_node->left_subtree == parent_node)
                            {
                                balance_method_after_dispose(path, tree);
                            }
                            else if (grandparent_node)
                            {
                                balance_method_after_dispose(path, tree);
                            }
                        }
                        else
                        {
                            path.pop();
                            red_black_tree<tkey, tvalue>::node *grandpa = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
                            if (grandpa && grandpa->right_subtree == parent_node)
                            {
                                balance_method_after_dispose(path, tree);
                            }
                            else if (grandpa)
                            {
                                balance_method_after_dispose(path, tree);
                            }
                        }
                    }
                }
            }
            else
            {
                parent_node->color = node_color::RED;
                brother_node->color = node_color::BLACK;

                if (left_subtree)
                {
                    tree->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_node_ptr));
                    balance_method_after_dispose(path, tree);
                }
                else
                {
                    tree->small_right_rotation((*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_node_ptr)));
                    balance_method_after_dispose(path, tree);
                }
            }
        }
    };


private:

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
            this->balance_method_after_insert(path, dynamic_cast<red_black_tree<tkey, tvalue> const *>(this->_tree));
        }

    };

    class disposal_template_method final:
            public binary_search_tree<tkey, tvalue>::disposal_template_method,
            public balancer
    {

    public:

        explicit disposal_template_method(
                red_black_tree<tkey, tvalue> *tree,
                typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy);
    private:

        void balance(std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
            path.pop();

            this->balance_method_after_dispose(path, dynamic_cast<red_black_tree<tkey, tvalue> const *>(this->_tree));
        }

        template<typename T>
        inline void swap(T &&one, T &&another)
        {
            T temp = std::move(one);
            one = std::move(another);
            another = std::move(temp);
        }

    public:

        tvalue dispose(tkey const &key) override
        {
            auto path = this->find_path(key);

            if (*(path.top()) == nullptr)
            {
                std::cout << "case path is empty" << std::endl;
                return tvalue();
            }

            tvalue value = (*(path.top()))->value;

            if ((*(path.top()))->left_subtree != nullptr && (*(path.top()))->right_subtree != nullptr)
            {
                std::cout << "node has 2 subtrees" << std::endl;

                auto *dispose_node = *(path.top());

                auto **max_left_subtree = reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(&((*(path.top()))->left_subtree));

                while (*max_left_subtree != nullptr)
                {
                    path.push(max_left_subtree);
                    max_left_subtree = reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(&((*max_left_subtree)->right_subtree));
                }

                swap(std::move(dispose_node->key), std::move((*(path.top()))->key));
                swap(std::move(dispose_node->value), std::move((*(path.top()))->value));

            }

            if ((*path.top())->left_subtree != nullptr || (*path.top())->right_subtree != nullptr)
            {
                std::cout << "node has 1 subtree" << std::endl;

                typename binary_search_tree<tkey, tvalue>::node** subtree;

                subtree = ((*path.top())->right_subtree == nullptr) ? &(*path.top())->left_subtree : &(*path.top())->right_subtree;

                swap(std::move((*subtree)->key), std::move((*(path.top()))->key));
                swap(std::move((*subtree)->value), std::move((*(path.top()))->value));

                path.push(subtree);
            }

            if ((*path.top())->left_subtree == nullptr && (*path.top())->right_subtree == nullptr)
            {
                std::cout << "node is without subtrees" << std::endl;

                red_black_tree<tkey, tvalue>::node* current_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));

                if (current_node->color == node_color::BLACK)
                {
                    std::cout << "dispose node is black" << std::endl;

                    allocator::destruct(*(path.top()));
                    this->deallocate_with_guard(*(path.top()));

                    *(path.top()) = nullptr;

                    this->balance(path);

                    return value;
                }
                else
                {
                    std::cout << "dispose node is red" << std::endl;

                    allocator::destruct(*(path.top()));
                    this->deallocate_with_guard(*(path.top()));

                    *(path.top()) = nullptr;

                    return value;
                }
            }
        }
//
//    private:
//
//        inline std::string get_typename() const noexcept override
//        {
//            return "RED_BLACK_TREE: ";
//        }

    };

public:

    explicit red_black_tree(
            std::function<int(tkey const &, tkey const &)> comparer = associative_container<tkey, tvalue>::default_key_comparer(),
            allocator *allocator = nullptr,
            logger *logger = nullptr,
            typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy = binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy::throw_an_exception,
            typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy = binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy::throw_an_exception);

public:

    ~red_black_tree() noexcept final;

    red_black_tree(red_black_tree<tkey, tvalue> const &other);

    red_black_tree<tkey, tvalue> &operator=(red_black_tree<tkey, tvalue> const &other);

    red_black_tree(red_black_tree<tkey, tvalue> &&other) noexcept;

    red_black_tree<tkey, tvalue> &operator=(red_black_tree<tkey, tvalue> &&other) noexcept;

public:

    static node_color get_color(red_black_tree<tkey, tvalue>::node* current_node) noexcept
    {
        if (current_node == nullptr)
        {
            return node_color::BLACK;
        }
        else if (current_node->color == node_color::RED)
        {
            return node_color::RED;
        }

        return node_color::BLACK;

    }

    static node_color set_color(red_black_tree<tkey, tvalue>::node* current_node) noexcept
    {
        if (get_color(current_node) == node_color::RED)
        {
            return node_color::BLACK;
        }

        return node_color::RED;
    }

protected:

    size_t get_node_size() const noexcept
    {
        return sizeof(typename red_black_tree<tkey, tvalue>::node);
    }

//    void call_node_constructor(
//            node *raw_space,
//            tkey const &key,
//            tvalue const &value) override
//    {
//        allocator::construct(reinterpret_cast<typename red_black_tree<tkey, tvalue>::node*>(raw_space), key, value);
//    }
//
//     void call_node_constructor(
//            node *raw_space,
//            tkey const &key,
//            tvalue &&value) override
//    {
//        allocator::construct(reinterpret_cast<typename red_black_tree<tkey, tvalue>::node*>(raw_space), key, std::move(value));
//    }
//
//     void inject_additional_data(
//            iterator_data *destination,
//            node *source) override
//    {
//        auto *rbt_destination = dynamic_cast<red_black_tree<tkey, tvalue>::node*>(destination);
//        auto *rbt_source = dynamic_cast<red_black_tree<tkey, tvalue>::node const*>(source);
//
//        rbt_destination->color = rbt_source->color;
//    }


};

//template<typename tkey, typename tvalue>
//red_black_tree<tkey, tvalue>::iterator_data::iterator_data(
//        unsigned int depth,
//        tkey const &key,
//        tvalue const &value,
//        typename red_black_tree<tkey, tvalue>::node_color color):
//            binary_search_tree<tkey, tvalue>::iterator_data(depth, key, value)
//{
//    this->_color = color;
//}

template<typename tkey, typename tvalue>
red_black_tree<tkey, tvalue>::insertion_template_method::insertion_template_method(
        red_black_tree<tkey, tvalue> *tree,
        typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy):
        binary_search_tree<tkey, tvalue>::insertion_template_method(tree, insertion_strategy)
{

}

template<typename tkey, typename tvalue>
red_black_tree<tkey, tvalue>::disposal_template_method::disposal_template_method(
        red_black_tree<tkey, tvalue> *tree,
        typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy):
        binary_search_tree<tkey, tvalue>::disposal_template_method(tree, disposal_strategy)
{

}

template<typename tkey, typename tvalue>
red_black_tree<tkey, tvalue>::red_black_tree(
        std::function<int(tkey const &, tkey const &)> comparer,
        allocator *allocator,
        logger *logger,
        typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy,
        typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy):
        binary_search_tree<tkey, tvalue>(new red_black_tree<tkey, tvalue>::insertion_template_method(this, insertion_strategy),
                                         new typename binary_search_tree<tkey, tvalue>::obtaining_template_method(dynamic_cast<binary_search_tree<tkey, tvalue> *>(this)),
                                         new red_black_tree<tkey, tvalue>::disposal_template_method(this, disposal_strategy), comparer, allocator, logger)

{
    if (this->_insertion_template == nullptr || this->_obtaining_template == nullptr || this->_disposal_template == nullptr)
    {
        delete this->_insertion_template;
        delete this->_obtaining_template;
        delete this->_disposal_template;

        throw std::bad_alloc();
    }

}

template<typename tkey, typename tvalue>
red_black_tree<tkey, tvalue>::~red_black_tree() noexcept
{

}

template<typename tkey, typename tvalue>
red_black_tree<tkey, tvalue>::red_black_tree(red_black_tree<tkey, tvalue> const &other) :
        red_black_tree<tkey, tvalue>(other._keys_comparer, other.get_allocator(), other.get_logger(),
                                     binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy::throw_an_exception,
                                     binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy::throw_an_exception)
{

}

template<typename tkey, typename tvalue>
red_black_tree<tkey, tvalue> &red_black_tree<tkey, tvalue>::operator=(red_black_tree<tkey, tvalue> const &other)
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

template<typename tkey, typename tvalue>
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

template<typename tkey, typename tvalue>
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

        this->_root =other._root;
        other._root = nullptr;
    }

    return *this;
}

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H