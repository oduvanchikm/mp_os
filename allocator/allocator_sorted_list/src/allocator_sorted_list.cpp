#include <not_implemented.h>
#include "../include/allocator_sorted_list.h"

allocator_sorted_list::~allocator_sorted_list()
{
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

    if (logger != nullptr)
    {
        logger->trace(get_typename() + "allocator is deleted");
    }
}

size_t allocator_sorted_list::get_ancillary_space_size() const noexcept
{
    return sizeof(logger*) + sizeof(allocator*) + sizeof(size_t) + sizeof(void*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*);
}

std::mutex* allocator_sorted_list::get_mutex() const noexcept
{
    return reinterpret_cast<std::mutex*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
}

allocator_with_fit_mode::fit_mode allocator_sorted_list::get_fit_mode() const noexcept
{
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t));
}

void *allocator_sorted_list::get_first_aviable_block() const noexcept
{
    return *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));
}

allocator::block_size_t allocator_sorted_list::get_aviable_block_size(void *block_address) const noexcept
{
    return *reinterpret_cast<size_t *>(reinterpret_cast<void **>(block_address) + 1);
}

void *allocator_sorted_list::get_aviable_block_next_block_address(void *block_address) noexcept
{
    return *reinterpret_cast<void**>(block_address);
}

//allocator::block_size_t allocator_sorted_list::get_occupied_block_size(void *block_address) noexcept
//{
//    return *reinterpret_cast<allocator::block_size_t *>(block_address);
//}

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

    if (space_size < sizeof(block_size_t) + sizeof(block_pointer_t))
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

    allocator **parent_allocator_space_address = reinterpret_cast<allocator **>(_trusted_memory);
    *parent_allocator_space_address = parent_allocator;

    class logger **logger_space_address = reinterpret_cast<class logger **>(parent_allocator_space_address + 1);
    *logger_space_address = logger;

    size_t *space_size_space_address = reinterpret_cast<size_t *>(logger_space_address + 1);
    *space_size_space_address = space_size;

    allocator_with_fit_mode::fit_mode *fit_mode_space_address = reinterpret_cast<allocator_with_fit_mode::fit_mode *>(space_size_space_address + 1);
    *fit_mode_space_address = allocate_fit_mode;

    std::mutex** mutex_space_address = reinterpret_cast<std::mutex**>(fit_mode_space_address + 1);
    auto* mutex_buddy_system = new std::mutex;
    *mutex_space_address = mutex_buddy_system;

    void **first_block_address_space_address = reinterpret_cast<void **>(mutex_space_address + 1);
    *first_block_address_space_address = reinterpret_cast<void *>(first_block_address_space_address + 1);

    *reinterpret_cast<void **>(*first_block_address_space_address) = nullptr;

    *reinterpret_cast<size_t *>(reinterpret_cast<void **>(*first_block_address_space_address) + 1) = space_size - sizeof(void*) - sizeof(size_t);

    if (logger != nullptr)
    {
        logger->trace(get_typename() + "allocator is created")
        ->information(get_typename() + "size of memory allocated: " + std::to_string(space_size));
        std::cout << get_typename() + "allocator has created" << std::endl;
    }
}

[[nodiscard]] void *allocator_sorted_list::allocate(size_t value_size, size_t values_count)
{
    logger* log = get_logger();

    if (log != nullptr)
    {
        log->trace(get_typename() + "method allocate has started");
        std::cout << get_typename() + "method allocate has started" << std::endl;
    }

    auto requested_size = values_count * value_size;

    std::cout << "requested size: " << requested_size << std::endl;

    if (requested_size < sizeof(block_size_t) + sizeof(block_pointer_t))
    {
        requested_size = sizeof(block_size_t) + sizeof(block_pointer_t);
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
            size_t current_block_size = get_aviable_block_size(current_block);

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
            }
            previous_block = current_block;
            current_block = get_aviable_block_next_block_address(current_block);
        }
    }

    if (target_block == nullptr)
    {
        std::cout << get_typename() + "can't allocate" << std::endl;
        error_with_guard(get_typename() + "can't allocate");
        throw std::bad_alloc();
    }

    size_t size_available_block = get_aviable_block_size(target_block);

//    std::cout << "size of target block: " << size_available_block << std::endl;

    if (previous_to_target_block == nullptr)
    {
        auto blocks_sizes_difference = get_aviable_block_size(target_block) - requested_size;

        if (blocks_sizes_difference > 0 && blocks_sizes_difference <  sizeof(block_pointer_t) + sizeof(block_size_t))
        {
            requested_size = get_aviable_block_size(target_block);
            void* next_available_block = get_aviable_block_next_block_address(target_block);
            void** first_available_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex));
            *first_available_block = next_available_block;
        }
        else
        {
            size_t target_block_size = get_aviable_block_size(target_block);
            // TODO: what is this????

        }
    }
    else
    {

    }

    return this;
}

void allocator_sorted_list::deallocate(void *at)
{


}

inline void allocator_sorted_list::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    throw not_implemented("inline void allocator_sorted_list::set_fit_mode(allocator_with_fit_mode::fit_mode)", "your code should be here...");
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info() const noexcept
{
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->debug(get_typename() + "method get block info has started");
        std::cout << get_typename() + "method get block info has started" << std::endl;
    }

    std::vector<allocator_test_utils::block_info> data_vector_block_info;

    void** l_current_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
    void* current_block = *l_current_block;

    size_t current_size = 0;
    size_t size_trusted_memory = 1 << *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*));

    std::cout << size_trusted_memory << std::endl;

    while (current_size < size_trusted_memory)
    {
        allocator_test_utils::block_info blocks_info;

        blocks_info.is_block_occupied = *reinterpret_cast<unsigned char*>(current_block);

        blocks_info.block_size =  1 << (*reinterpret_cast<short*>(reinterpret_cast<unsigned char*>(current_block) + 1));

        data_vector_block_info.push_back(blocks_info);
        current_block = get_aviable_block_next_block_address(current_block);

        current_size += blocks_info.block_size;
    }

    std::string data_str;

    for (block_info value : data_vector_block_info)
    {
        std::string is_oc = value.is_block_occupied ? "YES" : "NO";
        data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
    }

    log_with_guard(get_typename() + "state blocks: " + data_str, logger::severity::debug);
    std::cout << get_typename() + "state block " << data_str << std::endl;

    if (log != nullptr)
    {
        log->debug(get_typename() + "method get block info has finished");
    }

    return data_vector_block_info;
}

inline logger *allocator_sorted_list::get_logger() const
{
    return *reinterpret_cast<logger **>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*));
}

inline std::string allocator_sorted_list::get_typename() const noexcept
{
    std::string typename_of_allocator = "ALLOCATOR_SORTED_LIST: ";

    return typename_of_allocator;
}