#include "../include/allocator_buddies_system.h"

size_t allocator_buddies_system::get_ancillary_space_size() const noexcept
{
    return sizeof(logger*) + sizeof(allocator*) + sizeof(size_t) + sizeof(void*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t);
}

size_t allocator_buddies_system::get_aviable_block_size() const
{
    return sizeof(unsigned char) + sizeof(short) + sizeof(void*) + sizeof(void*);
}

allocator_buddies_system::allocator_buddies_system(
        size_t space_size,
        allocator *parent_allocator,
        logger *logger,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (1 << space_size < (sizeof(unsigned char) + sizeof(short) + sizeof(void*) + sizeof(void*)))
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
    *free_size_of_memory = space_size;

    void** first_free_block_address = reinterpret_cast<void**>(free_size_of_memory + 1);
//    *first_free_block_address = reinterpret_cast<void*>(first_free_block_address + 1);
    *first_free_block_address = first_free_block_address + 1;

    // TODO: metadata block

    auto* is_busy_space_block = reinterpret_cast<unsigned char*>(*first_free_block_address);
    *is_busy_space_block = 0;     // 0 - free, 1 - occupied

//    std::cout << "is busy block " << std::to_string(*reinterpret_cast<unsigned char*>(*first_free_block_address)) << std::endl;

    auto* block_space_power = reinterpret_cast<short*>(is_busy_space_block + 1);
    *block_space_power = space_size;

//    std::cout << "power " << std::to_string(*reinterpret_cast<short*>(is_busy_space_block + 1)) << std::endl;

    void** previous_available_block = reinterpret_cast<void**>(block_space_power + 1);
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
    return *reinterpret_cast<short*>(block_memory) == 0;
}

short allocator_buddies_system::get_power_of_block_size(void* available_block_address) const
{
    return *reinterpret_cast<short*>(reinterpret_cast<unsigned char*>(available_block_address) + 1); //TODO
}

void* allocator_buddies_system::get_next_block(void* address_block) const noexcept
{
    size_t power_memory = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*));

    if (reinterpret_cast<void *>(reinterpret_cast<unsigned char *>(address_block) + (1 << *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(address_block) + 1))) >=
        reinterpret_cast<void *>(reinterpret_cast<unsigned char *>(_trusted_memory) + (1 << power_memory)))
    {
        return nullptr;
    }
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(address_block) + (1 << *reinterpret_cast<size_t*>(address_block)) + 1); // TODO
}

void* allocator_buddies_system::get_next_available_block(void* address_block) const
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(address_block) + sizeof(unsigned char) + sizeof(short) + sizeof(void*));
}

short allocator_buddies_system::get_power_of_two(size_t number)
{
    return static_cast<short>(std::floor(ceil(log2(number))));
}

[[nodiscard]] void *allocator_buddies_system::allocate(size_t value_size, size_t values_count)
{
    logger* log = get_logger();
    std::mutex* mutex_buddy_system = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_buddy_system);
    std::cout << "MUTEX HAS CREATED AND LOCKED" << std::endl;

    if (log != nullptr)
    {
        log->trace("ALLOCATOR_BUDDIES_SYSTEM: method of allocate has started")
                ->debug("ALLOCATOR_BUDDIES_SYSTEM: mutex has locked");
        std::cout << "ALLOCATOR_BUDDIES_SYSTEM: method of allocate has started" << std::endl;
    }

    size_t requested_size_mult = values_count * value_size;
    size_t requested_size = requested_size_mult + sizeof(unsigned char) + sizeof(short) + sizeof(void*) + sizeof(void*);
    size_t requested_power_size = get_power_of_two(requested_size);

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
            size_t current_block_size = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(current_block) + 1);

            std::cout << "current_block_size: " << current_block_size << std::endl;
            std::cout << "requested_power_size: " << requested_power_size << std::endl;

            if (current_block_size >= requested_power_size &&
                (fit_mode == allocator_with_fit_mode::fit_mode::first_fit ||
                 fit_mode == allocator_with_fit_mode::fit_mode::the_best_fit &&
                 (target_block == nullptr || get_power_of_block_size(target_block) > current_block_size) ||
                 fit_mode == allocator_with_fit_mode::fit_mode::the_worst_fit &&
                 (target_block == nullptr || get_power_of_block_size(target_block) < current_block_size)))
            {
                previous_to_target_block = previous_block;
                target_block = current_block;
                next_to_target_block = get_next_available_block(current_block);
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

    auto target_block_size_power = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(target_block) + 1);

    if ((1 << target_block_size_power) >= requested_size)
    {
        void *next_available_block_ptr = get_next_available_block(target_block);

        while (((1 << target_block_size_power) >> 1) >= requested_size)
        {
            target_block_size_power--;

            void *buddie = reinterpret_cast<void *>(reinterpret_cast<char *>(target_block) +
                                                    (1 << target_block_size_power));

            *reinterpret_cast<unsigned char *>(buddie) = 0;
            *reinterpret_cast<short *>(reinterpret_cast<unsigned char *>(buddie) + 1) = target_block_size_power;
            *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(buddie) + sizeof(short) + 1) = target_block;
            *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(buddie) + sizeof(short) + sizeof(void *) +
                                       1) = next_available_block_ptr;

            *reinterpret_cast<short *>(reinterpret_cast<unsigned char *>(target_block) + 1) = target_block_size_power;
            *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(target_block) + sizeof(short) +
                                       sizeof(void *) +
                                       1) = buddie;
        }

        if (previous_to_target_block == nullptr)
        {
            void **first_available_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(short) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
            *first_available_block = next_available_block_ptr;
        }

        *reinterpret_cast<unsigned char*>(target_block) = 1;

//        target_block = get_next_available_block(target_block);

    }


    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char) + sizeof(short)) = _trusted_memory;

//    target_block = get_next_available_block(target_block);

    (*available_size) -= (1 << target_block_size_power);

    trace_with_guard("ALLOCATOR_BUDDIES_SYSTEM: memory allocate has finished");
    information_with_guard("ALLOCATOR_BUDDIES_SYSTEM: information about free size: " + std::to_string(*available_size));
    std::cout << "ALLOCATOR_BUDDIES_SYSTEM: information about free size: " + std::to_string(*available_size) << std::endl;
    std::cout << "ALLOCATOR_BUDDIES_SYSTEM: memory allocate has finished" << std::endl;

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
    std::cout << "ALLOCATOR_BUDDIES_SYSTEM: get buddy has started" << std::endl;

    size_t target_block_size_first_buddy = 1 << get_power_of_block_size(target_block_first_buddy);

    auto* allocated_memory_address = get_start_allocated_memory_address();

    auto address = reinterpret_cast<unsigned char*>(target_block_first_buddy) - reinterpret_cast<unsigned char*>(allocated_memory_address);
    auto buddies = address ^ target_block_size_first_buddy;

    auto result = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(allocated_memory_address) + buddies);

    trace_with_guard("ALLOCATOR_BUDDIES_SYSTEM: get buddy has finished");
    std::cout << "ALLOCATOR_BUDDIES_SYSTEM: get buddy has finished" << std::endl;

    return result;
}

void allocator_buddies_system::deallocate(void *at)
{
    std::mutex* mutex_buddy_system = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_buddy_system);
    logger* log = get_logger();

    if (log != nullptr)
    {
        std::cout << "ALLOCATOR_BUDDIES_SYSTEM: memory deallocate has started" << std::endl;
        log->trace("ALLOCATOR_BUDDIES_SYSTEM: memory deallocate has started");
        log->debug("ALLOCATOR_BUDDIES_SYSTEM: mutex has locked");
    }

    if (at == nullptr)
    {
        if (log != nullptr)
        {
            log->error("ALLOCATOR_BUDDIES_SYSTEM: error with deallocate memory")
                    ->trace("ALLOCATOR_BUDDIES_SYSTEM: memory deallocate has finished");
        }
        std::cout << "bad allocc" << std::endl;
        throw std::bad_alloc();
    }

    std::cout << "good" << std::endl;

    size_t* available_size = get_free_size();

    void* target_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(at) - sizeof(unsigned char) - sizeof(short) - sizeof(void*) - sizeof(void*));

    short target_block_size = get_power_of_block_size(target_block);

    void* next_target_block = get_first_available_block();
    void* previous_target_block = nullptr;

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
        *reinterpret_cast<void**>(reinterpret_cast<short *>(reinterpret_cast<unsigned char*>(target_block) + sizeof(short) + sizeof(void*) + sizeof(void*))) = target_block;
    }

    if (next_target_block != nullptr)
    {
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(short) + 1) = target_block;
    }

    auto buddies_address = get_buddy(target_block);

    while (reinterpret_cast<unsigned char*>(buddies_address) <
    reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_space_size() +
    (*reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) +
    sizeof(logger*) + 1)) &&
            buddies_address != nullptr && is_free_block(buddies_address) &&
            get_power_of_block_size(buddies_address) == target_block_size)
    {
        if (buddies_address < target_block)
        {
            auto temp = buddies_address;
            buddies_address = target_block;
            target_block = temp;
        }

        void* next_block_buddie = get_next_available_block(buddies_address);

        *reinterpret_cast<void**>(reinterpret_cast<short *>(reinterpret_cast<unsigned char*>(target_block) + sizeof(short) + sizeof(void*) + sizeof(void*))) = next_block_buddie;

        (*reinterpret_cast<short *>(reinterpret_cast<unsigned char*>(target_block) + sizeof(short)))++;

        if (next_block_buddie != nullptr)
        {
            *reinterpret_cast<void**>(reinterpret_cast<short *>(reinterpret_cast<unsigned char*>(next_block_buddie) + sizeof(short) + sizeof(void*))) = target_block;
        }

        buddies_address = get_buddy(target_block);
    }

    (*available_size) += (1 << target_block_size);


    std::cout << "aval size: " << std::to_string(*available_size) << std::endl;

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
        std::cout << "ALLOCATOR_BUDDIES_SYSTEM: method get block info has started" << std::endl;
    }

    std::vector<allocator_test_utils::block_info> data_vector_block_info;

    void** l_current_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
//    void* current_block = *l_current_block;
    void* current_block = reinterpret_cast<void*>(l_current_block + 1);

    size_t current_size = 0;
    size_t size_trusted_memory = 1 << (*reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*)));

    std::cout << size_trusted_memory << std::endl;

    while (current_size < size_trusted_memory)
    {
        allocator_test_utils::block_info blocks_info;

        blocks_info.is_block_occupied = *reinterpret_cast<unsigned char*>(current_block);

        blocks_info.block_size =  1 << (*reinterpret_cast<short*>(reinterpret_cast<unsigned char*>(current_block) + 1));

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
        std::cout << "I DO THIS!!!!!!!!!!!!!!!" << std::endl;
        logger->trace("ALLOCATOR_BUDDIES_SYSTEM: destructor has finished working, allocator is deleted\n");
    }
}