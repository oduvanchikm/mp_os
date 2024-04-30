#include <allocator_red_black_tree.h>

allocator_red_black_tree::~allocator_red_black_tree()
{
    delete get_mutex();

    auto* parent_allocator = get_allocator();

    if (parent_allocator == nullptr)
    {
        ::operator delete(_trusted_memory);
    }
    else
    {
        parent_allocator->deallocate(_trusted_memory);
    }
}

allocator_red_black_tree::allocator_red_black_tree(
        size_t space_size,
        allocator *parent_allocator,
        logger *logger,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (logger != nullptr)
    {
        logger->debug(get_typename() + "start creating allocator");
    }
    std::cout << get_typename() + "start creating allocator" << std::endl;

    if (space_size < get_meta_size_aviable_block())
    {
        if (logger != nullptr)
        {
            logger->error(get_typename() + "size is too small");
        }

        throw std::logic_error(get_typename() + "size is too small");
    }

    auto common_size = space_size + get_ancillary_space_size();

    try
    {
        _trusted_memory = parent_allocator == nullptr
                          ? ::operator new(common_size)
                          : parent_allocator->allocate(common_size, 1);
    }
    catch (std::bad_alloc const &ex)
    {
        logger->error(get_typename() + "can't get memory");
        throw std::bad_alloc();
    }

    allocator** parent_allocator_space_address = reinterpret_cast<allocator **>(_trusted_memory);
    *parent_allocator_space_address = parent_allocator;

    class logger** logger_space_address = reinterpret_cast<class logger**>(parent_allocator_space_address + 1);
    *logger_space_address = logger;

    size_t *space_size_space_address = reinterpret_cast<size_t *>(logger_space_address + 1);
    *space_size_space_address = space_size;

    size_t *size_memory = reinterpret_cast<size_t *>(space_size_space_address + 1);
    *size_memory = space_size;

    allocator_with_fit_mode::fit_mode *fit_mode_space_address = reinterpret_cast<allocator_with_fit_mode::fit_mode *>(size_memory + 1);
    *fit_mode_space_address = allocate_fit_mode;

    std::mutex** mutex_space_address = reinterpret_cast<std::mutex** >(fit_mode_space_address + 1);
    *mutex_space_address = new std::mutex;

    void **root = reinterpret_cast<void **>(mutex_space_address + 1);
    *root = reinterpret_cast<void *>(root + 1);

    // free blocks metadata:
    // bool is_occupied
    // void* previous
    // void* next
    // bool colour
    // node* parent
    // node* left_subtree
    // node* right_subtree
    // size_t size

    void** previous_block_ptr =  reinterpret_cast<void**>(*root);
    *previous_block_ptr = nullptr;

    void** next_block_ptr = reinterpret_cast<void**>(previous_block_ptr + 1);
    *next_block_ptr = nullptr;

    // 0 - free, 1 - occupied

    bool* is_occupied_block = reinterpret_cast<bool*>(next_block_ptr + 1);
    *is_occupied_block = false;

    size_t* size_block = reinterpret_cast<size_t*>(is_occupied_block + 1);
    *size_block = space_size - get_meta_size_aviable_block();

    // 0 - black colour
    // 1 - red colour

    bool* is_red_color_block = reinterpret_cast<bool*>(size_block + 1);
    *is_red_color_block = 0;

    void** ptr_to_parent = reinterpret_cast<void**>(is_red_color_block + 1);
    *ptr_to_parent = nullptr;

    void** left_subtree_block_ptr = reinterpret_cast<void**>(ptr_to_parent + 1);
    *left_subtree_block_ptr = nullptr;

    void** right_subtree_block_ptr = reinterpret_cast<void**>(left_subtree_block_ptr + 1);
    *right_subtree_block_ptr = nullptr;

    if (logger != nullptr)
    {
        logger->debug(get_typename() + "allocator has created");
    }
    std::cout << get_typename() + "finish creating allocator" << std::endl;
}

[[nodiscard]] void *allocator_red_black_tree::allocate(size_t value_size, size_t values_count)
{
    trace_with_guard(get_typename() + "allocate method has started");
    std::cout << get_typename() + "allocate method has started" << std::endl;

    std::mutex* mutex_boundary_tags = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_boundary_tags);

    size_t requested_size = value_size * values_count;

    std::cout << requested_size << std::endl;

    if (requested_size < get_meta_size_occupied_block())
    {
        warning_with_guard(get_typename() + "requested size has changed");
        requested_size = get_meta_size_occupied_block();
    }

    std::cout << get_meta_size_occupied_block() << std::endl;

    allocator_with_fit_mode::fit_mode fit_mode = get_fit_mode();

    std::cout << "hi" << std::endl;

    void* target_block = nullptr;

    switch(fit_mode)
    {
        case allocator_with_fit_mode::fit_mode::the_worst_fit:
            target_block = get_worst_fit(requested_size);
            break;

        case allocator_with_fit_mode::fit_mode::the_best_fit:
            std::cout << "hi" << std::endl;
            target_block = get_best_fit(requested_size);
            std::cout << "hi" << std::endl;
            break;

        case allocator_with_fit_mode::fit_mode::first_fit:
            target_block = get_first_fit(requested_size);
            break;
    }

//    std::cout << "hi" << std::endl;

    if (target_block == nullptr)
    {
        std::logic_error(get_typename() + "nullptr");
    }

    std::cout << std::to_string(get_size_aviable_block(target_block)) << std::endl;

    void* next_to_target_block = get_next_block(target_block);
    void* prev_to_target_block = get_previous_block(target_block);

    std::cout << "bobobobo " << get_size_aviable_block(target_block) << std::endl;

    auto size_target_block = get_size_aviable_block(target_block) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool);

    auto blocks_sizes_difference = size_target_block - requested_size;

    if (blocks_sizes_difference < get_meta_size_aviable_block())
    {
        requested_size = size_target_block;
    }

    if (requested_size != size_target_block)
    {
        void* remaining_block_from_target_block_size = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(target_block) + get_meta_size_occupied_block() + requested_size);

        void **previous_block_to_remaining_block_from_target_block_size = reinterpret_cast<void **>(remaining_block_from_target_block_size);
        *previous_block_to_remaining_block_from_target_block_size = target_block;

        void **next_block_to_remaining_block_from_target_block_size = reinterpret_cast<void **>(previous_block_to_remaining_block_from_target_block_size + 1);
        *next_block_to_remaining_block_from_target_block_size = next_to_target_block;

        void** next_block_to_target_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*));
        *next_block_to_target_block = remaining_block_from_target_block_size;

        if (next_to_target_block != nullptr)
        {
            void** ptr_next_to_this = reinterpret_cast<void**>(next_to_target_block);
            *ptr_next_to_this = remaining_block_from_target_block_size;
        }

        insert_block_in_red_black_tree(remaining_block_from_target_block_size, blocks_sizes_difference - get_meta_size_aviable_block());
    }

    dispose_block_from_red_black_tree(target_block);

    bool* is_occup = reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*));
    *is_occup = 1;

    void** ptr_to_trusted = reinterpret_cast<void**>(is_occup + 1);
    *ptr_to_trusted = this->_trusted_memory;

    std::vector<allocator_test_utils::block_info> data = get_blocks_info();
    std::string data_str;

    for (block_info value : data)
    {
        std::string is_oc = value.is_block_occupied ? "YES" : "NO";
        data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
    }

    debug_with_guard(get_typename() + "state blocks: " + data_str);

    std::cout << get_typename() + "state blocks: " + data_str << std::endl;
    std::cout << get_typename() + "allocate method has finished" << std::endl;

    return reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) * 3 + sizeof(bool);
}

void allocator_red_black_tree::deallocate(void *at)
{
    std::mutex* mutex_boundary_tags = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_boundary_tags);

    if (at == nullptr)
    {
        return;
    }

    void *target_block = reinterpret_cast<void*>(reinterpret_cast<char *>(at) - sizeof(void*) * 3 - sizeof(bool));

    void* pointer = *reinterpret_cast<void**>(reinterpret_cast<char *>(target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool));

    if (pointer != _trusted_memory)
    {
        error_with_guard(get_typename() + "block doesn't belong this allocator!");
        throw std::logic_error("block doesn't belong this allocator!");
    }

    size_t size_target_block;

    void* next_block = get_next_block(target_block);
    void* prev_block = get_previous_block(target_block);

    if (next_block == nullptr)
    {
        size_target_block = reinterpret_cast<char*>(_trusted_memory) + get_ancillary_space_size() + get_all_size() - reinterpret_cast<char*>(at);
    }

    else size_target_block = reinterpret_cast<char*>(next_block) - reinterpret_cast<char*>(at);

    std::string result;

    auto* bytes = reinterpret_cast<unsigned char*>(at);

    for (int i = 0; i < size_target_block; i++)
    {
        result += std::to_string(static_cast<int>(bytes[i])) + " ";
    }

    size_t clear_size_target_without_meta_for_free = size_target_block + get_meta_size_occupied_block() - get_meta_size_aviable_block();

    bool done = 0;

    if (next_block && !is_occupied_block(next_block))
    {
        size_t size_next_block = get_size_aviable_block(next_block);
        size_t size_next_with_meta = size_next_block + get_meta_size_aviable_block();
        size_t new_size_target = clear_size_target_without_meta_for_free + size_next_with_meta;

        dispose_block_from_red_black_tree(next_block);
        insert_block_in_red_black_tree(target_block, new_size_target);

        void* next_next_block = get_next_block(next_block);
        void** ptr_this_next = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*));

        *ptr_this_next = next_next_block;

        if (next_next_block != nullptr)
        {
            void **ptr_next_to_this = reinterpret_cast<void**>(next_next_block);
            *ptr_next_to_this = target_block;
        }

        done = 1;

        clear_size_target_without_meta_for_free = new_size_target;
    }

    if (prev_block && !is_occupied_block(prev_block))
    {
        size_t size_prev_block = get_size_aviable_block(prev_block);

        size_t new_size_target = clear_size_target_without_meta_for_free + size_prev_block + get_meta_size_aviable_block();
        if (!is_occupied_block(target_block))
        {
            dispose_block_from_red_black_tree(target_block);
        }

        dispose_block_from_red_black_tree(prev_block);
        insert_block_in_red_black_tree(prev_block, new_size_target);

        next_block = get_next_block(target_block);

        void** ptr_this_next = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(prev_block) + sizeof(void*));
        *ptr_this_next = next_block;

        if (next_block != nullptr)
        {
            void **ptr_next_to_this = reinterpret_cast<void**>(next_block);
            *ptr_next_to_this = prev_block;
        }

        debug_with_guard(get_typename() + "finish dellocate memory");

        std::vector<allocator_test_utils::block_info> data = get_blocks_info();

        std::string data_str;

        for (block_info value : data)
        {
            std::string is_oc = value.is_block_occupied ? "YES" : "NO";
            data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
        }
        debug_with_guard(get_typename() + "state blocks: " + data_str);

        return;
    }

    if (!done)
    {
        insert_block_in_red_black_tree(target_block, clear_size_target_without_meta_for_free);
    }

    std::vector<allocator_test_utils::block_info> data = get_blocks_info();
    std::string data_str;

    for (block_info value : data)
    {
        std::string is_oc = value.is_block_occupied ? "YES" : "NO";
        data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
    }

    debug_with_guard(get_typename() + "state blocks: " + data_str);

    trace_with_guard(get_typename() + "deallocate method has finished");
}

inline void allocator_red_black_tree::set_fit_mode(
        allocator_with_fit_mode::fit_mode mode)
{
    std::mutex* mutex_boundary_tags = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_boundary_tags);

    *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t)) = mode;
}

inline allocator *allocator_red_black_tree::get_allocator() const
{
    return *reinterpret_cast<allocator**>(_trusted_memory);
}

size_t allocator_red_black_tree::get_all_size() const
{
    return *reinterpret_cast<size_t*>(reinterpret_cast<char *>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t));
}

inline logger *allocator_red_black_tree::get_logger() const
{
    return *reinterpret_cast<logger **>(reinterpret_cast<allocator **>(_trusted_memory) + 1);
}

inline std::string allocator_red_black_tree::get_typename() const noexcept
{
    return "ALLOCATOR_RED_BLACK_TREE: ";
}

size_t allocator_red_black_tree::get_ancillary_space_size() const noexcept
{
    return sizeof(logger *) + sizeof(allocator *) + sizeof(size_t) +  sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(void *); //sizeof(sem_t*)
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
    return *reinterpret_cast<std::mutex **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t)
                                       + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));;
}

size_t allocator_red_black_tree::get_meta_size_aviable_block() const noexcept
{
    return sizeof(void*) * 2 + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) * 3;
}

size_t allocator_red_black_tree::get_meta_size_occupied_block() const noexcept
{
    return sizeof(void*) * 3 + sizeof(bool);
}

void* allocator_red_black_tree::get_previous_block(void* block) const
{
    return *reinterpret_cast<void**>(block);
}

void* allocator_red_black_tree::get_next_block(void* block) const
{
    return *reinterpret_cast<void**>(reinterpret_cast<void**>(block) + 1);
}

bool allocator_red_black_tree::is_occupied_block(void* block) const
{
    return *reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) * 2);
}

size_t allocator_red_black_tree::get_size_aviable_block(void* block) const
{
    return *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) * 2 + sizeof(bool));
}

bool allocator_red_black_tree::is_color_red(void* block) const
{
    return *reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t));
}

void* allocator_red_black_tree::get_parent(void* block) const
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool));
}

void* allocator_red_black_tree::get_left_subtree_block(void* block) const
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
}

void* allocator_red_black_tree::get_right_subtree_block(void* block) const
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
}

void allocator_red_black_tree::change_color(void* block)
{
    bool* is_red_color_block = reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t));
    (*is_red_color_block) ? *is_red_color_block = 0 : *is_red_color_block = 1;
}

void* allocator_red_black_tree::get_best_fit(size_t size)
{
    trace_with_guard(get_typename() + "get_best_fit has started");
    void* current = get_root();

    if (!current)
    {
        return nullptr;
    }

    std::cout << get_size_aviable_block(current) << std::endl;

    size_t current_block_size = get_size_aviable_block(current) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool);
    std::cout << current_block_size << std::endl;

    while (current_block_size < size)
    {
        current = get_right_subtree_block(current);
        if (current)
        {
            current_block_size = get_size_aviable_block(current) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool);
            std::cout << "b " << current_block_size << std::endl;
        }
        else
        {
            break;
        }
    }

    if (!current)
    {
        return nullptr;
    }

    while(get_left_subtree_block(current) != nullptr)
    {
        void* left = get_left_subtree_block(current);
        auto size_left =  get_size_aviable_block(left) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool);
        if (size_left >= size)
        {
            current = left;
        }
        else
        {
            break;
        }
    }

    return current;
}

void* allocator_red_black_tree::get_worst_fit(size_t size)
{
    trace_with_guard(get_typename() + "get_worst_fit has started");

    void* current = get_root();

    while (get_right_subtree_block(current))
    {
        current = get_right_subtree_block(current);
    }

    size_t current_block_size = reinterpret_cast<size_t>(reinterpret_cast<unsigned char*>(current) + get_size_aviable_block(current) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool));

    return (current_block_size >= size) ? current : nullptr;
}
void* allocator_red_black_tree::get_first_fit(size_t size)
{
    trace_with_guard(get_typename() + "get_first_fit has started");

    void* current = get_root();

    size_t current_block_size = reinterpret_cast<size_t>(reinterpret_cast<unsigned char*>(current) + get_size_aviable_block(current) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool));

    while (current && current_block_size < size)
    {
        current = get_right_subtree_block(current);

        if (current)
        {
            current_block_size = reinterpret_cast<size_t>(reinterpret_cast<unsigned char*>(current) + get_size_aviable_block(current) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool));
        }
    }
    return current;
}

//-------------------------------------------------INSERT------------------------------------------------------

void allocator_red_black_tree::insert_block_in_red_black_tree(void* target_block, size_t size_to_target_block)
{
    trace_with_guard(get_typename() + "insert method has started");
    std::cout << get_typename() + "insert method has started" << std::endl;

    void* parent = nullptr;
    void* current_block = get_root();

    while (true)
    {
        parent = current_block;

        if (size_to_target_block < get_size_aviable_block(current_block))
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

    *reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*)) = 0;
    *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool)) = size_to_target_block;
    *reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t)) = 1;
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(void*)) = parent;
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(void*) + sizeof(void*)) = nullptr;
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(void*) + sizeof(void*) + sizeof(void*)) = nullptr;

    size_t size_parent_block = get_size_aviable_block(parent);
    bool color_parent_block = is_color_red(parent);

    if (size_to_target_block < size_parent_block)
    {
        void** left_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
        *left_subtree = target_block;
    }
    else
    {
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*)) = target_block;
    }

    trace_with_guard(get_typename() + "insert method has finished");
    std::cout << get_typename() + "insert method has finished" << std::endl;

    fix_red_black_tree_after_insert(target_block, color_parent_block);
}

void allocator_red_black_tree::fix_red_black_tree_after_insert(void* target_block, bool color_parent_block)
{
    trace_with_guard(get_typename() + "fix tree after insert method has started");
    std::cout << get_typename() + "fix tree after insert method has started" << std::endl;

    void* parent_to_target_block = get_parent(target_block);

    std::cout << "hohohohhohohohohoh" << std::endl;

    if (parent_to_target_block == nullptr)
    {
        if (is_color_red(target_block))
        {
            change_color(target_block);
        }

        std::cout << get_typename() + "target block is root, color is black" << std::endl;

        trace_with_guard(get_typename() + "fix tree after insert method has finished");
        std::cout << get_typename() + "fix tree after insert method has finished" << std::endl;

        return;
    }

    if (color_parent_block == 0)
    {
        std::cout << get_typename() + "parent is black, it's true" << std::endl;
        trace_with_guard(get_typename() + "fix tree after insert method has finished");
        std::cout << get_typename() + "fix tree after insert method has finished" << std::endl;
        return;
    }

    std::cout << "tell me why it sooo difficult difficult" << std::endl;

    void* grandparent_to_target_block = get_parent(parent_to_target_block);
//    void* grandparent_to_target_block = *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_to_target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool));

    std::cout << "difficult" << std::endl;

    void* uncle_to_target_block = nullptr;

    if (get_left_subtree_block(grandparent_to_target_block) == parent_to_target_block)
    {
        uncle_to_target_block = get_right_subtree_block(grandparent_to_target_block);
    }
    else
    {
        uncle_to_target_block = get_left_subtree_block(grandparent_to_target_block);
    }

    bool color_uncle_target_block = is_color_red(uncle_to_target_block);

    if (get_left_subtree_block(grandparent_to_target_block) == parent_to_target_block)
    {
        if (uncle_to_target_block && color_uncle_target_block == 1)
        {
            change_color(parent_to_target_block);
            change_color(uncle_to_target_block);
            change_color(grandparent_to_target_block);
            fix_red_black_tree_after_insert(grandparent_to_target_block, color_parent_block);
        }
        else
        {
            change_color(target_block);
            change_color(grandparent_to_target_block);
            if (get_right_subtree_block(parent_to_target_block) == target_block)
            {
                left_rotate(parent_to_target_block);
            }

            right_rotate(grandparent_to_target_block);
        }
    }
    else
    {
        if (uncle_to_target_block && is_color_red(uncle_to_target_block))
        {
            change_color(parent_to_target_block);
            change_color(uncle_to_target_block);
            change_color(grandparent_to_target_block);
            fix_red_black_tree_after_insert(grandparent_to_target_block, color_parent_block);
        }
        else
        {
            change_color(target_block);
            change_color(grandparent_to_target_block);
            if (get_left_subtree_block(parent_to_target_block) == target_block)
            {
                right_rotate(parent_to_target_block);
            }

            left_rotate(grandparent_to_target_block);
        }
    }

    trace_with_guard(get_typename() + "fix tree after insert method has finished");
    std::cout << get_typename() + "fix tree after insert method has finished" << std::endl;
}

//------------------------------------------------DISPOSE----------------------------------------------

void allocator_red_black_tree::dispose_block_from_red_black_tree(void* target_block)
{
    trace_with_guard(get_typename() + "dispose method has started");

    bool is_red_color_block = is_color_red(target_block);
    void* parent_to_target_block = get_parent(target_block);
    bool is_left_child = false;

    bool is_black_target_block_for_fix_red_black_tree = false;

    if (get_right_subtree_block(target_block) == nullptr && get_left_subtree_block(target_block) == nullptr)
    {
        trace_with_guard(get_typename() + "block without subtrees");
        std::cout << get_typename() + "block without subtrees" << std::endl;
        if (is_red_color_block)
        {
            if (get_right_subtree_block(parent_to_target_block) == target_block)
            {
                void** right_subtree_to_parent_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_to_target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
                *right_subtree_to_parent_block = nullptr;
            }
            else
            {
                void** left_subtree_to_parent_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_to_target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
                *left_subtree_to_parent_block = nullptr;
            }

            trace_with_guard(get_typename() + "disposed red block without subtrees");
            std::cout << get_typename() + "disposed red block without subtrees" << std::endl;
        }
        else
        {
            is_black_target_block_for_fix_red_black_tree = true;

            if (get_right_subtree_block(parent_to_target_block) == target_block)
            {
                void** right_subtree_to_parent_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_to_target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
                *right_subtree_to_parent_block = nullptr;
            }
            else
            {
                void** left_subtree_to_parent_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_to_target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
                *left_subtree_to_parent_block = nullptr;
                is_left_child = true;
            }

            trace_with_guard(get_typename() + "disposed black block without subtrees");
            std::cout << get_typename() + "disposed black block without subtrees" << std::endl;
        }
    }
    else if (get_right_subtree_block(target_block) != nullptr || get_left_subtree_block(target_block) != nullptr)
    {
        trace_with_guard(get_typename() + "block with 1 subtree");
        std::cout << get_typename() + "block with 1 subtree" << std::endl;

        if (!is_red_color_block)
        {
//            void *child_to_target_block = (get_left_subtree_block(target_block) != nullptr) ? get_left_subtree_block(
//                    target_block) : get_right_subtree_block(target_block);

            void *child_to_target_block = nullptr;

            if (get_left_subtree_block(target_block) != nullptr)
            {
                child_to_target_block = get_left_subtree_block(target_block);
            }
            if (get_right_subtree_block(target_block) != nullptr)
            {
                child_to_target_block = get_right_subtree_block(target_block);
            }

//            bool color_child = reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(child_to_target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t));

            bool* color_child_ptr = reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(child_to_target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t));
            *color_child_ptr = 0;

            std::cout << "child " << *color_child_ptr << std::endl;

            bool* color_target_block = reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(child_to_target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t));
            *color_target_block = 1;

            std::cout << "target " << *color_target_block << std::endl;

            if (parent_to_target_block != nullptr)
            {
                if (get_left_subtree_block(parent_to_target_block) == target_block)
                {
                    void **left_subtree_to_parent = reinterpret_cast<void **>(
                            reinterpret_cast<unsigned char *>(parent_to_target_block) + sizeof(void *) +
                            sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void *));
                    *left_subtree_to_parent = child_to_target_block;
                }
                else
                {
                    void **right_subtree_to_parent = reinterpret_cast<void **>(
                            reinterpret_cast<unsigned char *>(parent_to_target_block) + sizeof(void *) +
                            sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void *) +
                            sizeof(void *));
                    *right_subtree_to_parent = child_to_target_block;
                }

                void** new_parent_to_child = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(child_to_target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool));
                *new_parent_to_child = parent_to_target_block;
            }
            else
            {
                std::cout << get_typename() + "parent to target == nullptr" << std::endl;

                void** new_parent_to_child = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(child_to_target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool));
                *new_parent_to_child = nullptr;
            }
        }

        trace_with_guard(get_typename() + "disposed block with 1 subtree");
        std::cout << get_typename() + "disposed block with 1 subtree" << std::endl;
    }
    else
    {
        trace_with_guard(get_typename() + "block with 2 subtrees");
        std::cout << "block with 2 subtrees" << std::endl;

        void* max_node_in_left_subtree = get_left_subtree_block(target_block);

        while (get_right_subtree_block(max_node_in_left_subtree) != nullptr)
        {
            max_node_in_left_subtree = get_right_subtree_block(max_node_in_left_subtree);
        }

        void* parent_to_max_node_in_left_subtree = get_parent(max_node_in_left_subtree);

        void* left_subtree_for_target_block = get_left_subtree_block(target_block);
        void* right_subtree_for_target_block = get_right_subtree_block(target_block);

        void* left_subtree_for_max = get_left_subtree_block(max_node_in_left_subtree);
        void* right_subtree_for_max = get_right_subtree_block(max_node_in_left_subtree);

        if (is_color_red(target_block) != is_color_red(max_node_in_left_subtree))
        {
            change_color(target_block);
            change_color(max_node_in_left_subtree);
        }

        void** right_child_for_parent_to_max_node_in_left_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_to_max_node_in_left_subtree) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
        *right_child_for_parent_to_max_node_in_left_subtree = target_block;

        void** new_parent_for_target_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(max_node_in_left_subtree) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool));
        *new_parent_for_target_block = parent_to_max_node_in_left_subtree;

        if (get_left_subtree_block(max_node_in_left_subtree) != nullptr)
        {
            void** left_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
            *left_subtree = left_subtree_for_max;
            void** right_subtree = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
            *right_subtree = nullptr;
        }

        if (parent_to_target_block != nullptr)
        {
            if (get_right_subtree_block(parent_to_target_block) == target_block)
            {
                void** right = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_to_target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
                *right = max_node_in_left_subtree;
            }
            else
            {
                void** left = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
                *left = max_node_in_left_subtree;
            }

            void** new_parent_to_max_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(max_node_in_left_subtree) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool));
            *new_parent_to_max_block = parent_to_target_block;
        }

        void** new_left_subtree_to_max = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
        *new_left_subtree_to_max = left_subtree_for_target_block;

        void** new_right_subtree_to_max = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_to_target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
        *new_right_subtree_to_max = right_subtree_for_target_block;

        dispose_block_from_red_black_tree(target_block);

        trace_with_guard(get_typename() + "disposed block with 2 subtrees");
        std::cout << get_typename() + "disposed block with 2 subtrees" << std::endl;
    }

    if (is_black_target_block_for_fix_red_black_tree)
    {
        fix_red_black_tree_after_dispose(target_block, is_left_child);
    }

    trace_with_guard(get_typename() + "dispose method has finished");
    std::cout << get_typename() + "dispose method has finished" << std::endl;

}

void allocator_red_black_tree::fix_red_black_tree_after_dispose(void* target_block, bool is_left_child)
{
    trace_with_guard(get_typename() + "fix red black tree after dispose has started");
    std::cout << get_typename() + "fix red black tree after dispose has started" << std::endl;

    if (target_block == nullptr || get_parent(target_block) == nullptr)
    {
        return;
    }

    void* parent_to_target_block = get_parent(target_block);
    void* brother_to_target_block = (is_left_child) ? get_right_subtree_block(parent_to_target_block) : get_left_subtree_block(parent_to_target_block);

    if (!is_color_red(brother_to_target_block))
    {
        void* left_child_to_brother_to_target_block = get_left_subtree_block(brother_to_target_block);
        void* right_child_to_brother_to_target_block = get_right_subtree_block(brother_to_target_block);

        if ((left_child_to_brother_to_target_block != nullptr && is_color_red(left_child_to_brother_to_target_block) ||
                (right_child_to_brother_to_target_block != nullptr && is_color_red(right_child_to_brother_to_target_block))))
        {
            if (is_left_child)
            {
                if (right_child_to_brother_to_target_block != nullptr && is_color_red(right_child_to_brother_to_target_block))
                {
                    if (is_color_red(brother_to_target_block) != is_color_red(parent_to_target_block))
                    {
                        change_color(brother_to_target_block);
                    }

                    change_color(right_child_to_brother_to_target_block);
                    change_color(parent_to_target_block);
                    left_rotate(parent_to_target_block);

                    return;
                }
                if ((left_child_to_brother_to_target_block != nullptr && is_color_red(left_child_to_brother_to_target_block) &&
                        (right_child_to_brother_to_target_block != nullptr && !is_color_red(right_child_to_brother_to_target_block))))
                {
                    change_color(brother_to_target_block);
                    change_color(left_child_to_brother_to_target_block);
                    right_rotate(brother_to_target_block);
                    fix_red_black_tree_after_dispose(target_block, is_left_child);
                }
            }
            else
            {
                if (left_child_to_brother_to_target_block != nullptr && is_color_red(left_child_to_brother_to_target_block))
                {
                    if (is_color_red(brother_to_target_block) != is_color_red(parent_to_target_block))
                    {
                        change_color(brother_to_target_block);
                    }

                    change_color(left_child_to_brother_to_target_block);
                    change_color(parent_to_target_block);
                    right_rotate(parent_to_target_block);
                    return;
                }
                if ((right_child_to_brother_to_target_block != nullptr && is_color_red(right_child_to_brother_to_target_block) &&
                        (left_child_to_brother_to_target_block != nullptr && !is_color_red(left_child_to_brother_to_target_block))))
                {
                    change_color(brother_to_target_block);
                    change_color(right_child_to_brother_to_target_block);
                    left_rotate(brother_to_target_block);
                    fix_red_black_tree_after_dispose(target_block, is_left_child);
                }
            }
        }
        if ((left_child_to_brother_to_target_block != nullptr && !is_color_red(left_child_to_brother_to_target_block) &&
                (right_child_to_brother_to_target_block != nullptr && !is_color_red(right_child_to_brother_to_target_block))))
        {
            change_color(brother_to_target_block);
            if (is_color_red(parent_to_target_block))
            {
                change_color(parent_to_target_block);
                return;
            }
            else
            {
                if (is_left_child)
                {
                    void *grandpa = get_parent(parent_to_target_block);
                    if (grandpa && get_left_subtree_block(grandpa) == parent_to_target_block)
                    {
                        fix_red_black_tree_after_dispose(parent_to_target_block, 1);
                    }
                    else if (grandpa)
                    {
                        fix_red_black_tree_after_dispose(parent_to_target_block, 0);
                    }
                }
                else
                {
                    void *grandpa = get_parent(parent_to_target_block);
                    if (grandpa && get_right_subtree_block(grandpa) == parent_to_target_block)
                    {
                        fix_red_black_tree_after_dispose(parent_to_target_block, 1);
                    }
                    else if (grandpa)
                    {
                        fix_red_black_tree_after_dispose(parent_to_target_block, 0);
                    }
                }
            }
        }
    }
    else
    {
        change_color(parent_to_target_block);
        change_color(brother_to_target_block);
        if (is_left_child)
        {
            left_rotate(parent_to_target_block);
            fix_red_black_tree_after_dispose(target_block, 1);
        }
        else {
            right_rotate(parent_to_target_block);
            fix_red_black_tree_after_dispose(target_block, 0);
        }
    }
}

//----------------------------------------------------ROTATE------------------------------------------------------------

void allocator_red_black_tree::left_rotate(void* block)
{
    trace_with_guard(get_typename() + "start left rotate");
    std::cout << get_typename() + "start left rotate" << std::endl;

    void* parent_old_root = get_parent(block);
    void *new_root =  get_right_subtree_block(block);

    void* left_root_child = get_left_subtree_block(new_root);
    void** right_child_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
    *right_child_block = left_root_child;

    if (left_root_child)
    {
        *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(left_root_child) + sizeof(void *) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = block;
    }

    void** left_child_new_root = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
    *left_child_new_root = block;
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = new_root;

    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(new_root) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = parent_old_root;
    if (!parent_old_root)
    {
        *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*)) = new_root;
    }
    else
    {
        if (block == get_right_subtree_block(parent_old_root))
        {
            void** right_child_parent = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_old_root) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
            *right_child_parent = new_root;
        }
        else
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_old_root) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*)) = new_root;
        }
    }

    trace_with_guard(get_typename() + "finish left rotate");
    std::cout << get_typename() + "finish left rotate" << std::endl;
}

void allocator_red_black_tree::right_rotate(void* block)
{
    trace_with_guard(get_typename() + "start right rotate");
    std::cout << get_typename() + "start right rotate" << std::endl;

    void* parent_old_root = get_parent(block);

    void *new_root =  get_left_subtree_block(block);

    void* right_root_child = get_right_subtree_block(new_root);

    void** left_child_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
    *left_child_block = right_root_child;

    if (right_root_child)
    {
        *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(right_root_child) + sizeof(void *) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = block;
    }

    void** right_child_new_root = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
    *right_child_new_root = block;

    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = new_root;

    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(new_root) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = parent_old_root;

    if (!parent_old_root)
    {
        *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*)) = new_root;
    }
    else
    {
        if (block == get_right_subtree_block(parent_old_root))
        {
            void** right_child_parent = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_old_root) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
            *right_child_parent = new_root;
        }
        else
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_old_root) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*)) = new_root;
        }
    }

    trace_with_guard(get_typename() + "finish right rotate");
    std::cout << get_typename() + "finish right rotate" << std::endl;
}

std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const noexcept
{
    trace_with_guard(get_typename() + "get blocks info method has started");
    std::cout << get_typename() + "get blocks info method has started" << std::endl;

    std::vector<allocator_test_utils::block_info> data;

    void** current_block_l = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(std::mutex*) + sizeof(allocator_with_fit_mode::fit_mode));
    void* current_block = reinterpret_cast<void **>(current_block_l + 1);

    while(current_block != nullptr)
    {
        allocator_test_utils::block_info value;

        value.is_block_occupied = is_occupied_block(current_block);

        if (value.is_block_occupied)
        {
            if (get_next_block(current_block) == nullptr)
            {
                value.block_size = reinterpret_cast<char *>(_trusted_memory) + get_ancillary_space_size() + get_all_size() - reinterpret_cast<char *>(current_block) - get_meta_size_occupied_block();
            }
            else value.block_size = reinterpret_cast<char *>(get_next_block(current_block)) - reinterpret_cast<char *>(current_block) - get_meta_size_occupied_block();
        }
        else
        {
            value.block_size = get_size_aviable_block(current_block);
        }
        data.push_back(value);

        current_block = get_next_block(current_block);
    }

    trace_with_guard(get_typename() + "get blocks info method has finished");
    std::cout << get_typename() + "get blocks info method has finished" << std::endl;
    return data;
}
