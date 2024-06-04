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

        node_color _color;

    public:

        explicit node(
                tkey const &key,
                tvalue const &value):
                    binary_search_tree<tkey, tvalue>::node(key, value),
                    _color(node_color::RED)
        {

        }

        explicit node(
                tkey const &key,
                tvalue &&value):
                    binary_search_tree<tkey, tvalue>::node(key, std::move(value)),
                    _color(node_color::RED)
        {

        }

        ~node() noexcept override = default;

    public:

        void set_color(node_color new_color)
        {
            _color = new_color;
        }

    };

public:

    struct iterator_data final:
            public binary_search_tree<tkey, tvalue>::iterator_data
    {

    public:

        node_color color;

    public:

        node_color get_color() const
        {
            if (this->is_state_initialized())
            {
                return color;
            }

            throw std::logic_error("tried to read color from uninitialized iterator data");
        }

    public:

        iterator_data();

    public:

        explicit iterator_data(
                unsigned int depth,
                tkey const &key,
                tvalue const &value,
                node_color color) :
                    binary_search_tree<tkey, tvalue>::iterator_data(depth, key, value),
                    color(color)
        {

        }

        explicit iterator_data(
                unsigned int depth,
                node *src_node) :
                    binary_search_tree<tkey, tvalue>::iterator_data(
                            depth,reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node*>(src_node))
        {
            if(src_node != nullptr)
            {
                color = src_node->_color;
            }
        }

    public:

        iterator_data(
                iterator_data const &other) :
                    binary_search_tree<tkey, tvalue>::iterator_data(other),
                    color(other._color)
        {

        }

        iterator_data(
                iterator_data &&other) noexcept :
                    binary_search_tree<tkey, tvalue>::iterator_data(std::move(other)),
                    color(std::move(other._color))
        {

        }

        iterator_data &operator=(
                iterator_data const &other)
        {
            if (this != &other)
            {
                binary_search_tree<tkey, tvalue>::iterator_data::operator=(other);
                color = other._color;
            }

            return *this;
        }

        iterator_data &operator=(
                iterator_data &&other) noexcept
        {
            if (this != &other)
            {
                binary_search_tree<tkey, tvalue>::iterator_data::operator=(std::move(other));
                color = std::move(other._color);
            }

            return *this;
        }

        ~iterator_data() noexcept final = default;
    };

private:

    class insertion_template_method final:
            public binary_search_tree<tkey, tvalue>::insertion_template_method
    {

    public:

        explicit insertion_template_method(
                red_black_tree<tkey, tvalue> *tree,
                typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy);

    private:

        void balance(std::stack<typename binary_search_tree<tkey, tvalue>::node**> &path) override
        {
            if (path.empty())
            {
                return;
            }

            auto *son_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));

            if (path.size() == 1)
            {
                son_node->color = node_color::BLACK;
                return;
            }

            path.pop();

            auto *parent_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node*>(*(path.top()));
            auto **parent_node_ptr = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node**>(path.top());

            if (parent_node->color == node_color::BLACK)
            {
                son_node->color = node_color::RED;
                return;
            }

            path.pop();

            auto *grandparent_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node*>(*(path.top()));
            auto **grandparent_node_ptr = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node**>(path.top());

            red_black_tree<tkey, tvalue>::node* uncle_node;

            if (grandparent_node->right_subtree == parent_node)
            {
                uncle_node = reinterpret_cast<red_black_tree<tkey, tvalue>::node*>(grandparent_node->left_subtree);

                if (uncle_node && get_color(uncle_node) == node_color::RED)
                {
                    uncle_node->color = node_color::BLACK;
                    parent_node->color = node_color::BLACK;
                    grandparent_node->color = node_color::RED;

                    balance(path);
                }
                else
                {
                    if (parent_node->left_subtree == son_node)
                    {
                        dynamic_cast<red_black_tree<tkey, tvalue>*>(this->_tree)->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_node_ptr));
                    }
                    else
                    {
                        dynamic_cast<red_black_tree<tkey, tvalue>*>(this->_tree)->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(grandparent_node_ptr));
                    }

                    balance(path);
                }
            }
            else
            {
                uncle_node = reinterpret_cast<red_black_tree<tkey, tvalue>::node*>(grandparent_node->right_subtree);
                if (uncle_node && get_color(uncle_node) == node_color::RED)
                {
                    parent_node->color = node_color::BLACK;
                    uncle_node->color = node_color::BLACK;
                    grandparent_node->color = node_color::RED;

                    balance(path);
                }
                else
                {
                    if (parent_node->left_subtree == son_node)
                    {
                        dynamic_cast<red_black_tree<tkey, tvalue>*>(this->_tree)->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(grandparent_node_ptr));
                    }
                    else
                    {
                        dynamic_cast<red_black_tree<tkey, tvalue>*>(this->_tree)->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_node_ptr));
                    }
                }
            }
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

        void balance(std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
            path.pop();

            if (path.empty())
            {
                return;
            }

            red_black_tree<tkey, tvalue>::node *parent_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
            red_black_tree<tkey, tvalue>::node **parent_node_ptr = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(path.top());

            bool left_subtree = false;

            if (parent_node == nullptr || parent_node->color == node_color::RED)
            {
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

                            dynamic_cast<red_black_tree<tkey, tvalue>*>(this->_tree)->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_node_ptr));
                            return;
                        }
                        else if (left_child_to_brother != nullptr && left_child_to_brother->color == node_color::RED &&
                                 right_child_to_brother && right_child_to_brother->color == node_color::BLACK)
                        {
                            left_child_to_brother->color = node_color::BLACK;
                            brother_node->color = node_color::RED;

                            dynamic_cast<red_black_tree<tkey, tvalue>*>(this->_tree)->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(brother_node_ptr));
                            this->balance(path);
                        }
                    }
                    else
                    {
                        if (left_child_to_brother != nullptr && left_child_to_brother->color == node_color::RED)
                        {
                            brother_node->color = parent_node->color;
                            parent_node->color = node_color::BLACK;
                            left_child_to_brother->color = node_color::BLACK;
                            dynamic_cast<red_black_tree<tkey, tvalue>*>(this->_tree)->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_node_ptr));
                            return;
                        }
                        else if (right_child_to_brother != nullptr && right_child_to_brother->color == node_color::RED &&
                                 left_child_to_brother && left_child_to_brother->color == node_color::BLACK)
                        {
                            right_child_to_brother->color = node_color::BLACK;
                            brother_node->color = node_color::RED;
                            dynamic_cast<red_black_tree<tkey, tvalue>*>(this->_tree)->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(brother_node_ptr));
                            this->balance(path);
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
                                this->balance(path);
                            }
                            else if (grandparent_node)
                            {
                                this->balance(path);
                            }
                        }
                        else
                        {
                            path.pop();
                            red_black_tree<tkey, tvalue>::node *grandpa = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
                            if (grandpa && grandpa->right_subtree == parent_node)
                            {
                                this->balance(path);
                            }
                            else if (grandpa)
                            {
                                this->balance(path);
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
                    dynamic_cast<red_black_tree<tkey, tvalue>*>(this->_tree)->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_node_ptr));
                    this->balance(path);
                }
                else
                {
                    dynamic_cast<red_black_tree<tkey, tvalue>*>(this->_tree)->small_right_rotation((*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_node_ptr)));
                    this->balance(path);
                }
            }
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
                return tvalue();
            }

            if ((*(path.top()))->left_subtree != nullptr && (*(path.top()))->right_subtree != nullptr)
            {
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

            tvalue value = (*(path.top()))->value;

            if ((*path.top())->left_subtree != nullptr || (*path.top())->right_subtree != nullptr)
            {
                typename binary_search_tree<tkey, tvalue>::node** subtree;

                subtree = ((*path.top())->right_subtree == nullptr) ? &(*path.top())->left_subtree : &(*path.top())->right_subtree;

                swap(std::move((*subtree)->key), std::move((*(path.top()))->key));
                swap(std::move((*subtree)->value), std::move((*(path.top()))->value));

                path.push(subtree);
            }

            if ((*path.top())->left_subtree == nullptr && (*path.top())->right_subtree == nullptr)
            {
                red_black_tree<tkey, tvalue>::node* current_node = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));

                if (current_node->color == node_color::BLACK)
                {
                    allocator::destruct(*(path.top()));
                    this->deallocate_with_guard(*(path.top()));

                    *(path.top()) = nullptr;

                    this->balance(path);

                    return value;
                }
                else
                {
                    allocator::destruct(*(path.top()));
                    this->deallocate_with_guard(*(path.top()));

                    *(path.top()) = nullptr;

                    return value;
                }
            }
        }
    };

public:

    explicit red_black_tree(
            std::function<int(tkey const &, tkey const &)> comparer = associative_container<tkey, tvalue>::default_key_comparer(),
            allocator *allocator = nullptr,
            logger *logger = nullptr,
            typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy = binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy::throw_an_exception,
            typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy = binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy::throw_an_exception);

public:

    ~red_black_tree() noexcept final
    {

    }

    red_black_tree(red_black_tree<tkey, tvalue> const &other) :
            red_black_tree<tkey, tvalue>(other._keys_comparer, other.get_allocator(), other.get_logger(),
                                         binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy::throw_an_exception,
                                         binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy::throw_an_exception)
    {

    }

    red_black_tree<tkey, tvalue> &operator=(red_black_tree<tkey, tvalue> const &other)
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

    red_black_tree(red_black_tree<tkey, tvalue> &&other) noexcept :
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

    red_black_tree<tkey, tvalue> &operator=(red_black_tree<tkey, tvalue> &&other) noexcept
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

protected:

    size_t get_node_size() const noexcept
    {
        return sizeof(typename red_black_tree<tkey, tvalue>::node);
    }

    void call_node_constructor(
            node *raw_space,
            tkey const &key,
            tvalue const &value) override
    {
        allocator::construct(reinterpret_cast<typename red_black_tree<tkey, tvalue>::node*>(raw_space), key, value);
    }

     void call_node_constructor(
            node *raw_space,
            tkey const &key,
            tvalue &&value) override
    {
        allocator::construct(reinterpret_cast<typename red_black_tree<tkey, tvalue>::node*>(raw_space), key, std::move(value));
    }

     void inject_additional_data(
            iterator_data *destination,
            node *source) override
    {
        auto *rbt_destination = dynamic_cast<red_black_tree<tkey, tvalue>::iterator_data*>(destination);
        auto *rbt_source = dynamic_cast<red_black_tree<tkey, tvalue>::node const*>(source);

        rbt_destination->color = rbt_source->color;
    }
};

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

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H