#include <not_implemented.h>

#include "../include/allocator_red_black_tree.h"

allocator_red_black_tree::~allocator_red_black_tree()
{
    delete get_mutex();
    auto* allocator = get_allocator();

    if (allocator == nullptr)
    {
        ::operator delete(_trusted_memory);
    }
    else
    {
        allocator->deallocate(_trusted_memory);
    }
}

allocator_red_black_tree::allocator_red_black_tree(allocator_red_black_tree &&other) noexcept :
    _trusted_memory(other._trusted_memory)
{
    _trusted_memory = nullptr;
}

allocator_red_black_tree &allocator_red_black_tree::operator=(allocator_red_black_tree &&other) noexcept
{
    if (this != &other)
    {
        allocator* all = get_allocator();

        if (all != nullptr)
        {
            all->deallocate(_trusted_memory);
        }
        else
        {
            ::operator delete(_trusted_memory);
        }

        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }

    return *this;
}

inline size_t allocator_red_black_tree::get_ancillary_space_size() const noexcept
{
    return sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(void*);
}

allocator_red_black_tree::allocator_red_black_tree(
    size_t space_size,
    allocator *parent_allocator,
    logger *logger,
    allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    auto common_size = space_size + get_ancillary_space_size();

    try
    {
        _trusted_memory = parent_allocator == nullptr
                          ? ::operator new(common_size)
                          : parent_allocator->allocate(common_size, 1);
    }
    catch (std::bad_alloc const &ex)
    {
        logger->error(get_typename() + "error with allocate memory");
    }

    allocator** parent_allocator_space_address = reinterpret_cast<allocator **>(_trusted_memory);
    *parent_allocator_space_address = parent_allocator;

    class logger** logger_space_address = reinterpret_cast<class logger**>(parent_allocator_space_address + 1);
    *logger_space_address = logger;

    size_t *space_size_space_address = reinterpret_cast<size_t *>(logger_space_address + 1);
    *space_size_space_address = space_size;

    size_t *size_of_free_memory = reinterpret_cast<size_t *>(space_size_space_address + 1);
    *size_of_free_memory = space_size;

    allocator_with_fit_mode::fit_mode *fit_mode_space_address = reinterpret_cast<allocator_with_fit_mode::fit_mode *>(size_of_free_memory + 1);
    *fit_mode_space_address = allocate_fit_mode;

    std::mutex** mutex_space_address = reinterpret_cast<std::mutex** >(fit_mode_space_address + 1);
    *mutex_space_address = new std::mutex;

    void **root = reinterpret_cast<void **>(mutex_space_address + 1);
    *root = reinterpret_cast<void *>(root + 1);

    // free blocks metadata:
    // bool is_occupied
    // void* previous
    // void* next
    // unsigned char colour
    // node* parent
    // node* left_subtree
    // node* right_subtree
    // size_t size

    // 0 - free, 1 - occupied

    auto* is_occupied_block = reinterpret_cast<unsigned char*>(*root);
    *is_occupied_block = 0;

    // 0 - black colour
    // 1 - red colour
    auto* block_colour = reinterpret_cast<unsigned char*>(is_occupied_block + 1);
    *block_colour = 0;

    void** next_free_block = reinterpret_cast<void**>(block_colour + 1);
    *next_free_block = nullptr;

    void** previous_free_block = reinterpret_cast<void**>(next_free_block + 1);
    *previous_free_block = nullptr;

    size_t* size_block = reinterpret_cast<size_t*>(previous_free_block + 1);
    *size_block = space_size;

    void** parent_of_blocks = reinterpret_cast<void**>(size_block + 1);
    *parent_of_blocks = nullptr;

    void** left_subtree_blocks = reinterpret_cast<void**>(parent_of_blocks + 1);
    *left_subtree_blocks = nullptr;

    void** right_subtree_blocks = reinterpret_cast<void**>(left_subtree_blocks + 1);
    *right_subtree_blocks = nullptr;

    // occupied block metadata:
    // void* previous
    // void* next
}

inline allocator *allocator_red_black_tree::get_allocator() const
{
    return *reinterpret_cast<allocator**>(_trusted_memory);
}

inline logger *allocator_red_black_tree::get_logger() const
{
    return *reinterpret_cast<logger **>(reinterpret_cast<allocator **>(_trusted_memory) + 1);
}

size_t allocator_red_black_tree::get_common_size_of_allocator() const
{
    return *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*));
}

size_t allocator_red_black_tree::get_free_size_of_allocator() const
{
    return *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t));
}

allocator_with_fit_mode::fit_mode allocator_red_black_tree::get_fit_mode() const noexcept
{
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t));
}

void *allocator_red_black_tree::get_root() const noexcept
{
    return *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));
}

std::mutex *allocator_red_black_tree::get_mutex() const noexcept
{
    return *reinterpret_cast<std::mutex **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
}

size_t allocator_red_black_tree::get_small_free_metadata() const noexcept
{
    return sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) * 5 + sizeof(size_t);
}

unsigned char allocator_red_black_tree::get_is_occupied_block(void* block_address) const noexcept
{
    return *reinterpret_cast<unsigned char*>(block_address);
}

unsigned char allocator_red_black_tree::get_colour_block(void* block_address) const noexcept
{
    return *reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char);
}

void* allocator_red_black_tree::get_next_free_block(void* block_address) const noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char) * 2);
}

void* allocator_red_black_tree::get_previous_free_block(void* block_address) const noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char) * 2 + sizeof(void*));
}

size_t allocator_red_black_tree::get_size_block(void* block_address) const noexcept
{
    return *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char) * 2 + sizeof(void*) * 2);
}

void* allocator_red_black_tree::get_parent_block(void* block_address) const noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char) * 2 + sizeof(void*) * 2 + sizeof(size_t));
}

void* allocator_red_black_tree::get_left_subtree_block(void* block_address) const noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char) * 2 + sizeof(void*) * 3 + sizeof(size_t));
}

void* allocator_red_black_tree::get_right_subtree_block(void* block_address) const noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char) * 2 + sizeof(void*) * 4 + sizeof(size_t));
}

void allocator_red_black_tree::delete_block(void* target_block) noexcept
{
    bool flag_black_tree = false;

    if (get_left_subtree_block(target_block) != nullptr && get_right_subtree_block(target_block) != nullptr)
    {
        void* left_subtree_for_target_block = get_left_subtree_block(target_block);
        void* right_subtree_for_target_block = get_right_subtree_block(target_block);

        void* parent_for_target_block = get_parent_block(target_block);

        void* max_node_in_left_subtree = left_subtree_for_target_block;

        while (get_right_subtree_block(max_node_in_left_subtree) != nullptr)
        {
            max_node_in_left_subtree = get_right_subtree_block(max_node_in_left_subtree);
        }

        unsigned char colour_target_block = get_colour_block(max_node_in_left_subtree);

        void* parent_for_max_node_in_left_subtree = get_parent_block(max_node_in_left_subtree);

        if (parent_for_target_block == nullptr)
        {
            if (get_left_subtree_block(max_node_in_left_subtree) != nullptr)
            {
                void* left_subtree_for_max_node_in_left_subtree = get_left_subtree_block(max_node_in_left_subtree);

                if (get_right_subtree_block(parent_for_max_node_in_left_subtree) == max_node_in_left_subtree)
                {
                    void** right_subtree_p = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_for_max_node_in_left_subtree) + sizeof(unsigned char) * 2 + sizeof(void*) * 3 + sizeof(size_t) + sizeof(void*));
                    *right_subtree_p = left_subtree_for_max_node_in_left_subtree;
                }
            }

            void **first_free_block = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) +
                                                                sizeof(allocator *) + sizeof(logger *) +
                                                                sizeof(size_t) + sizeof(size_t) +
                                                                sizeof(allocator_with_fit_mode::fit_mode) +
                                                                sizeof(std::mutex *));
            *first_free_block = max_node_in_left_subtree;

            void** left_subtree_for_new_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(max_node_in_left_subtree) + sizeof(unsigned char) * 2 + sizeof(void*) * 3 + sizeof(size_t));
            *left_subtree_for_new_block = left_subtree_for_target_block;

            void** right_subtree_for_new_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(max_node_in_left_subtree) + sizeof(unsigned char) * 2 + sizeof(void*) * 4 + sizeof(size_t));
            *right_subtree_for_new_block = right_subtree_for_target_block;
        }
        else
        {
            if (get_left_subtree_block(max_node_in_left_subtree) != nullptr)
            {
                void* left_subtree_for_max_node_in_left_subtree = get_left_subtree_block(max_node_in_left_subtree);

                if (get_right_subtree_block(parent_for_max_node_in_left_subtree) == max_node_in_left_subtree)
                {
                    void** right_subtree_p = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_for_max_node_in_left_subtree) + sizeof(unsigned char) * 2 + sizeof(void*) * 3 + sizeof(size_t) + sizeof(void*));
                    *right_subtree_p = left_subtree_for_max_node_in_left_subtree;
                }
            }

            *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(max_node_in_left_subtree) + sizeof(unsigned char) * 2 +
                                           sizeof(void *) * 2 + sizeof(size_t)) = parent_for_target_block;

            void** left_subtree_for_new_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(max_node_in_left_subtree) + sizeof(unsigned char) * 2 + sizeof(void*) * 3 + sizeof(size_t));
            *left_subtree_for_new_block = left_subtree_for_target_block;

            void** right_subtree_for_new_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(max_node_in_left_subtree) + sizeof(unsigned char) * 2 + sizeof(void*) * 4 + sizeof(size_t));
            *right_subtree_for_new_block = right_subtree_for_target_block;
        }

        unsigned char* color_max_node = reinterpret_cast<unsigned char*>(max_node_in_left_subtree) + sizeof(unsigned char);
        *color_max_node = colour_target_block;

        if (*color_max_node == 0)
        {
            flag_black_tree = true;
        }
    }
    else if (get_left_subtree_block(target_block) == nullptr || get_right_subtree_block(target_block) == nullptr)
    {
        void* subtree = nullptr;
        void* parent_block_for_target_block = get_parent_block(target_block);

        if (get_right_subtree_block(target_block) == nullptr)
        {
            subtree = get_left_subtree_block(target_block);
        }
        if (get_left_subtree_block(target_block) == nullptr)
        {
            subtree = get_right_subtree_block(target_block);
        }

        if (subtree == nullptr)
        {
            std::logic_error("wrong with ptr");
        }

        unsigned char colour_target_block = get_colour_block(target_block);

        if (colour_target_block == 1)
        {
            std::logic_error("wrong color of block");
        }

        // цвет удаляемого элемента - черный, красного быть не может из-за нарушения свойства черной высоты
        // 0 - black, 1 - red

        if (colour_target_block == 0)
        {
            flag_black_tree = true;
        }

        *reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned char*>(subtree) +
            sizeof(unsigned char)) = 1; // red

        if (parent_block_for_target_block == nullptr)
        {
            void** first_free_block = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) +
                    sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));
            *first_free_block = subtree;
        }
        else
        {
            *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(subtree) + sizeof(unsigned char) * 2 +
                                       sizeof(void *) * 2 + sizeof(size_t)) = parent_block_for_target_block;

            if (get_right_subtree_block(parent_block_for_target_block) == target_block)
            {
                void** right_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_block_for_target_block) + sizeof(unsigned char) * 2 + sizeof(void*) * 3 + sizeof(size_t) + sizeof(void*));
                *right_subtree = subtree;
            }
            else
            {
                void** left_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_block_for_target_block) + sizeof(unsigned char) * 2 + sizeof(void*) * 3 + sizeof(size_t));
                *left_subtree = subtree;
            }
        }
    }
    else
    {
         void* parent_block_for_target_block = get_parent_block(target_block);

         unsigned char colour_block = get_colour_block(target_block);

         if (colour_block == 0)
         {
             flag_black_tree = true;
         }

         if (parent_block_for_target_block == nullptr)
         {
             void** first_free_block = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) +
                                                                 sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));
             *first_free_block = nullptr;
         }
         else
         {
             if (get_right_subtree_block(parent_block_for_target_block) == target_block)
             {
                 void** right_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_block_for_target_block) + sizeof(unsigned char) * 2 + sizeof(void*) * 3 + sizeof(size_t) + sizeof(void*));
                 *right_subtree = nullptr;
             }
             else
             {
                 void** left_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_block_for_target_block) + sizeof(unsigned char) * 2 + sizeof(void*) * 3 + sizeof(size_t));
                 *left_subtree = nullptr;
             }
         }
    }

    if (flag_black_tree)
    {
        fix_red_black_tree(target_block);
    }

}

void allocator_red_black_tree::fix_red_black_tree(void* target_block) noexcept
{
    // 0 - black, 1 - red
    void* parent_for_target_block = get_parent_block(target_block);
    void* grandparent_for_target_block = get_parent_block(parent_for_target_block);
    void* uncle_for_target_block = nullptr;
    unsigned char color_target_block = 0;

    if (parent_for_target_block == nullptr)
    {
        *reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char)) = 0;
    }
    else
    {
        void* left_subtree = nullptr;
        void* right_subtree = nullptr;
        bool flag_left_or_right = false;

        // false - left, right - true

        if (get_right_subtree_block(grandparent_for_target_block) == parent_for_target_block)
        {
            uncle_for_target_block = get_left_subtree_block(grandparent_for_target_block);
            flag_left_or_right = false;
        }
        else
        {
            uncle_for_target_block = get_right_subtree_block(grandparent_for_target_block);
            flag_left_or_right = true;
        }

        unsigned char* colour_for_uncle = reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned char*>(uncle_for_target_block) + sizeof(unsigned char));
        *colour_for_uncle = 0;

        if (*colour_for_uncle == 0) // black
        {

        }
        else // red
        {
            if (flag_left_or_right)
            {
                // TODO SMALL RIGHT ROTATION
            }
            else
            {
                // TODO SMALL LEFT ROTATION
            }
        }


    }
}

void allocator_red_black_tree::small_right_rotation(void* address_block) noexcept
{

}

void allocator_red_black_tree::big_right_rotation(void* address_block) noexcept
{

}

void allocator_red_black_tree::small_left_rotation(void* address_block) noexcept
{

}

void allocator_red_black_tree::big_left_rotation(void* address_block) noexcept
{

}

void* allocator_red_black_tree::get_best_fit(size_t size) const noexcept
{
    void* current_block = get_root();
    void* previous_block = nullptr;

    if (current_block == nullptr)
    {
        return nullptr;
    }

    while (current_block != nullptr)
    {
        size_t current_block_size = get_size_block(current_block);

        if (current_block_size >= size)
        {
            previous_block = current_block;
        }

        if (current_block_size < size)
        {
            current_block = get_right_subtree_block(current_block);
        }
        else if (current_block_size > size)
        {
            current_block = get_left_subtree_block(current_block);
        }
        else
        {
            current_block = nullptr;
        }
    }

    return previous_block;
}

void* allocator_red_black_tree::get_worst_fit(size_t size) const noexcept
{
    void* current_block = get_root();
    void* previous_block = nullptr;

    if (current_block == nullptr)
    {
        return nullptr;
    }

    while (current_block != nullptr)
    {
        size_t current_block_size = get_size_block(current_block);

        if (current_block_size >= size)
        {
            previous_block = current_block;
        }

        current_block = get_right_subtree_block(current_block);
    }

    return previous_block;
}

void* allocator_red_black_tree::get_first_fit(size_t size) const noexcept
{
    void* current_block = get_root();
    void* previous_block = nullptr;

    if (current_block == nullptr)
    {
        return nullptr;
    }

    while (current_block != nullptr)
    {
        size_t current_block_size = get_size_block(current_block);

        if (current_block_size >= size)
        {
            return current_block;
        }

        current_block = get_right_subtree_block(current_block);
    }

    return nullptr;
}

[[nodiscard]] void *allocator_red_black_tree::allocate(
    size_t value_size,
    size_t values_count)
{
    size_t requested_size = value_size * values_count;

    allocator_with_fit_mode::fit_mode fit_mode = get_fit_mode();

    std::cout << "Information about blocks of memory: " << std::to_string(requested_size) << std::endl;

    void *target_block = nullptr;
    void *previous_to_target_block = get_previous_free_block(target_block);
    void* next_to_target_block = get_next_free_block(target_block);

    switch (fit_mode)
    {
        case allocator_with_fit_mode::fit_mode::the_worst_fit:
            target_block = get_worst_fit(requested_size);
            break;

        case allocator_with_fit_mode::fit_mode::first_fit:
            target_block = get_first_fit(requested_size);
            break;

        case allocator_with_fit_mode::fit_mode::the_best_fit:
            target_block = get_best_fit(requested_size);
            break;
    }

    if (target_block == nullptr)
    {
        error_with_guard(get_typename() + "can't allocate");
        throw std::bad_alloc();
    }

    delete_block(target_block);

    *reinterpret_cast<unsigned char*>(target_block) = 1;

    size_t target_block_size = get_size_block(target_block);

    if (previous_to_target_block == nullptr)
    {
        size_t blocks_sizes_difference = target_block_size - requested_size;

        if (blocks_sizes_difference > 0 && blocks_sizes_difference < get_small_free_metadata())
        {
            requested_size = target_block_size;
            void* next_available_block = get_next_free_block(target_block);
            void** first_available_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));
            *first_available_block = next_available_block;
        }
        else
        {
            *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char) * 2 + sizeof(void*) * 2) = requested_size;

        }
    }
    else
    {
        size_t blocks_sizes_difference = target_block_size - requested_size;

        if (blocks_sizes_difference > 0 && blocks_sizes_difference < get_small_free_metadata())
        {
            requested_size = target_block_size;
            void* next_available_block = get_next_free_block(target_block);
            void** first_available_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));
            *first_available_block = next_available_block;
        }
        else
        {
            *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char) * 2 + sizeof(void*) * 2) = requested_size;

        }
    }

}

void allocator_red_black_tree::deallocate(
    void *at)
{
    throw not_implemented("void allocator_red_black_tree::deallocate(void *)", "your code should be here...");
}

inline void allocator_red_black_tree::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{

}

std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const noexcept
{
    throw not_implemented("std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const noexcept", "your code should be here...");
}

inline std::string allocator_red_black_tree::get_typename() const noexcept
{
    return "ALLOCATOR_RED_BLACK_TREE: ";
}