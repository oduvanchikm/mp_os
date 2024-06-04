#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_AVL_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_AVL_TREE_H
#include <binary_search_tree.h>

template<typename tkey, typename tvalue>
class AVL_tree final:
        public binary_search_tree<tkey, tvalue>
{

private:

    struct node final:
            binary_search_tree<tkey, tvalue>::node
    {

    public:

        size_t _subtree_height;

    public:

        explicit node(
                tkey const &key,
                tvalue const &value):
                    binary_search_tree<tkey, tvalue>::node(key, value),
                    _subtree_height(1)
        {

        }

        explicit node(
                tkey const &key,
                tvalue &&value):
                    binary_search_tree<tkey, tvalue>::node(key, std::move(value)),
                    _subtree_height(1)
        {

        }

        size_t get_height()
        {
            return _subtree_height;
        }

        void set_height(size_t subtree_height)
        {
            _subtree_height = subtree_height;
        }

    };

public:

    struct iterator_data final:
            public binary_search_tree<tkey, tvalue>::iterator_data
    {

    public:

        size_t subtree_height;

    public:

        explicit iterator_data(
                unsigned int depth,
                tkey const &key,
                tvalue const &value,
                size_t subtree_height) :
                    binary_search_tree<tkey, tvalue>::iterator_data(depth, key, value)
        {
            this->subtree_height = subtree_height;
        }


    public:

        iterator_data(AVL_tree<tkey, tvalue>::iterator_data const &other) :
            iterator_data(other.depth, other.get_key(), other.get_value(), other.subtree_height)
        {

        }

        iterator_data &operator=(AVL_tree<tkey, tvalue>::iterator_data const &other)
        {
            if (this != &other)
            {
                if (this->is_state_initialized())
                {
                    this->_key = other.key;
                    this->_value = other.value;
                }
                else
                {
                    allocator::construct(this->_key, other._key);
                    allocator::construct(this->_value, other._value);
                    this->_is_state_initialized = true;
                }

                this->depth = other.depth;
                this->subtree_height = other.subtree_height;
            }

            return *this;
        }

        iterator_data(AVL_tree<tkey, tvalue>::iterator_data &&other) noexcept :
            iterator_data(other.depth, other.get_key(), other.get_value(), other.subtree_height)
        {
            allocator::destruct(other._key);
            allocator::destruct(other._value);

            other._is_state_initialized = false;
            this->_is_state_initialized = true;
        }

        iterator_data &operator=(AVL_tree<tkey, tvalue>::iterator_data &&other) noexcept
        {
            if (this != &other)
            {
                if (this->_is_state_initialized)
                {
                    _key(std::move(other.key));
                    _value(std::move(other.value));
                }
                else
                {
                    allocator::construct(this->_key, std::move(other._key));
                    allocator::construct(this->_value, std::move(other._value));

                    this->_is_state_initialized = true;
                }

                this->depth = other.depth;
                this->subtree_height = other.subtree_height;

                other.is_state_initialized = false;
            }

            return *this;
        }

        virtual ~iterator_data() noexcept = default;
    };

private:

    class balancer_for_avl_tree
    {

    public:

        int get_subtree_height(typename binary_search_tree<tkey, tvalue>::node *node) noexcept
        {
            auto avl_tree = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(node);

            if (avl_tree == nullptr)
            {
                return 0;
            }

            return avl_tree->_subtree_height;
        }

        int get_balance_node(typename AVL_tree<tkey, tvalue>::node *node_tree)
        {
            if (node_tree->left_subtree != nullptr && node_tree->right_subtree != nullptr)
            {
                return get_subtree_height(node_tree->left_subtree) - get_subtree_height(node_tree->right_subtree);
            }
            else if (node_tree->left_subtree == nullptr && node_tree->right_subtree != nullptr)
            {
                return -get_subtree_height(node_tree->right_subtree);
            }
            else if (node_tree->right_subtree == nullptr && node_tree->left_subtree != nullptr)
            {
                return get_subtree_height(node_tree->left_subtree);
            }

            return 0;
        }

        void update_height(typename AVL_tree<tkey, tvalue>::node *node)
        {
            node->_subtree_height =
                    std::max(get_subtree_height(node->left_subtree), get_subtree_height(node->right_subtree)) + 1;
        }

        void balance_for_insert_and_dispose(std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path,
                                            AVL_tree<tkey, tvalue> const *tree)
        {
            auto **current_node = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node **>(path.top());
            auto *current_node_additional = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(*(path.top()));
            std::cout << "node1: " << current_node_additional->key << std::endl;
            path.pop();

            int height_difference = get_balance_node(*current_node);
            std::cout << "height1: " << height_difference << std::endl;

            while (!path.empty())
            {
                current_node = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node **>(path.top());
                current_node_additional = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(*(path.top()));
                path.pop();

                std::cout << "node2: " << current_node_additional->key << std::endl;

                update_height(*current_node);
                height_difference = get_balance_node(*current_node);

                std::cout << "height2: " << height_difference << std::endl;

                if (height_difference == 2)
                {
                    std::cout << "2" << std::endl;
                    auto *left_child = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(current_node_additional->left_subtree);
                    auto **left_child_additional = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node **>(&(current_node_additional->left_subtree));
                    height_difference = get_balance_node(left_child);

                    if (height_difference >= 0)
                    {
                        std::cout << ">= 0" << std::endl;
                        tree->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(current_node));
                        update_height(*current_node);
                    }
                    else
                    {
                        std::cout << "> 0" << std::endl;
                        tree->big_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(current_node));
                        update_height(*current_node);
                    }
                }
                else if (height_difference == -2)
                {
                    std::cout << "-2" << std::endl;
                    auto *right_child = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(current_node_additional->right_subtree);
                    height_difference = get_balance_node(right_child);

                    if (height_difference <= 0)
                    {
                        std::cout << "<= 0" << std::endl;
                        tree->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(current_node));
                        update_height(*current_node);
                    }
                    else
                    {
                        std::cout << "> 0" << std::endl;
                        tree->big_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(current_node));
                        update_height(*current_node);
                    }
                }
            }
        }
    };

    class insertion_template_method final:
            public binary_search_tree<tkey, tvalue>::insertion_template_method,
            public balancer_for_avl_tree
    {

    public:

        explicit insertion_template_method(
                AVL_tree<tkey, tvalue> *tree,
                typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy) :
                    binary_search_tree<tkey, tvalue>::insertion_template_method(tree, insertion_strategy)
        {

        }

    private:

        void balance(std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
            this->balance_for_insert_and_dispose(path, dynamic_cast<AVL_tree<tkey, tvalue> const *>(this->_tree));
        }
    };

    class disposal_template_method final:
            public binary_search_tree<tkey, tvalue>::disposal_template_method,
            public balancer_for_avl_tree
    {

    public:

        explicit disposal_template_method(
                AVL_tree<tkey, tvalue> *tree,
                typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy) :
                    binary_search_tree<tkey, tvalue>::disposal_template_method(tree, disposal_strategy)
        {

        }

    private:

        void balance(std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
            path.pop();
            this->balance_for_insert_and_dispose(path, dynamic_cast<AVL_tree<tkey, tvalue> const *>(this->_tree));
        }

    };

public:

    explicit AVL_tree(
            std::function<int(tkey const &, tkey const &)> comparer,
            allocator *allocator = nullptr,
            logger *logger = nullptr,
            typename binary_search_tree<tkey, tvalue>::
                    insertion_of_existent_key_attempt_strategy insertion_strategy = binary_search_tree<tkey, tvalue>::
                            insertion_of_existent_key_attempt_strategy::throw_an_exception,
            typename binary_search_tree<tkey, tvalue>::
                    disposal_of_nonexistent_key_attempt_strategy disposal_strategy = binary_search_tree<tkey, tvalue>::
                            disposal_of_nonexistent_key_attempt_strategy::throw_an_exception) :
                binary_search_tree<tkey, tvalue>(new AVL_tree<tkey, tvalue>::
                        insertion_template_method(this, insertion_strategy), new typename binary_search_tree<tkey, tvalue>::
                                obtaining_template_method(dynamic_cast<binary_search_tree<tkey, tvalue> *>(this)), new AVL_tree<tkey, tvalue>::
                                        disposal_template_method(this, disposal_strategy), comparer, allocator, logger)
    {

    }

public:

    ~AVL_tree() noexcept final = default;

    AVL_tree(AVL_tree<tkey, tvalue> const &other) :
            AVL_tree<tkey, tvalue>(other._keys_comparer, other.get_allocator(), other.get_logger(),
                                   binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy::throw_an_exception,
                                   binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy::throw_an_exception)
    {

    }

    AVL_tree<tkey, tvalue> &operator=(AVL_tree<tkey, tvalue> const &other)
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

    AVL_tree(AVL_tree<tkey, tvalue> &&other) noexcept :
            AVL_tree<tkey, tvalue>(other._keys_comparer, other.get_allocator(), other.get_logger())
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

    AVL_tree<tkey, tvalue> &operator=(AVL_tree<tkey, tvalue> &&other) noexcept
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

private:

    size_t get_node_size() const noexcept override
    {
        return sizeof(typename AVL_tree<tkey, tvalue>::node);
    }

    void call_node_constructor(
            typename binary_search_tree<tkey, tvalue>::node *raw_space,
            tkey const &key,
            tvalue const &value) override
    {
        allocator::construct(reinterpret_cast<typename AVL_tree<tkey, tvalue>::node*>(raw_space), key, value);
    }

    void call_node_constructor(
            typename binary_search_tree<tkey, tvalue>::node *raw_space,
            tkey const &key,
            tvalue &&value) override
    {
        allocator::construct(dynamic_cast<typename AVL_tree<tkey, tvalue>::node*>(raw_space), key, std::move(value));
    }

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_AVL_TREE_H