#include <not_implemented.h>
#include "../include/allocator_global_heap.h"

allocator_global_heap::allocator_global_heap(logger *logger)
{
    if (logger != nullptr)
    {
        logger->log("ALLOCATOR_GLOBAL_HEAP: the beginning of the constructor's work, allocator start creating",
                    logger::severity::debug);
    }

    _logger = logger;

    debug_with_guard("ALLOCATOR_GLOBAL_HEAP: constructor has finished");
}

allocator_global_heap::~allocator_global_heap()
{
    debug_with_guard("ALLOCATOR_GLOBAL_HEAP: the beginning of the destructor's work, allocator start deleting");
}

[[nodiscard]] void *allocator_global_heap::allocate(size_t value_size, size_t values_count)
{
    log_with_guard("ALLOCATOR_GLOBAL_HEAP: allocate method is started",
                   logger::severity::debug);

    size_t requested_size = value_size * values_count;

    if (requested_size < sizeof(size_t) + sizeof(allocator*))
    {
        requested_size = sizeof(size_t) + sizeof(allocator*);
        warning_with_guard("ALLOCATOR_GLOBAL_HEAP: size too small, requested size has changed");
    }

    size_t common_size = requested_size + sizeof(size_t) + sizeof(allocator*);

    void* target_block;

    try
    {
        target_block = ::operator new(common_size);
    }
    catch (std::bad_alloc const &ex)
    {
        log_with_guard("ALLOCATOR_GLOBAL_HEAP: can't allocate memory",
                       logger::severity::error);
        throw std::bad_alloc();
    }

    auto** al = reinterpret_cast<allocator**>(target_block);
    auto* block_size = reinterpret_cast<size_t*>(al) + 1;
    *block_size = requested_size;

    log_with_guard("ALLOCATOR_GLOBAL_HEAP: allocate method has finished",
                   logger::severity::debug);
    return reinterpret_cast<unsigned char *>(target_block) + sizeof(size_t) + sizeof(allocator*);
}

std::string allocator_global_heap::get_block_of_memory_state(void *at) const
{
    log_with_guard("ALLOCATOR_GLOBAL_HEAP: start to get block memory state",
                   logger::severity::debug);

    std::string state_string;
    auto* bytes = reinterpret_cast<unsigned char *>(at);

    void* block = reinterpret_cast<void *>(reinterpret_cast<unsigned char*>(at) - sizeof(size_t));
    size_t* block_size = reinterpret_cast<size_t*>(block);

    for(int i = 0; i < *block_size; i++)
    {
        state_string += std::to_string(static_cast<int>(bytes[i])) + " ";

    }

    log_with_guard("ALLOCATOR_GLOBAL_HEAP: finish to get block memory state",
                   logger::severity::debug);

    return state_string;
}

void allocator_global_heap::deallocate(
    void *at)
{
    trace_with_guard("ALLOCATOR_GLOBAL_HEAP: deallocate method is started");

    size_t* block_size = reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(at) - 1);

    std::string string_state_memory_blocks = get_block_of_memory_state(at);

    log_with_guard("ALLOCATOR_GLOBAL_HEAP: string state memory block: " + string_state_memory_blocks,
                   logger::severity::debug);

    allocator* al;

    try
    {
        al = *(reinterpret_cast<allocator**>(block_size) - 1);
        if (al != this)
        {
            log_with_guard("ALLOCATOR_GLOBAL_HEAP: doesn't belong this allocate!",
                           logger::severity::error);
            throw std::logic_error("ALLOCATOR_GLOBAL_HEAP: doesn't belong this allocate!");
        }
    }
    catch (const std::logic_error &ex)
    {
        log_with_guard("ALLOCATOR_GLOBAL_HEAP: can't deallocate memory",
                       logger::severity::error);
        throw std::logic_error("ALLOCATOR_GLOBAL_HEAP: can't deallocate memory");
    }
    ::operator delete(al);

    log_with_guard("ALLOCATOR_GLOBAL_HEAP: deallocate method has finished",
                   logger::severity::debug);
}

inline logger *allocator_global_heap::get_logger() const
{
    return _logger;
}

inline std::string allocator_global_heap::get_typename() const noexcept
{
    throw not_implemented("inline std::string allocator_global_heap::get_typename() const noexcept", "your code should be here...");
}