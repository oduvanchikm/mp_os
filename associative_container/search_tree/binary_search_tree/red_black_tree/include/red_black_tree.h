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
        
        node_color color;

    public:

        explicit node(
                tkey const &key,
                tvalue const &value) :
                    binary_search_tree<tkey, tvalue>::node(key, value),
                    color(node_color::RED)
        {

        }

        explicit node(
                tkey const &key,
                tvalue const &&value) :
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
        
        node_color color;
    
    public:
        
        explicit iterator_data(
            unsigned int depth,
            tkey const &key,
            tvalue const &value,
            node_color color);

    public:

        virtual ~iterator_data() noexcept
        {

        }

        iterator_data(red_black_tree<tkey, tvalue>::iterator_data const &other) :
                iterator_data(other.depth, other.get_key(), other.get_value(), other.color)
        {

        }

        iterator_data &operator=(red_black_tree<tkey, tvalue>::iterator_data const &other) noexcept
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
                this->color = other.color;
            }

            return *this;
        }

        iterator_data(red_black_tree<tkey, tvalue>::iterator_data const &&other) noexcept :
                iterator_data(other.depth, other.get_key(), other.get_value(), other.color)
        {
            allocator::destruct(other._key);
            allocator::destruct(other._value);

            other._is_state_initialized = false;
            this->_is_state_initialized = true;

        }

        iterator_data &operator=(red_black_tree<tkey, tvalue>::iterator_data const &&other) noexcept
        {
            if (this != &other)
            {
                if (this->is_state_initialized())
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
                this->color = other.color;
                other.is_state_initialized = false;

            }

            return *this;
        }
        
    };

private:

    class balancer
    {

    public:

        void balancer_after_insert(
                std::stack<typename binary_search_tree<tkey,tvalue>::node **> &path,
                red_black_tree<tkey, tvalue> const *tree)
        {
            // balance logic for insert

            if (path.empty())
            {
                return;
            }

            if (path.size() == 1 && path.top() != nullptr)
            {
                static_cast<node*>(*path.top())->color = node_color::BLACK;
            }
            else
            {
                typename 
            }

            
        }

        void balancer_after_disposal(
                std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path,
                red_black_tree<tkey, tvalue> const *tree)
        {
            // balance logic for disposal
        }
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

         void balance(
                 std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
             this->balancer_after_insert(path, dynamic_cast<red_black_tree<tkey, tvalue> const*>(this->_tree));
        }
        
    };
    
//    class obtaining_template_method final:
//        public binary_search_tree<tkey, tvalue>::obtaining_template_method
//    {
//
//    public:
//
//        explicit obtaining_template_method(red_black_tree<tkey, tvalue> *tree);
//
//         TODO: think about it!
//
//    };
    
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
            this->balancer_after_insert(path, dynamic_cast<red_black_tree<tkey, tvalue> const*>(this->_tree));
        }
        
    };

public:
    
    explicit red_black_tree(
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

    size_t get_node_size() const noexcept override
    {
        return sizeof(typename red_black_tree<tkey, tvalue>::node);
    }

    void call_node_constructor(
            typename binary_search_tree<tkey, tvalue>::node *raw_space,
            tkey const &key,
            tvalue const &value) override
    {
        allocator::construct(dynamic_cast<red_black_tree<tkey, tvalue>::node>(raw_space), key, value);
    }

    void call_node_constructor(
            typename binary_search_tree<tkey, tvalue>::node *raw_space,
            tkey const &key,
            tvalue &&value) override
    {
        allocator::construct(dynamic_cast<red_black_tree<tkey, tvalue>::node>(raw_space), key, std::move(value));
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
    throw not_implemented("template<typename tkey, typename tvalue> red_black_tree<tkey, tvalue>::iterator_data::iterator_data(unsigned int, tkey const &, tvalue const &, typename red_black_tree<tkey, tvalue>::node_color)", "your code should be here...");
}

template<typename tkey, typename tvalue>
red_black_tree<tkey, tvalue>::insertion_template_method::insertion_template_method(
    red_black_tree<tkey, tvalue> *tree,
    typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy):
        binary_search_tree<tkey, tvalue>::insertion_template_method(tree, insertion_strategy)
{
    throw not_implemented("template<typename tkey, typename tvalue> red_black_tree<tkey, tvalue>::insertion_template_method::insertion_template_method(red_black_tree<tkey, tvalue> *, typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy)", "your code should be here...");
}

//template<typename tkey,typename tvalue>
//red_black_tree<tkey, tvalue>::obtaining_template_method::obtaining_template_method(red_black_tree<tkey, tvalue> *tree)
//{
//    throw not_implemented("template<typename tkey, typename tvalue> red_black_tree<tkey, tvalue>::obtaining_template_method::obtaining_template_method(red_black_tree<tkey, tvalue> *)", "your code should be here...");
//}

template<typename tkey,typename tvalue>
red_black_tree<tkey, tvalue>::disposal_template_method::disposal_template_method(
    red_black_tree<tkey, tvalue> *tree,
    typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy) :
        binary_search_tree<tkey, tvalue>::disposal_template_method(tree, disposal_strategy)
{
    throw not_implemented("template<typename tkey, typename tvalue> red_black_tree<tkey, tvalue>::disposal_template_method::disposal_template_method(red_black_tree<tkey, tvalue> *, typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy)", "your code should be here...");
}

template<typename tkey,typename tvalue>
red_black_tree<tkey, tvalue>::red_black_tree(
        allocator* allocator,
        logger *logger,
        typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy,
        typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy) :
        binary_search_tree<tkey, tvalue>()
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
    throw not_implemented("template<typename tkey, typename tvalue> red_black_tree<tkey, tvalue> &red_black_tree<tkey, tvalue>::operator=(red_black_tree<tkey, tvalue> const &)", "your code should be here...");
}

template<typename tkey,typename tvalue>
red_black_tree<tkey, tvalue>::red_black_tree(red_black_tree<tkey, tvalue> &&other) noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue> red_black_tree<tkey, tvalue>::red_black_tree(red_black_tree<tkey, tvalue> &&) noexcept", "your code should be here...");
}

template<typename tkey,typename tvalue>
red_black_tree<tkey, tvalue> &red_black_tree<tkey, tvalue>::operator=(red_black_tree<tkey, tvalue> &&other) noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue> red_black_tree<tkey, tvalue> &red_black_tree<tkey, tvalue>::operator=(red_black_tree<tkey, tvalue> &&) noexcept", "your code should be here...");
}

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H
