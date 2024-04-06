#include <not_implemented.h>
#include "../include/allocator_sorted_list.h"

allocator_sorted_list::allocator_sorted_list(allocator_sorted_list &&other) noexcept :
        _trusted_memory(other._trusted_memory)
{
    other._trusted_memory = nullptr;
}

allocator_sorted_list &allocator_sorted_list::operator=(allocator_sorted_list &&other) noexcept
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

allocator_sorted_list::~allocator_sorted_list()
{
    std::mutex* mutex_buddy_system = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_buddy_system);
    auto* allocator = get_allocator();
    auto* logger = get_logger();

    if (allocator == nullptr)
    {
        ::operator delete(_trusted_memory);
    }
    else
    {
        allocator->deallocate(_trusted_memory);
    }

    delete mutex_buddy_system;


    if (logger != nullptr)
    {
        logger->trace(get_typename() + "allocator is deleted");
    }
}

size_t allocator_sorted_list::get_small_metadata() const noexcept
{
    return sizeof(size_t) + sizeof(void*);
}

size_t allocator_sorted_list::get_ancillary_space_size() const noexcept
{
    return sizeof(logger*) + sizeof(allocator*) + sizeof(size_t) + sizeof(void*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t);
}

std::mutex* allocator_sorted_list::get_mutex() const noexcept
{
    return *reinterpret_cast<std::mutex**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
}

allocator_with_fit_mode::fit_mode allocator_sorted_list::get_fit_mode() const noexcept
{
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t));
}

void *allocator_sorted_list::get_first_aviable_block() const noexcept
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
}

size_t allocator_sorted_list::get_aviable_block_size(void *block_address) const noexcept
{
    return *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(block_address) + sizeof(void*));
}

void *allocator_sorted_list::get_aviable_block_next_block_address(void *block_address) noexcept
{
    return *reinterpret_cast<void**>(block_address);
}

inline allocator *allocator_sorted_list::get_allocator() const
{
    return *reinterpret_cast<allocator**>(_trusted_memory);
}

allocator_sorted_list::allocator_sorted_list(
    size_t space_size,
    allocator *parent_allocator,
    logger *logger,
    allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (logger != nullptr)
    {
        logger->trace(get_typename() + "the beginning of the constructor's work, allocator start creating");
        std::cout << get_typename() + "the beginning of the constructor's work, allocator start creating" << std::endl;
    }

    if (space_size < get_small_metadata())
    {
        if (logger != nullptr)
        {
            logger->error(get_typename() + "can't initialize allocator instance");
        }
        throw std::logic_error(get_typename() + "can't initialize allocator instance");
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
        logger->error(get_typename() + "error with allocate memory");
    }

    allocator** parent_allocator_space_address = reinterpret_cast<allocator**>(_trusted_memory);
    *parent_allocator_space_address = parent_allocator;

    class logger** logger_space_address = reinterpret_cast<class logger**>(parent_allocator_space_address + 1);
    *logger_space_address = logger;

    size_t* space_size_space_address = reinterpret_cast<size_t*>(logger_space_address + 1);
    *space_size_space_address = space_size;

    allocator_with_fit_mode::fit_mode* fit_mode_space_address = reinterpret_cast<allocator_with_fit_mode::fit_mode*>(space_size_space_address + 1);
    *fit_mode_space_address = allocate_fit_mode;

    std::mutex** mutex_space_address = reinterpret_cast<std::mutex**>(fit_mode_space_address + 1);
    auto* mutex_buddy_system = new std::mutex;
    *mutex_space_address = mutex_buddy_system;

    size_t* available_block = reinterpret_cast<size_t*>(mutex_space_address + 1);
    *available_block = space_size;

    void** first_block_address_space_address = reinterpret_cast<void **>(available_block + 1);
    *first_block_address_space_address = reinterpret_cast<void *>(first_block_address_space_address + 1);

    *reinterpret_cast<void**>(*first_block_address_space_address) = nullptr;
    *reinterpret_cast<size_t*>(reinterpret_cast<void **>(*first_block_address_space_address) + 1) = space_size - sizeof(void*) - sizeof(size_t);

    if (logger != nullptr)
    {
        logger->trace(get_typename() + "allocator is created");
        std::cout << get_typename() + "allocator has created" << std::endl;
    }
}

[[nodiscard]] void *allocator_sorted_list::allocate(size_t value_size, size_t values_count)
{
    logger* log = get_logger();
    std::mutex* mutex_buddy_system = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_buddy_system);

    if (log != nullptr)
    {
        log->trace(get_typename() + "method allocate has started");
    }

    auto requested_size = values_count * value_size;

    if (requested_size < get_small_metadata())
    {
        requested_size = get_small_metadata();
        warning_with_guard(get_typename() + "request space size was changed");
    }

    allocator_with_fit_mode::fit_mode fit_mode = get_fit_mode();

    void* target_block = nullptr;
    void* previous_to_target_block = nullptr;
    void* next_to_target_block = nullptr;
    {
        void* previous_block = nullptr;
        void* current_block = get_first_aviable_block();

        while (current_block != nullptr)
        {
//            size_t current_block_size = get_aviable_block_size(current_block);

            size_t current_block_size = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(current_block) + sizeof(void*));

            std::cout << "block size " << current_block_size << std::endl;

            if (current_block_size >= requested_size &&
                fit_mode == allocator_with_fit_mode::fit_mode::first_fit ||
                fit_mode == allocator_with_fit_mode::fit_mode::the_best_fit &&
                (target_block == nullptr ||
                 get_aviable_block_size(target_block) > current_block_size) ||
                fit_mode == allocator_with_fit_mode::fit_mode::the_worst_fit &&
                (target_block == nullptr ||
                 get_aviable_block_size(target_block) < current_block_size))
            {
                previous_to_target_block = previous_block;
                target_block = current_block;
                next_to_target_block = get_aviable_block_next_block_address(current_block);

                if (fit_mode == allocator_with_fit_mode::fit_mode::first_fit)
                {
                    break;
                }

            }
            previous_block = current_block;
            current_block = get_aviable_block_next_block_address(current_block);
        }
    }

    if (target_block == nullptr)
    {
        error_with_guard(get_typename() + "can't allocate");
        throw std::bad_alloc();
    }

    if (previous_to_target_block == nullptr)
    {
        size_t blocks_sizes_difference = get_aviable_block_size(target_block) - requested_size;
        void* next_available_block = get_aviable_block_next_block_address(target_block);

        if (blocks_sizes_difference > 0 && blocks_sizes_difference <  get_small_metadata())
        {
            requested_size = get_aviable_block_size(target_block);

            void** first_available_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
            *first_available_block = next_available_block;

            warning_with_guard(get_typename() + "request space size was changed");
        }
        else
        {
            size_t target_block_size = get_aviable_block_size(target_block);

            void* new_available_block = reinterpret_cast<unsigned char*>(target_block) + target_block_size + sizeof(size_t) + sizeof(void*);
            size_t* size_new_available_block = reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(new_available_block) + 1);

            void** first_available_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
            *first_available_block = new_available_block;
        }

        next_available_block = _trusted_memory;
    }
    else
    {
        auto blocks_sizes_difference = get_aviable_block_size(target_block) - requested_size;
        void* next_available_block = get_aviable_block_next_block_address(target_block);

        if (blocks_sizes_difference > 0 && blocks_sizes_difference <  get_small_metadata())
        {
            requested_size = get_aviable_block_size(target_block);
            void** new_next_for_previous_available_block = reinterpret_cast<void**>(previous_to_target_block);

            *new_next_for_previous_available_block = next_available_block;

            warning_with_guard(get_typename() + "request space size was changed");
        }
        else
        {
            size_t new_available_block_size = requested_size;
            void* new_next_after_target_available_block = reinterpret_cast<unsigned char *>(target_block) + new_available_block_size;

            void** new_next_for_previous_available_block = reinterpret_cast<void **>(previous_to_target_block);
            *new_next_for_previous_available_block = new_next_after_target_available_block;
        }

        next_available_block = _trusted_memory;
    }

    size_t size_before = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *));
    size_t* size_space = reinterpret_cast<size_t*>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *));

    *size_space = size_before - requested_size - sizeof(size_t) - sizeof(void*);

    get_blocks_info();

    if (log != nullptr)
    {
        log->trace(get_typename() + "method allocate has finished");
        std::cout << get_typename() + "method allocate has finished" << std::endl;
    }

    return reinterpret_cast<unsigned char*>(target_block) + get_small_metadata();
}

void allocator_sorted_list::deallocate(void *at)
{
    logger* log = get_logger();
    std::mutex* mutex_buddy_system = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_buddy_system);

    if (at == nullptr)
    {
        return;
    }

    if (log != nullptr)
    {
        log->trace(get_typename() + "method deallocate has started");
        std::cout << get_typename() + "method deallocate has started" << std::endl;
    }

    void* target_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(at) - sizeof(void*) - sizeof(size_t));
    size_t target_block_size = get_aviable_block_size(target_block);

    void* previous_block = nullptr;
    void* next_block = get_first_aviable_block();

    while (next_block != nullptr && next_block < target_block)
    {
        previous_block = next_block;
        next_block = get_aviable_block_next_block_address(next_block);
    }

    // первый блок занят
    if (previous_block == nullptr && next_block != nullptr)
    {
        void** first_available_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
        *first_available_block = target_block;

        void** next_block_to_target_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block));
        *next_block_to_target_block = next_block;

        size_t block_size = target_block_size + sizeof(void*) + sizeof(size_t);

        if (reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + block_size) == next_block)
        {
            *reinterpret_cast<size_t*>(target_block) = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*)) + block_size;

            void** next_next_block_to_target_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(next_block));
            *next_next_block_to_target_block = next_block;
        }
    }
    // занятый блок в середине
    else if (previous_block != nullptr && next_block != nullptr)
    {
        void* next_to_prev_block = get_aviable_block_next_block_address(previous_block);

        void** next_to_previous_available_block = reinterpret_cast<void**>(previous_block);
        *next_to_previous_available_block = target_block;

        void** next_to_target_available_block = reinterpret_cast<void**>(target_block);
        *next_to_target_available_block = next_to_prev_block;

        size_t previous_block_size = get_aviable_block_size(previous_block) + get_small_metadata();

        if (reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(previous_block) + previous_block_size) == target_block)
        {
            *reinterpret_cast<size_t*>(target_block) = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(previous_block) + sizeof(void*)) + previous_block_size + target_block_size;

            void** next_to_target_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(previous_block));
            *next_to_target_block = target_block;
        }

        size_t target_block_size_new = get_aviable_block_size(target_block) + get_small_metadata();

        if (reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + target_block_size_new) == next_block)
        {
            *reinterpret_cast<size_t*>(target_block) = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*)) + target_block_size_new;

            void** next_next_block_to_target_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(next_block));
            *next_next_block_to_target_block = next_block;
        }
    }
    // занятый блок в конце
    else if (previous_block != nullptr && next_block == nullptr)
    {
        void** next_to_previous_available_block = reinterpret_cast<void**>(previous_block);
        *next_to_previous_available_block = target_block;

        void** next_to_target_available_block = reinterpret_cast<void**>(target_block);
        *next_to_target_available_block = nullptr;

        size_t previous_block_size = get_aviable_block_size(previous_block) + get_small_metadata();

        if (reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(previous_block) + previous_block_size) == target_block)
        {
            *reinterpret_cast<size_t*>(target_block) = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(previous_block) + sizeof(void*)) + previous_block_size + target_block_size;

            void** next_to_target_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(previous_block));
            *next_to_target_block = target_block;
        }
    }

    get_blocks_info();

    if (log != nullptr)
    {
        log->trace(get_typename() + "method deallocate has finished");
        std::cout << get_typename() + "method deallocate has finished" << std::endl;
    }
}

inline void allocator_sorted_list::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    logger* log = get_logger();
    std::mutex* mutex_buddy_system = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_buddy_system);

    if (log != nullptr)
    {
        log->trace(get_typename() + "set fit mode method has started")
        ->debug(get_typename() + "mutex has locked");
    }

    *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t)) = mode;

    if (log != nullptr)
    {
        log->trace(get_typename() + "set fit mode method has finished")
                ->debug(get_typename() + "mutex has unlocked");
    }
}

bool allocator_sorted_list::is_block_occupied(void* block) const noexcept
{
    return (block == _trusted_memory);
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info() const noexcept
{
    logger* log = get_logger();

    trace_with_guard(get_typename() + "method get blocks info has started");

    std::vector<allocator_test_utils::block_info> blocks_info;

    void** l_current_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
    void* current_block = reinterpret_cast<void**>(l_current_block + 1);

    int block_number = 0;
    size_t current_size = 0;
    size_t size_trusted_memory = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));

    while(current_size < size_trusted_memory)
    {
        allocator_test_utils::block_info value;

        ++block_number;

        value.block_size = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(current_block) + 1);

        void* memory = *reinterpret_cast<void**>(current_block);

        value.is_block_occupied = is_block_occupied(memory);

        blocks_info.push_back(std::move(value));

        current_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(current_block) + sizeof(size_t) + sizeof(void*) + value.block_size);

        current_size = current_size + sizeof(size_t) + sizeof(void*) + value.block_size;
    }

    if (block_number == 0)
    {
        warning_with_guard(get_typename() + "allocator without free blocks");
    }

    std::string string_data;

//    for (block_info value : blocks_info)
//    {
//        std::string is_oc = value.is_block_occupied ? "YES" : "NO";
//        string_data += (is_oc + "  " + std::to_string(value.block_size) + " | ");
//    }

    std::cout << "info: " << string_data << std::endl;

    trace_with_guard(get_typename() + "method get blocks info has finished");

    return blocks_info;
}

inline logger *allocator_sorted_list::get_logger() const
{
    return *reinterpret_cast<logger **>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*));
}

inline std::string allocator_sorted_list::get_typename() const noexcept
{
    return "ALLOCATOR_SORTED_LIST: ";
}
