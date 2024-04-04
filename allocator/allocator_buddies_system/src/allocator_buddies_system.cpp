#include "../include/allocator_buddies_system.h"

size_t allocator_buddies_system::get_ancillary_space_size() const noexcept
{
    return sizeof(logger*) + sizeof(allocator*) + sizeof(size_t) + sizeof(void*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t);
}

allocator_buddies_system::allocator_buddies_system(allocator_buddies_system &&other) noexcept :
        _trusted_memory(other._trusted_memory)
{
    _trusted_memory = nullptr;
}

allocator_buddies_system &allocator_buddies_system::operator=(allocator_buddies_system &&other) noexcept
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

size_t allocator_buddies_system::get_meta_block() const
{
    return sizeof(void*) + sizeof(void*) + sizeof(short) + sizeof(unsigned char);
}

allocator_buddies_system::allocator_buddies_system(
        size_t space_size,
        allocator *parent_allocator,
        logger *logger,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if ((1 << space_size) < get_meta_block())
    {
        if (logger != nullptr)
        {
            logger->error("ALLOCATOR_BUDDIES_SYSTEM: can't initialize allocator instance, size is too small");
        }
        throw std::logic_error("ALLOCATOR_BUDDIES_SYSTEM: can't initialize allocator instance, size is too small");
    }

    if (logger != nullptr)
    {
        logger->debug("ALLOCATOR_BUDDIES_SYSTEM: the beginning of the constructor's work, allocator start creating");
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

    auto** logger_space_address = reinterpret_cast<class logger**>(parent_allocator_space_address + 1);
    *logger_space_address = logger;

    size_t* space_size_power = reinterpret_cast<size_t*>(logger_space_address + 1);
    *space_size_power = space_size;

    allocator_with_fit_mode::fit_mode* space_fit_mode_address = reinterpret_cast<allocator_with_fit_mode::fit_mode*>(space_size_power + 1);
    *space_fit_mode_address = allocate_fit_mode;

    std::mutex** mutex_space_address = reinterpret_cast<std::mutex**>(space_fit_mode_address + 1);
    auto* mutex_buddy_system = new std::mutex;
    *mutex_space_address = mutex_buddy_system;

    size_t* free_size_of_memory = reinterpret_cast<size_t*>(mutex_space_address + 1);
    *free_size_of_memory = 1 << space_size;

    void** first_free_block_address = reinterpret_cast<void**>(free_size_of_memory + 1);
//    *first_free_block_address = reinterpret_cast<void*>(first_free_block_address + 1);
    *first_free_block_address = first_free_block_address + 1;

    // TODO: metadata block

    auto* is_busy_space_block = reinterpret_cast<unsigned char*>(*first_free_block_address);
    *is_busy_space_block = 0;     // 0 - free, 1 - occupied

    auto* block_space_power = reinterpret_cast<short*>(is_busy_space_block + 1);
    *block_space_power = space_size;

    void** previous_available_block = reinterpret_cast<void**>(block_space_power + 1);
    *previous_available_block = nullptr;

    void** next_available_block = reinterpret_cast<void**>(previous_available_block + 1);
    *next_available_block = nullptr;

    if (logger != nullptr)
    {
        logger->trace("ALLOCATOR_BUDDIES_SYSTEM: allocator created");
    }
}

inline allocator *allocator_buddies_system::get_allocator() const
{
    return *reinterpret_cast<allocator**>(reinterpret_cast<unsigned char*>(_trusted_memory));
}

size_t* allocator_buddies_system::get_free_size() const noexcept
{
    return reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_buddies_system::fit_mode) + sizeof(std::mutex*));
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
    return *reinterpret_cast<std::mutex**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
}

void* allocator_buddies_system::get_first_available_block() const noexcept
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
}

bool allocator_buddies_system::is_free_block(void *block_memory) const
{
    return *reinterpret_cast<unsigned char*>(block_memory) == 0;
}

short allocator_buddies_system::get_power_of_block_size(void* available_block_address) const
{
    return *reinterpret_cast<short*>(reinterpret_cast<unsigned char*>(available_block_address) + sizeof (unsigned char)); //TODO
}

void* allocator_buddies_system::get_next_block(void* address_block) const noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(address_block) + (1 << (*reinterpret_cast<short*>(reinterpret_cast<unsigned char*>(address_block) + sizeof(unsigned char))))); // aaaaaa krokodil v vannoi
}

void* allocator_buddies_system::get_next_available_block(void* address_block) const
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(address_block) + sizeof(unsigned char) + sizeof(short) + sizeof(void*));
}

short allocator_buddies_system::get_power_of_two_up(size_t number)
{
    return static_cast<short>(std::ceil(log2(number)));
}

[[nodiscard]] void *allocator_buddies_system::allocate(size_t value_size, size_t values_count)
{
    logger* log = get_logger();
    std::mutex* mutex_buddy_system = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_buddy_system);

    if (log != nullptr)
    {
        log->trace("ALLOCATOR_BUDDIES_SYSTEM: method of allocate has started")
                ->debug("ALLOCATOR_BUDDIES_SYSTEM: mutex has locked");
    }

    size_t requested_size_mult = values_count * value_size;
    size_t requested_size = requested_size_mult + get_meta_block();
    short requested_power_size = get_power_of_two_up(requested_size);

    allocator_with_fit_mode::fit_mode fit_mode = get_fit_mode();
    size_t* available_size = get_free_size();

    void* previous_to_target_block = nullptr;
    void* target_block = nullptr;
    void* next_to_target_block = nullptr;

    {
        void* current_block = get_first_available_block();
        void* previous_block = nullptr;

        while (current_block != nullptr)
        {
            short current_block_size = *reinterpret_cast<short*>(reinterpret_cast<unsigned char*>(current_block) + sizeof (unsigned char));

            if ((1 << current_block_size) >= requested_size &&
                (fit_mode == allocator_with_fit_mode::fit_mode::first_fit ||
                 fit_mode == allocator_with_fit_mode::fit_mode::the_best_fit &&
                 (target_block == nullptr || get_power_of_block_size(target_block) > current_block_size) ||
                 fit_mode == allocator_with_fit_mode::fit_mode::the_worst_fit &&
                 (target_block == nullptr || get_power_of_block_size(target_block) < current_block_size)))
            {
                previous_to_target_block = previous_block;
                target_block = current_block;
                next_to_target_block = get_next_available_block(current_block);
                if (fit_mode == allocator_with_fit_mode::fit_mode::first_fit)
                {
                    break;
                }
            }
            previous_block = current_block;
            current_block = get_next_available_block(current_block);
        }
    }

    if (target_block == nullptr)
    {
        error_with_guard("ALLOCATOR_BUDDIES_SYSTEM: memory allocation error")->
                trace_with_guard("ALLOCATOR_BUDDIES_SYSTEM: memory allocate has finished");
        throw std::bad_alloc();
    }

    short target_block_size_power = *reinterpret_cast<short*>(reinterpret_cast<unsigned char*>(target_block) + 1);//check
    if ((1 << target_block_size_power) > requested_size)
    {
        void *next_available_block_ptr = get_next_available_block(target_block);
        short* space_power_target = reinterpret_cast<short*>(reinterpret_cast<unsigned char*>(target_block) + sizeof (unsigned char));

        while (((1 << target_block_size_power) >> 1) > requested_size)
        {
            target_block_size_power--;

            void *buddie = reinterpret_cast<void *>(reinterpret_cast<char *>(target_block) + (1 << target_block_size_power));

            *reinterpret_cast<unsigned char *>(buddie) = 0;
            *reinterpret_cast<short *>(reinterpret_cast<unsigned char *>(buddie) + sizeof (unsigned char)) = target_block_size_power;
            *reinterpret_cast<void **>(reinterpret_cast<unsigned char* >(buddie) + sizeof(unsigned char) + sizeof(short)) = target_block;
            *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(buddie) + sizeof(unsigned char) + sizeof(short) + sizeof(void*)) = get_next_available_block(target_block);

            *space_power_target = target_block_size_power;
            *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(target_block) + sizeof(unsigned char) + sizeof(short) + sizeof(void *)) = buddie;
        }

    }
    if (previous_to_target_block == nullptr)
    {
        void **first_available_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
        *first_available_block = get_next_available_block(target_block);
    }
    else
    {
        void** ptr_to_next_in_prev = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(previous_to_target_block) + sizeof (unsigned char) + sizeof (short) + sizeof(void*));
        *ptr_to_next_in_prev = target_block;
    }


    *reinterpret_cast<unsigned char*>(target_block) = 1;
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char) + sizeof(short)) = _trusted_memory;

    (*available_size) -= (1 << target_block_size_power);

    trace_with_guard("ALLOCATOR_BUDDIES_SYSTEM: memory allocate has finished");
    information_with_guard("ALLOCATOR_BUDDIES_SYSTEM: information about free size: " + std::to_string(*available_size));

    get_blocks_info();

    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char) + sizeof(short) + sizeof(void*) + sizeof(void*));
}

void* allocator_buddies_system::get_start_allocated_memory_address() noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_space_size());
}

void* allocator_buddies_system::get_buddy(void* target_block_first_buddy) noexcept
{
    trace_with_guard("ALLOCATOR_BUDDIES_SYSTEM: get buddy has started");

    size_t target_block_size_first_buddy = 1 << get_power_of_block_size(target_block_first_buddy);

    auto* allocated_memory_address = get_start_allocated_memory_address();

    auto address = reinterpret_cast<unsigned char*>(target_block_first_buddy) - reinterpret_cast<unsigned char*>(allocated_memory_address);
    auto buddies = address ^ target_block_size_first_buddy;

    auto result = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(allocated_memory_address) + buddies);

    trace_with_guard("ALLOCATOR_BUDDIES_SYSTEM: get buddy has finished");

    return result;
}

void allocator_buddies_system::deallocate(void *at)
{
    std::mutex* mutex_buddy_system = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_buddy_system);
    logger* log = get_logger();

    if (log != nullptr)
    {
        log->trace("ALLOCATOR_BUDDIES_SYSTEM: memory deallocate has started")
        ->debug("ALLOCATOR_BUDDIES_SYSTEM: mutex has locked");
    }

    size_t* available_size = get_free_size();

    void* target_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(at) - sizeof(unsigned char) - sizeof(short) - sizeof(void*) - sizeof(void*));

    short target_block_size = get_power_of_block_size(target_block);
    void* next_target_block = get_first_available_block();
    void* previous_target_block = nullptr;

    while (next_target_block != nullptr && next_target_block <= target_block)
    {
        previous_target_block = next_target_block;
        next_target_block = get_next_available_block(next_target_block);
    }

    *reinterpret_cast<unsigned char*>(target_block) = 0;
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(short) + 1) = previous_target_block;
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(short) + sizeof(void*) + 1) = next_target_block;

    if (previous_target_block == nullptr)
    {
        void** first_free_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
        *first_free_block = target_block;
    }
    else
    {
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(previous_target_block) + sizeof (unsigned char) + sizeof(short) + sizeof(void*)) = target_block;
    }

    if (next_target_block != nullptr)
    {
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(next_target_block) + sizeof(short) + sizeof (unsigned char)) = target_block;
    }

    auto buddies_address = get_buddy(target_block);

    while (reinterpret_cast<unsigned char*>(buddies_address) < reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_space_size() +
        (*reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) +
        sizeof(logger*))) && buddies_address != nullptr && is_free_block(buddies_address) &&
        get_power_of_block_size(buddies_address) == target_block_size)
    {
        void* next_block_buddie = get_next_available_block(buddies_address);

        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(short) + sizeof(void*) + sizeof(unsigned char)) = next_block_buddie;

        (*reinterpret_cast<short *>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char)))++;

        if (next_block_buddie != nullptr)
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(next_block_buddie) + sizeof(short) + sizeof(unsigned char)) = target_block;
        }

        buddies_address = get_buddy(target_block);
    }

    (*available_size) += (1 << target_block_size);

    get_blocks_info();

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
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->debug("ALLOCATOR_BUDDIES_SYSTEM: method get block info has started");
    }

    std::vector<allocator_test_utils::block_info> data_vector_block_info;

    void** l_current_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
    void* current_block = reinterpret_cast<void*>(l_current_block + 1);

    size_t current_size = 0;
    size_t size_trusted_memory = 1 << (*reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*)));

    while (current_size < size_trusted_memory)
    {
        allocator_test_utils::block_info blocks_info;

        blocks_info.is_block_occupied = *reinterpret_cast<unsigned char*>(current_block);
        blocks_info.block_size =  1 << (*reinterpret_cast<short*>(reinterpret_cast<unsigned char*>(current_block) + sizeof(unsigned char)));

        std::cout << blocks_info.block_size << std::endl;
        std::cout << blocks_info.is_block_occupied << std::endl;

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

    if (log != nullptr)
    {
        log->debug("ALLOCATOR_BUDDIES_SYSTEM: method get block info has finished");
    }

    return data_vector_block_info;
}

inline std::string allocator_buddies_system::get_typename() const noexcept
{
    return "allocator_buddies_system ";
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

    delete mutex_buddy_system;

    if (logger != nullptr)
    {
        logger->trace("ALLOCATOR_BUDDIES_SYSTEM: destructor has finished working, allocator is deleted");
    }
}

