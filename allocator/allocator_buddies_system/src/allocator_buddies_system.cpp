#include "../include/allocator_buddies_system.h"

std::string allocator_buddies_system::get_block_of_memory_state(void *at) const
{
    debug_with_guard("ALLOCATOR_BUDDIES_SYSTEM: start to get block memory state");
    std::cout << "ALLOCATOR_BUDDIES_SYSTEM: start to get block memory state" << std::endl;

    std::string state_string;
    auto* bytes = reinterpret_cast<unsigned char *>(at);

    void* block = reinterpret_cast<void *>(reinterpret_cast<unsigned char*>(at) - sizeof(bool*) - sizeof(short) - sizeof(void*) - sizeof(void*));
    size_t* block_size = reinterpret_cast<size_t*>(block);

    for(int i = 0; i < *block_size; i++)
    {
        state_string += std::to_string(static_cast<int>(bytes[i])) + " ";
    }

    debug_with_guard("ALLOCATOR_BUDDIES_SYSTEM: finish to get block memory state");
    std::cout << "ALLOCATOR_BUDDIES_SYSTEM: finish to get block memory state" << std::endl;

    return state_string;
}

size_t allocator_buddies_system::get_ancillary_space_size() const noexcept
{
    return sizeof(logger*) + sizeof(allocator*) + sizeof(size_t) + sizeof(void*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*);
}

allocator_buddies_system::allocator_buddies_system(
    size_t space_size,
    allocator *parent_allocator,
    logger *logger,
    allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (logger != nullptr)
    {
        logger->debug("ALLOCATOR_BUDDIES_SYSTEM: the beginning of the constructor's work, allocator start creating");
    }

    if ((1 << space_size) < (sizeof(bool) + sizeof(short) + sizeof(void*) + sizeof(void*)))
    {
        if (logger != nullptr)
        {
            logger->error("ALLOCATOR_BUDDIES_SYSTEM: can't initialize allocator instance, size is too small");
        }
        throw std::logic_error("ALLOCATOR_BUDDIES_SYSTEM: can't initialize allocator instance, size is too small");
    }

    size_t common_size = (1 << space_size) + get_ancillary_space_size();

    try
    {
        _trusted_memory = parent_allocator == nullptr
                          ? ::operator new(common_size)
                          : parent_allocator->allocate(common_size, 1);
    }
    catch (std::bad_alloc const &ex)
    {
        if (logger != nullptr)
        {
            logger->error("ALLOCATOR_BUDDIES_SYSTEM: error with allocate memory");
        }
        throw std::bad_alloc();
    }

    // TODO: metadata

    allocator** parent_allocator_space_address = reinterpret_cast<allocator**>(_trusted_memory);
    *parent_allocator_space_address = parent_allocator;

    class logger** logger_space_address = reinterpret_cast<class logger**>(parent_allocator_space_address + 1);
    *logger_space_address = logger;

    size_t* space_size_space_address_power = reinterpret_cast<size_t*>(logger_space_address + 1);
    *space_size_space_address_power = space_size;

    allocator_with_fit_mode::fit_mode* space_fit_mode_address = reinterpret_cast<allocator_with_fit_mode::fit_mode*>(space_size_space_address_power + 1);
    *space_fit_mode_address = allocate_fit_mode;

    void** first_free_block_address = reinterpret_cast<void**>(space_fit_mode_address + 1);

    std::mutex** mutex_space_address = reinterpret_cast<std::mutex**>(space_fit_mode_address + 1);
    auto* mutex_buddy_system = new std::mutex;
    *mutex_space_address = mutex_buddy_system;

    *first_free_block_address = mutex_space_address + 1;

    // TODO: metadata block

    bool* is_busy_space_block = reinterpret_cast<bool*>(*first_free_block_address);
    *is_busy_space_block = 0;     // 0 - free, 1 - occupied

    short* block_space_power = reinterpret_cast<short*>(is_busy_space_block + 1);
    *block_space_power = space_size;

    void** previous_available_block = reinterpret_cast<void**>(is_busy_space_block + 1);
    *previous_available_block = nullptr;

    void** next_available_block = reinterpret_cast<void**>(previous_available_block + 1);
    *next_available_block = nullptr;

    if (logger != nullptr)
    {
        logger->trace("ALLOCATOR_BUDDIES_SYSTEM: allocator created");
        std::cout << "ALLOCATOR_BUDDIES_SYSTEM: allocator created" << std::endl;
    }
}

inline allocator *allocator_buddies_system::get_allocator() const
{
    return *reinterpret_cast<allocator**>(reinterpret_cast<unsigned char*>(_trusted_memory));
}

inline logger *allocator_buddies_system::get_logger() const
{
    return *reinterpret_cast<logger**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*));
}

allocator_with_fit_mode::fit_mode allocator_buddies_system::get_fit_mode() const noexcept
{
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t));
}

std::mutex* allocator_buddies_system::get_mutex() noexcept
{
    return reinterpret_cast<std::mutex*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(void*));
}

void* allocator_buddies_system::get_first_available_block() const noexcept
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
}

bool allocator_buddies_system::check_free_block(void* target_block) const
{
    return *reinterpret_cast<bool*>(target_block) == 0;
}

size_t allocator_buddies_system::get_power_of_block_size(void* available_block_address) const
{
    return *reinterpret_cast<size_t*>(available_block_address);
}

void* allocator_buddies_system::get_next_block(void* address_block) const noexcept
{
    size_t power_memory = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*));

    if (reinterpret_cast<void *>(reinterpret_cast<unsigned char *>(address_block) + (1 << *reinterpret_cast<size_t*>(address_block))) >=
        reinterpret_cast<void *>(reinterpret_cast<unsigned char *>(_trusted_memory) + (1 << power_memory)) )
    {
        return nullptr;
    }
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char *>(address_block) + (1 << *reinterpret_cast<size_t*>(address_block)));
}

size_t allocator_buddies_system::get_available_block_size(void* block_address) noexcept
{
    return *reinterpret_cast<size_t*>(block_address);
}

void* allocator_buddies_system::get_next_available_block(void* address_block)
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(address_block) + sizeof(bool) + sizeof(short) + sizeof(void*));
}

short allocator_buddies_system::get_power_of_two(size_t number)
{
    return static_cast<short>(ceil(log2(number)));
}

[[nodiscard]] void *allocator_buddies_system::allocate(size_t value_size, size_t values_count)
{
    logger* log = get_logger();
    std::mutex* mutex_buddy_system = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_buddy_system);

    if (log != nullptr)
    {
        log->trace("ALLOCATOR_BUDDIES_SYSTEM: memory allocate has started")
        ->debug("ALLOCATOR_BUDDIES_SYSTEM: mutex has locked");
        std::cout << "ALLOCATOR_BUDDIES_SYSTEM: memory allocate has started" << std::endl;
    }

    size_t requested_size_mult = values_count * value_size;
    size_t requested_size = requested_size_mult + get_ancillary_space_size();
    size_t requested_power_size = get_power_of_two(requested_size);

    allocator_with_fit_mode::fit_mode fit_mode = get_fit_mode();

    void *previous_to_target_block = nullptr;
    void *target_block = nullptr;
    void *next_to_target_block = nullptr;
    {
        void *current_block = get_first_available_block();
        void *previous_block = nullptr;

        while (current_block != nullptr)
        {
            size_t current_block_size = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char *>(current_block) + 1);

            std::cout << "current_block_size: " << current_block_size << std::endl;
            std::cout << "requested_power_size: " << requested_power_size << std::endl;

            if (current_block_size >= requested_power_size &&
                (fit_mode == allocator_with_fit_mode::fit_mode::first_fit ||
                fit_mode == allocator_with_fit_mode::fit_mode::the_best_fit &&
                (target_block == nullptr || get_available_block_size(target_block) > current_block_size) ||
                fit_mode == allocator_with_fit_mode::fit_mode::the_worst_fit && (target_block == nullptr ||
                get_available_block_size(target_block) < current_block_size)))
            {
                std::cout << "i hate programming" << std::endl;
                previous_to_target_block = previous_block;
                target_block = current_block;
                next_to_target_block = get_next_available_block(current_block);
            }
            std::cout << "lalala" << std::endl;
            previous_block = current_block;
            current_block = get_next_available_block(current_block);
        }
    }
    std::cout << "lalakgopkdgapkdla" << std::endl;

    if (target_block == nullptr)
    {
//        std::cout << "target_block " << target_block << std::endl;
        error_with_guard("ALLOCATOR_BUDDIES_SYSTEM: memory allocation error")->
        trace_with_guard("ALLOCATOR_BUDDIES_SYSTEM: memory allocate has finished");
        throw std::bad_alloc();
    }

    std::cout << "target_block_2 " << target_block << std::endl;

    auto target_block_size_power = get_power_of_block_size(target_block);

    while (((1 << target_block_size_power) >> 1) >= (1 << requested_power_size))
    {
        // TODO: THE LOGIC NEEDS TO BE REVIEWED AGAIN

        target_block_size_power--;

        bool* is_busy_space_block = reinterpret_cast<bool *>(target_block);
        short* block_space_size_power = reinterpret_cast<short *>(is_busy_space_block + 1);
        void** previous_available_block = reinterpret_cast<void**>(block_space_size_power + 1);
        void** next_available_block = reinterpret_cast<void**>(previous_available_block + 1);

        auto* left_buddie = reinterpret_cast<unsigned char*>(target_block);
        auto* right_buddie = left_buddie + (1 << target_block_size_power);

        bool* is_busy_space_block_right_buddie = reinterpret_cast<bool *>(right_buddie);
        short* block_space_size_power_right_buddie = reinterpret_cast<short *>(is_busy_space_block_right_buddie + 1);
        void** previous_available_block_right_buddie = reinterpret_cast<void**>(block_space_size_power_right_buddie + 1);
        void** next_available_block_right_buddie = reinterpret_cast<void**>(previous_available_block_right_buddie + 1);

        // 0 - free, 1 - occupied

        *is_busy_space_block_right_buddie = 1;
        *block_space_size_power_right_buddie = target_block_size_power;
        *previous_available_block_right_buddie = target_block;
        *next_available_block_right_buddie = get_next_available_block(target_block);

        // TODO: THE LOGIC NEEDS TO BE REVIEWED AGAIN

        *next_available_block = right_buddie;
        *block_space_size_power = target_block_size_power;
    }

    if (previous_to_target_block == nullptr)
    {
        void **ptr_first_available = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(short) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex));
        *ptr_first_available = get_next_block(target_block);
    }

    *reinterpret_cast<bool*>(reinterpret_cast<short*>(target_block) + 1) = true;

    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(bool) + sizeof(short)) = _trusted_memory;


    auto final_allocated_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(bool) + sizeof(short) + sizeof(void*) + sizeof(void*));

    std::string string_result_status_blocks = get_block_of_memory_state(target_block);
    std::cout << "ALLOCATOR_BUDDIES_SYSTEM: state of blocks: " << string_result_status_blocks << std::endl;
    trace_with_guard("ALLOCATOR_BUDDIES_SYSTEM: memory allocate has finished");

    return final_allocated_block;
}

void* allocator_buddies_system::get_start_allocated_memory_address() noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_space_size());
}

void* allocator_buddies_system::get_buddy(void* target_block_first_buddy) noexcept
{
    size_t target_block_size_first_buddy = 1 << get_power_of_block_size(target_block_first_buddy);

    auto* allocated_memory_address = get_start_allocated_memory_address();

    auto address = reinterpret_cast<unsigned char*>(target_block_first_buddy) - reinterpret_cast<unsigned char*>(allocated_memory_address);
    auto buddies = address ^ target_block_size_first_buddy;

    auto result = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(allocated_memory_address) + buddies);

    return result;
}

void allocator_buddies_system::deallocate(void *at)
{
    std::mutex* mutex_buddy_system = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_buddy_system);
    logger* log = get_logger();

    if (log != nullptr)
    {
        log->trace("ALLOCATOR_BUDDIES_SYSTEM: memory deallocate has started");
    }

    if (log != nullptr)
    {
        log->debug("ALLOCATOR_BUDDIES_SYSTEM: mutex has locked");
    }

    if (at == nullptr)
    {
        if (log != nullptr)
        {
            log->error("ALLOCATOR_BUDDIES_SYSTEM: error with deallocate memory")
            ->trace("ALLOCATOR_BUDDIES_SYSTEM: memory deallocate has finished");
        }
        throw std::bad_alloc();
    }

    void* target_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(at) - sizeof(bool*) - sizeof(short) - sizeof(void*) - sizeof(void*));

    short target_block_size = get_power_of_block_size(target_block);

    std::string string_block_status = get_block_of_memory_state(at);

    std::cout << "ALLOCATOR_BUDDIES_SYSTEM: state blocks memory" << string_block_status << std::endl;

    void* next_target_block = get_first_available_block();
    void* previous_target_block = nullptr;

    bool* is_busy_space_block = reinterpret_cast<bool *>(target_block);
    short* block_space_size_power = reinterpret_cast<short *>(is_busy_space_block + 1);
    void** previous_available_block = reinterpret_cast<void**>(block_space_size_power + 1);
    void** next_available_block = reinterpret_cast<void**>(previous_available_block + 1);

    *is_busy_space_block = 0;
    *previous_available_block = previous_target_block;
    *next_available_block = next_target_block;

    if (previous_target_block == nullptr)
    {
        void** first_free_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));
        *first_free_block = target_block;
    }
//    else
//    {
//        bool* is_busy_space_block_previous = reinterpret_cast<bool *>(previous_target_block);
//        short* block_space_size_power_previous = reinterpret_cast<short *>(is_busy_space_block_previous + 1);
//        void** previous_available_block_previous = reinterpret_cast<void**>(block_space_size_power_previous + 1);
//        void** next_available_block_previous = reinterpret_cast<void**>(previous_available_block_previous + 1);
//
//        *next_available_block_previous = target_block;
//    }

    if (next_target_block != nullptr)
    {
        bool* is_busy_space_block_next = reinterpret_cast<bool *>(next_target_block);
        short* block_space_size_power_next = reinterpret_cast<short *>(is_busy_space_block_next + 1);
        void** previous_available_block_next = reinterpret_cast<void**>(block_space_size_power_next + 1);

        *previous_available_block_next = target_block;
    }

    auto buddies_address = get_buddy(target_block);

    while (buddies_address != nullptr && check_free_block(buddies_address) &&
        get_power_of_block_size(buddies_address) == target_block_size)
    {
        if (buddies_address < target_block)
        {
            auto temp = buddies_address;
            buddies_address = target_block;
            target_block = temp;
        }

        bool* is_busy_space_block_target = reinterpret_cast<bool *>(target_block);
        short* block_space_size_power_target = reinterpret_cast<short *>(is_busy_space_block_target + 1);
        void** previous_available_block_target = reinterpret_cast<void**>(block_space_size_power_target + 1);
        void** next_available_block_target = reinterpret_cast<void**>(previous_available_block_target + 1);

        void* next_block_buddie = get_next_available_block(buddies_address);

        *next_available_block_target = next_block_buddie;

        (*block_space_size_power_target)++;

        if (next_block_buddie != nullptr)
        {
            bool* is_busy_space_block_buddie = reinterpret_cast<bool *>(next_block_buddie);
            short* block_space_size_power_buddie = reinterpret_cast<short *>(is_busy_space_block_buddie + 1);
            void** previous_available_block_buddie = reinterpret_cast<void**>(block_space_size_power_buddie + 1);

            *previous_available_block_buddie = target_block;
        }

        buddies_address = get_buddy(target_block);
    }


    if (log != nullptr)
    {
        log->trace("ALLOCATOR_BUDDIES_SYSTEM: memory deallocate has finished");
    }
}

inline void allocator_buddies_system::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    logger* log = get_logger();

    if (log != nullptr)
    {
        log->debug("ALLOCATOR_BUDDIES_SYSTEM: change fit mode");
    }

    *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t)) = mode;

    if (log != nullptr)
    {
        log->debug("ALLOCATOR_BUDDIES_SYSTEM: fit mode was changed");
    }
}

std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info() const noexcept
{
    // TODO: change this code
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->debug("ALLOCATOR_BUDDIES_SYSTEM: method get block info has started");
    }

    std::vector<allocator_test_utils::block_info> data_vector_block_info;

    void* current_block = get_first_available_block();

    size_t current_size = 0;
    size_t size_trusted_memory = 1 << get_power_of_block_size(_trusted_memory);

    while (current_size < size_trusted_memory)
    {
        allocator_test_utils::block_info blocks_info;

        blocks_info.is_block_occupied = reinterpret_cast<bool*>(current_block);

        blocks_info.block_size =  1 << get_power_of_block_size(current_block);

        data_vector_block_info.push_back(blocks_info);
        current_block = get_next_block(current_block);

        current_size += blocks_info.block_size;
    }

    std::string data_str;

    for (block_info value : data_vector_block_info)
    {
        std::string is_oc = value.is_block_occupied ? "YES" : "NO";
        data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
    }

    log_with_guard("ALLOCATOR_BUDDIES_SYSTEM: state blocks: " + data_str, logger::severity::debug);
    std::cout << "ALLOCATOR_BUDDIES_SYSTEM: state block " << data_str << std::endl;

    if (log != nullptr)
    {
        log->debug("ALLOCATOR_BUDDIES_SYSTEM: method get block info has finished");
    }

    return data_vector_block_info;
}

inline std::string allocator_buddies_system::get_typename() const noexcept
{
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->debug("ALLOCATOR_BUDDIES_SYSTEM: method get_typename start");
    }

    // TODO: have to write something

    if (log != nullptr)
    {
        log->debug("ALLOCATOR_BUDDIES_SYSTEM: method get_typename finish");
    }
}

allocator_buddies_system::~allocator_buddies_system()
{
    std::mutex* mutex_buddy_system = get_mutex();

    std::lock_guard<std::mutex> lock(*mutex_buddy_system);
    auto* allocator = get_allocator();
    auto* logger = get_logger();

    if (logger != nullptr)
    {
        logger->trace("ALLOCATOR_BUDDIES_SYSTEM: destructor start working");
    }

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
        logger->trace("ALLOCATOR_BUDDIES_SYSTEM: destructor has finished working, allocator is deleted\n");
    }
}
