#include <not_implemented.h>
#include <allocator_global_heap.h>
#include <string>

allocator_global_heap::allocator_global_heap(logger *logger)
{
    if (logger != nullptr)
    {
        logger->debug(get_typename() + "constructor has called");
    }

    _logger = logger;
}

allocator_global_heap::~allocator_global_heap()
{
    debug_with_guard(get_typename() + "destructor has called");
}

[[nodiscard]] void *allocator_global_heap::allocate(size_t value_size, size_t values_count)
{
    debug_with_guard(get_typename() + "allocate method is started");

    size_t requested_size = value_size * values_count;

    /*if (requested_size < sizeof(size_t) + sizeof(allocator*))
    {
        requested_size = sizeof(size_t) + sizeof(allocator*);
        warning_with_guard(get_typename() + "size too small, requested size has changed");
    }*/

    size_t common_size = requested_size + sizeof(size_t) + sizeof(allocator*);

    void* target_block;

    try
    {
        target_block = ::operator new(common_size);
    }
    catch (std::bad_alloc const &ex)
    {
        error_with_guard(get_typename() + "can't allocate memory");
        throw std::bad_alloc();
    }

    auto** allocator_ptr = reinterpret_cast<allocator**>(target_block);
    *allocator_ptr = this;
    auto* block_size = reinterpret_cast<size_t*>(allocator_ptr) + 1;
    *block_size = requested_size;

    debug_with_guard(get_typename() + "allocate method has finished");

    return reinterpret_cast<unsigned char*>(target_block) + sizeof(size_t) + sizeof(allocator*);
}

std::string allocator_global_heap::get_block_of_memory_state(void *at) const
{
    debug_with_guard(get_typename() + "start to get block memory state");

    std::string state_string;
    auto* bytes = reinterpret_cast<unsigned char*>(at);

    void* block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(at) - sizeof(size_t));

    size_t* block_size = reinterpret_cast<size_t*>(block);

    for(int i = 0; i < *block_size; i++)
    {
        state_string += std::to_string(static_cast<int>(bytes[i])) + " ";
    }

    debug_with_guard(get_typename() + "finish to get block memory state");

    return state_string;
}

void allocator_global_heap::deallocate(void *at)
{
    trace_with_guard(get_typename() + "deallocate method is started");

    void* block = reinterpret_cast<void *>(reinterpret_cast<unsigned char*>(at) - sizeof(size_t));
    size_t* block_size = reinterpret_cast<size_t*>(block);

    std::string string_state_memory_blocks = get_block_of_memory_state(at);

    debug_with_guard(get_typename() + "string state memory block: " + string_state_memory_blocks);

    allocator* allocator_ptr;

    allocator_ptr = *reinterpret_cast<allocator**>(block_size - 1);
    if (allocator_ptr != this)
    {
        error_with_guard("ALLOCATOR_GLOBAL_HEAP: can't deallocate memory");
        throw std::logic_error("ALLOCATOR_GLOBAL_HEAP: can't deallocate memory");
    }

    ::operator delete(allocator_ptr);

    debug_with_guard(get_typename() + "deallocate method has finished");
}

inline logger *allocator_global_heap::get_logger() const
{
    return _logger;
}

inline std::string allocator_global_heap::get_typename() const noexcept
{
    std::string message = "ALLOCATOR_GLOBAL_HEAP: ";

    return message;
}