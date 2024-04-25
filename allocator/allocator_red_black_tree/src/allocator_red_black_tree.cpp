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
    *size_block = space_size - get_small_free_metadata();

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

std::mutex *allocator_red_black_tree::get_mutex() const noexcept
{
    return *reinterpret_cast<std::mutex **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
}

void *allocator_red_black_tree::get_root() const noexcept
{
    return *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));
}

size_t allocator_red_black_tree::get_small_free_metadata() const noexcept
{
    return sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(void*) + sizeof(void*) + sizeof(void*);
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
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char) + sizeof(unsigned char));
}

void* allocator_red_black_tree::get_previous_free_block(void* block_address) const noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*));
}

size_t allocator_red_black_tree::get_size_block(void* block_address) const noexcept
{
    return *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*));
}

void* allocator_red_black_tree::get_parent_block(void* block_address) const noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t));
}

void* allocator_red_black_tree::get_left_subtree_block(void* block_address) const noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(void*));
}

void* allocator_red_black_tree::get_right_subtree_block(void* block_address) const noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(void*) + sizeof(void*));
}

void allocator_red_black_tree::print_tree(void *block, size_t depth)
{
    if (block != nullptr)
    {
        print_tree(get_right_subtree_block(block), depth + 1);
        for (size_t i = 0; i < depth; ++i)
        {
            std::cout << '\t';
        }

        std::cout << get_size_block(block) << " " << (get_colour_block(block) == 1) << " " << get_is_occupied_block(block) << std::endl;

        print_tree(get_left_subtree_block(block), depth + 1);
    }
}

void allocator_red_black_tree::delete_block(void* target_block) noexcept
{
    bool flag_black_tree = false;

    if (get_left_subtree_block(target_block) == nullptr && get_right_subtree_block(target_block) == nullptr)
    {
        void* parent_block_for_target_block = get_parent_block(target_block);

        unsigned char colour_block = get_colour_block(target_block);

        if (colour_block == 0)
        {
            flag_black_tree = true;
        }

        if (parent_block_for_target_block == nullptr)
        {
            void** first_free_block = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));
            *first_free_block = nullptr;
        }
        else
        {
            if (get_right_subtree_block(parent_block_for_target_block) == target_block)
            {
                void** right_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_block_for_target_block) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*) + sizeof(void*));
                *right_subtree = nullptr;
            }
            else
            {
                void** left_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_block_for_target_block) + + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*));
                *left_subtree = nullptr;
            }
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

        *reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned char*>(subtree) + sizeof(unsigned char)) = 1; // red

        if (parent_block_for_target_block == nullptr)
        {
            void** first_free_block = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));
            *first_free_block = subtree;
        }
        else
        {
            *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(subtree) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void *) + sizeof(void*)+ sizeof(size_t)) = parent_block_for_target_block;

            if (get_right_subtree_block(parent_block_for_target_block) == target_block)
            {
                void** right_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_block_for_target_block) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*) + sizeof(void*));
                *right_subtree = subtree;
            }
            else
            {
                void** left_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_block_for_target_block) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*));
                *left_subtree = subtree;
            }
        }
    }
    else
    {
        void* left_subtree_for_target_block = get_left_subtree_block(target_block);
        void* right_subtree_for_target_block = get_right_subtree_block(target_block);
        void* parent_for_target_block = get_parent_block(target_block);
        void* max_node_in_left_subtree = get_left_subtree_block(target_block);

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
                    void** right_subtree_p = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_for_max_node_in_left_subtree) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*) + sizeof(void*));
                    *right_subtree_p = left_subtree_for_max_node_in_left_subtree;
                }
            }

            void **first_free_block = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) +
                                                                sizeof(allocator *) + sizeof(logger *) +
                                                                sizeof(size_t) + sizeof(size_t) +
                                                                sizeof(allocator_with_fit_mode::fit_mode) +
                                                                sizeof(std::mutex *));
            *first_free_block = max_node_in_left_subtree;

            void** left_subtree_for_new_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(max_node_in_left_subtree) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*));
            *left_subtree_for_new_block = left_subtree_for_target_block;

            void** right_subtree_for_new_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(max_node_in_left_subtree) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*) + sizeof(void*));
            *right_subtree_for_new_block = right_subtree_for_target_block;
        }
        else
        {
            if (get_left_subtree_block(max_node_in_left_subtree) != nullptr)
            {
                void* left_subtree_for_max_node_in_left_subtree = get_left_subtree_block(max_node_in_left_subtree);

                if (get_right_subtree_block(parent_for_max_node_in_left_subtree) == max_node_in_left_subtree)
                {
                    void** right_subtree_p = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_for_max_node_in_left_subtree) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*) + sizeof(void*));
                    *right_subtree_p = left_subtree_for_max_node_in_left_subtree;
                }
            }

            *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(max_node_in_left_subtree) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void *) + sizeof(void *) + sizeof(size_t)) = parent_for_target_block;

            void** left_subtree_for_new_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(max_node_in_left_subtree) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*));
            *left_subtree_for_new_block = left_subtree_for_target_block;

            void** right_subtree_for_new_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(max_node_in_left_subtree) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*) + sizeof(void*));
            *right_subtree_for_new_block = right_subtree_for_target_block;
        }

        unsigned char* color_max_node = reinterpret_cast<unsigned char*>(max_node_in_left_subtree) + sizeof(unsigned char);
        *color_max_node = colour_target_block;

        if (*color_max_node == 0)
        {
            flag_black_tree = true;
        }
    }

    if (flag_black_tree)
    {
        fix_red_black_tree(get_parent_block(target_block));
    }
}

void allocator_red_black_tree::small_right_rotation(void* address_block) noexcept
{
    void* new_root = get_left_subtree_block(address_block);

    if (new_root != nullptr)
    {
        void** right_node_for_new_root = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(new_root) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*) + sizeof(void*));
        *right_node_for_new_root = address_block;

        void** left_node_for_address_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(new_root) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*));
        *left_node_for_address_block = get_right_subtree_block(new_root);

        void* parent_for_new_root = get_parent_block(address_block);

        if (parent_for_new_root == nullptr)
        {
            void** first_root_block = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));
            *first_root_block = new_root;
        }
        else
        {
            if (get_right_subtree_block(parent_for_new_root) == address_block)
            {
                void** right_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_for_new_root) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*) + sizeof(void*));
                *right_subtree = new_root;
            }
            else
            {
                void** left_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_for_new_root) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*));
                *left_subtree = new_root;
            }
        }
    }
}

void allocator_red_black_tree::small_left_rotation(void* address_block) noexcept
{
    void* new_root = get_right_subtree_block(address_block);

    if (new_root != nullptr)
    {
        void** right_node_for_address_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(new_root) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*) + sizeof(void*));
        *right_node_for_address_block = get_left_subtree_block(new_root);

        void** left_node_for_new_root = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(new_root) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*));
        *left_node_for_new_root = address_block;

        void* parent_for_new_root = get_parent_block(address_block);

        if (parent_for_new_root == nullptr)
        {
            void** first_root_block = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));
            *first_root_block = new_root;
        }
        else
        {
            if (get_right_subtree_block(parent_for_new_root) == address_block)
            {
                void** right_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_for_new_root) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*) + sizeof(void*));
                *right_subtree = new_root;
            }
            else
            {
                void** left_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_for_new_root) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(size_t) + sizeof(void*));
                *left_subtree = new_root;
            }
        }
    }
}

void allocator_red_black_tree::big_right_rotation(void* address_block) noexcept
{
    void* left_subtree_for_address_block = get_left_subtree_block(address_block);
    void* right_subtree_for_child_address_block = get_right_subtree_block(left_subtree_for_address_block);

    if (left_subtree_for_address_block != nullptr && right_subtree_for_child_address_block != nullptr)
    {
        small_left_rotation(left_subtree_for_address_block);
        small_right_rotation(address_block);
    }
}

void allocator_red_black_tree::big_left_rotation(void* address_block) noexcept
{
    void* right_subtree_for_address_block = get_right_subtree_block(address_block);
    void* left_subtree_for_child_address_block = get_left_subtree_block(right_subtree_for_address_block);

    if (right_subtree_for_address_block != nullptr && left_subtree_for_child_address_block != nullptr)
    {
        small_right_rotation(right_subtree_for_address_block);
        small_left_rotation(address_block);
    }
}

void* allocator_red_black_tree::get_best_fit(size_t size) const noexcept
{
    void* current_block = get_root();

    if (current_block == nullptr)
    {
        return nullptr;
    }

    size_t current_block_size = get_size_block(current_block);

    void* previous_block = nullptr;

    while (current_block_size < size && current_block)
    {
        current_block = get_right_subtree_block(current_block);
        if (current_block)
        {
            current_block_size = get_size_block(current_block);
        }
    }

    if (current_block == nullptr)
    {
        return nullptr;
    }

    while (get_left_subtree_block(current_block))
    {
        void* left_subtree = get_left_subtree_block(current_block);
        size_t size_left_subtree = get_size_block(left_subtree);

        if (size_left_subtree >= size)
        {
            current_block = left_subtree;
        }
        else
        {
            break;
        }
    }

    return current_block;
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

[[nodiscard]] void *allocator_red_black_tree::allocate(size_t value_size, size_t values_count)
{
    std::mutex* mutex_boundary_tags = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_boundary_tags);

    size_t requested_size = value_size * values_count;

    if (requested_size < get_small_free_metadata())
    {
        requested_size = get_small_free_metadata();
    }

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

    size_t target_block_size = get_size_block(target_block);

    std::cout << "target block size: " << std::to_string(target_block_size) << std::endl;

    size_t blocks_sizes_difference = target_block_size - requested_size;

    std::cout << "difference: " << std::to_string(blocks_sizes_difference) << std::endl;
    std::cout << "small meta: " << std::to_string(get_small_free_metadata()) << std::endl;

    if (blocks_sizes_difference < get_small_free_metadata())
    {
        requested_size = target_block_size;
    }

    *reinterpret_cast<unsigned char*>(target_block) = 1;

    if (requested_size != target_block_size)
    {
        void* remaining_block_from_target_block_size = reinterpret_cast<unsigned char*>(target_block) + get_small_free_metadata() + requested_size;

        std::cout << "bytes1: " << target_block << std::endl;
        std::cout << "bytes: " << remaining_block_from_target_block_size << std::endl;

        if (remaining_block_from_target_block_size == nullptr)
        {
            std::cout << "i hate programming" << std::endl;
        }

        *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(remaining_block_from_target_block_size) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*)) = blocks_sizes_difference;

        size_t remaining_block_size_from_target_block_size = get_size_block(remaining_block_from_target_block_size);

        std::cout << "remaining " << std::to_string(remaining_block_size_from_target_block_size) << std::endl;

        void** previous_to_remaining_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(remaining_block_from_target_block_size) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*));
        *previous_to_remaining_block = previous_to_target_block;

        void** next_to_remaining_block = reinterpret_cast<void **>(reinterpret_cast<unsigned char*>(remaining_block_from_target_block_size) + sizeof(unsigned char) + sizeof(unsigned char));
        *next_to_remaining_block = next_to_target_block;

//        if (previous_to_target_block != nullptr)
//        {
//            void** next_to_previous_to_target_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(previous_to_target_block) + sizeof(unsigned char) + sizeof(unsigned char));
//            *next_to_previous_to_target_block = remaining_block_from_target_block_size;
//        }
//
//        if (next_to_target_block != nullptr)
//        {
//            void** previous_to_next_target_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(next_to_target_block) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*));
//            *previous_to_next_target_block = remaining_block_from_target_block_size;
//        }

        std::cout << "hello" << std::endl;

        insert(remaining_block_from_target_block_size);

        std::cout << "hello" << std::endl;

        print_tree(target_block, 4);
    }

    delete_block(target_block);

    print_tree(target_block, 4);

    std::cout << "allocate has been great" << std::endl;

    return reinterpret_cast<unsigned char*>(target_block) + get_small_free_metadata();
}

void allocator_red_black_tree::insert(void* target_block) noexcept
{
    void* current_block = get_root();
    void* parent = nullptr;
    std::cout << "hello" << std::endl;

    size_t target_block_size = get_size_block(target_block);

    while(true)
    {
        parent = current_block;

        if (target_block_size < get_size_block(current_block))
        {
            current_block = get_left_subtree_block(current_block);
        }
        else
        {
            current_block = get_right_subtree_block(current_block);
        }

        if (current_block == nullptr)
        {
            break;
        }
    }

    std::cout << "hello3" << std::endl;

    // 0 - free, 1 - occupied
    // 0 - black, 1 - red;

    *reinterpret_cast<unsigned char*>(target_block) = 0;
    *reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char)) = 1;
    std::cout << "hello3" << std::endl;
    *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*)) = get_size_block(target_block);
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t)) = parent;
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(void*)) = nullptr;
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(void*) + sizeof(void*)) = nullptr;

    std::cout << "hello3" << std::endl;
    fix_red_black_tree(target_block);
}

void allocator_red_black_tree::fix_red_black_tree(void* target_block) noexcept
{
    void* parent_block_to_target_block = get_parent_block(target_block);
    void* grandparent_block_to_target_block = nullptr;
    void* uncle_block_to_target_block = nullptr;

    if (parent_block_to_target_block == nullptr)
    {
        *reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char)) = 0;
    }
    else
    {
        unsigned char colour_parent_to_target_block = get_colour_block(parent_block_to_target_block);

        if (colour_parent_to_target_block == 1) // red
        {
            grandparent_block_to_target_block = get_parent_block(parent_block_to_target_block);

            if (grandparent_block_to_target_block != nullptr)
            {
                if (get_right_subtree_block(grandparent_block_to_target_block) == parent_block_to_target_block)
                {
                    uncle_block_to_target_block = get_left_subtree_block(grandparent_block_to_target_block);
                }
                else
                {
                    uncle_block_to_target_block = get_right_subtree_block(grandparent_block_to_target_block);
                }

                unsigned char colour_uncle_to_target_block = get_colour_block(uncle_block_to_target_block);

                if (colour_uncle_to_target_block == 0) // black
                {
                    // if target is left son in parent
                    if (get_left_subtree_block(parent_block_to_target_block) == target_block)
                    {
                        small_left_rotation(grandparent_block_to_target_block);
                        // grandparent = red, parent = black
                        *reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned char*>(grandparent_block_to_target_block) + sizeof(unsigned char)) = 1;
                        *reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned char*>(parent_block_to_target_block) + sizeof(unsigned char)) = 0;
                    }
                    else
                    {
                        small_right_rotation(parent_block_to_target_block);
                        fix_red_black_tree(parent_block_to_target_block);
                    }
                }
                else // red
                {
                    // if uncle red => uncle = black, parent = black, grandparent = red

                    *reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned char*>(uncle_block_to_target_block) + sizeof(unsigned char)) = 0;
                    *reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned char*>(grandparent_block_to_target_block) + sizeof(unsigned char)) = 1;
                    *reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned char*>(parent_block_to_target_block) + sizeof(unsigned char)) = 0;

                    fix_red_black_tree(grandparent_block_to_target_block);
                }
            }
            else
            {
                *reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned char*>(parent_block_to_target_block) + sizeof(unsigned char)) = 0;
            }
        }
    }
}

void allocator_red_black_tree::deallocate(void *at)
{
    std::mutex* mutex_boundary_tags = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_boundary_tags);

    void* target_block = reinterpret_cast<unsigned char*>(at) - get_small_free_metadata();

    size_t target_block_size = get_size_block(target_block);

    *reinterpret_cast<unsigned char*>(target_block) = 0;

    void* previous_to_target_block = get_previous_free_block(target_block);


    void* next_to_target_block = get_next_free_block(target_block);

//    if (previous_to_target_block != nullptr && (*reinterpret_cast<unsigned char*>(previous_to_target_block) == 0))
//    {
//
//    }
//
//    if (next_to_target_block != nullptr && (*reinterpret_cast<unsigned char*>(next_to_target_block) == 0))

    // TODO хз как закончить

    insert(target_block);


}

inline void allocator_red_black_tree::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t)) = mode;
}

std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const noexcept
{
    // TODO хз че тут писать
}

inline std::string allocator_red_black_tree::get_typename() const noexcept
{
    return "ALLOCATOR_RED_BLACK_TREE: ";
}