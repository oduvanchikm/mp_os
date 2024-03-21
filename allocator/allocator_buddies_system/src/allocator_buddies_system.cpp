#include "../include/allocator_buddies_system.h"

// метаданные =
// размер аллокатора + размер логгера + размер всей памяти + размер указателя на первый блок + указатель на объект синхронизации
size_t allocator_buddies_system::get_ancillary_space_size() const noexcept
{
    return sizeof(logger*) + sizeof(allocator*) + sizeof(size_t) + sizeof(void*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*);
}

size_t allocator_buddies_system::closest_power_of_two(size_t number)
{
    size_t power = 1;

    while (power < number)
    {
        power <<= 1;
    }

    return power;
}

allocator_buddies_system::allocator_buddies_system(
    size_t space_size, // сколько хотим памяти (без метаданных)
    allocator *parent_allocator,
    logger *logger,
    allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (logger != nullptr)
    {
        logger->debug("ALLOCATOR_BUDDIES_SYSTEM: the beginning of the constructor's work, allocator start creating");
    }

    if (space_size < sizeof(block_size_t) + sizeof(block_pointer_t)) // если размер запрошенной памяти меньше, чем размер для хранения одного блока памяти
    {
        if (logger != nullptr)
        {
            logger->error("ALLOCATOR_BUDDIES_SYSTEM: error with size allocate memory");
        }

        throw std::logic_error("ALLOCATOR_BUDDIES_SYSTEM: can't initialize allocator instance, size too small");
    }

    if (logger != nullptr)
    {
        logger->debug("ALLOCATOR_BUDDIES_SYSTEM: start to allocate memory");
    }

    // находим ближайшее большее число степени двойки
    auto new_space_size = closest_power_of_two(space_size);
//    // считаем общий размер памяти с метаданными
    auto common_size = new_space_size + get_ancillary_space_size();
    try
    {
        _trusted_memory = parent_allocator == nullptr
                          ? ::operator new(common_size) // запрашиваем размер
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

    // проинициализировали аллокатор
    auto** parent_allocator_space_address = reinterpret_cast<allocator**>(_trusted_memory);
    *parent_allocator_space_address = parent_allocator;

    // проинициализировали логгер
    auto** logger_space_address = reinterpret_cast<class logger**>(parent_allocator_space_address + 1);
    *logger_space_address = logger;

    // записали размер памяти, которую хотим выделить
    auto* space_size_space_address = reinterpret_cast<size_t*>(logger_space_address + 1);
    *space_size_space_address = space_size;

    // проинициализировали fit_mode
    auto* space_fit_mode_address = reinterpret_cast<allocator_with_fit_mode::fit_mode*>(space_size_space_address + 1);
    *space_fit_mode_address = allocate_fit_mode;

    // создали мьютекс для синхронизации работы аллокатора
    auto** mutex_space_address = reinterpret_cast<std::mutex**>(space_fit_mode_address + 1);

    auto* mutex_buddy_system = new std::mutex;
    if (!mutex_buddy_system)
    {
        if (logger != nullptr)
        {
            logger->error("ALLOCATOR_BUDDIES_SYSTEM: error with mutex");
        }
        exit(1);
    }
    *mutex_space_address = mutex_buddy_system;


    // указатель на первый блок памяти в выделенном пространстве
    void** first_block_address = reinterpret_cast<void**>(mutex_space_address + 1);
    *first_block_address = reinterpret_cast<void*>(first_block_address + 1);

    // установили значение первого блока в нулевой указатель, чтобы обозначить, что этот блок свободен
    *reinterpret_cast<void**>(*first_block_address) = nullptr;

    // указатель на следующий свободный блок
    void** previous_available_block = reinterpret_cast<void**>(first_block_address + 1);
    *previous_available_block = nullptr;

    // указатель на предыдущий свободный блок
    void** next_available_block = reinterpret_cast<void**>(previous_available_block + 1);
    *next_available_block = nullptr;

    if (logger != nullptr)
    {
        logger->trace("ALLOCATOR_BUDDIES_SYSTEM: allocator created");
    }
}

allocator_with_fit_mode::fit_mode allocator_buddies_system::get_fit_mode() const noexcept
{
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t));
}

void* allocator_buddies_system::get_first_available_block() const noexcept
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
}

allocator::block_size_t allocator_buddies_system::get_available_block_size(void* block_address) noexcept
{
    return *reinterpret_cast<allocator::block_size_t*>(reinterpret_cast<void**>(block_address) + 1);
}

void* allocator_buddies_system::get_next_available_block(void* block_address) noexcept
{
    return *reinterpret_cast<void**>(block_address);
}

void* allocator_buddies_system::get_previous_available_block(void* block) noexcept
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(unsigned char));
}

[[nodiscard]] void *allocator_buddies_system::allocate(size_t value_size, size_t values_count)
{
    logger* log = get_logger();
    std::mutex* mutex_buddy_system = get_mutex();

    std::lock_guard<std::mutex> lock(*mutex_buddy_system);

    if (log != nullptr)
    {
        log->trace("ALLOCATOR_BUDDIES_SYSTEM: memory allocate has started");
    }

    auto requested_size_mult = values_count * value_size; // перемножаем, чтобы понять, сколько нужно
    if (requested_size_mult < sizeof(block_pointer_t) + sizeof(block_size_t))
    {
        requested_size_mult = sizeof(block_pointer_t) + sizeof(block_size_t);
        if (log != nullptr)
        {
            log->warning("ALLOCATOR_BUDDIES_SYSTEM: request space size was changed");
        }
    }

    auto requested_size_without_meta_data = closest_power_of_two(requested_size_mult);
    auto requested_size = requested_size_without_meta_data + get_ancillary_space_size();

    allocator_with_fit_mode::fit_mode fit_mode = get_fit_mode();

    // начинаем поиск подходящего свободного блока

    void *previous_to_target_block = nullptr; // указатель на предыдущий блок
    void *target_block = nullptr; // указатель на блок, который мы выделяем
    void *next_to_target_block = nullptr; // указатель на следующий блок


    void *current_block = get_first_available_block(); // указатель на текущий свободный блок
    void *previous_block = nullptr; // указатель на предыдущий блок
    void *next_block = nullptr; // указатель на следующий свободный блок

    while (current_block != nullptr)
    {
        size_t current_block_size = get_available_block_size(current_block); // находим размер текущего свободного блока

        if (current_block_size >= requested_size) // если вмещается
        {
            if (fit_mode == allocator_with_fit_mode::fit_mode::first_fit ||
                fit_mode == allocator_with_fit_mode::fit_mode::the_best_fit &&
                (target_block == nullptr ||
                 get_available_block_size(target_block) > current_block_size) ||
                fit_mode == allocator_with_fit_mode::fit_mode::the_worst_fit &&
                (target_block == nullptr ||
                 get_available_block_size(target_block) < current_block_size))
            {
                previous_to_target_block = previous_block;
                target_block = current_block; // сохраняем указатель на найденный блок
                next_to_target_block = next_block;
            }
        }

        next_block = get_next_available_block(current_block); // получаем указатель на следующий свободный блок
        current_block = next_block; // переходим к следующему блоку
    }

    if (target_block == nullptr) // если не нашли свободный блок, то выбрасываем еррор
    {
        error_with_guard("ALLOCATOR_BUDDIES_SYSTEM: memory allocation error")->
        trace_with_guard("ALLOCATOR_BUDDIES_SYSTEM: memory allocate has finished");
        throw std::bad_alloc();
    }

    size_t target_block_size = get_available_block_size(target_block);

    while ((target_block_size >> 1) >= (requested_size + get_ancillary_space_size()))
    {
        // разделяем блоки пополам

        // TODO: THE LOGIC NEEDS TO BE REVIEWED AGAIN

        size_t half_target_size_block = target_block_size >> 1;

        auto* left_buddy = reinterpret_cast<unsigned char*>(target_block);
        auto* right_buddy = left_buddy + (half_target_size_block);

        next_block = reinterpret_cast<void*>(right_buddy); // next_block = правый двойник
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(next_block) + 1) = reinterpret_cast<void*>(right_buddy);
        *(reinterpret_cast<void**>(right_buddy + 1) + 1) = next_block; // у правого двойника следующий блок, становится тот, который был следующим после изначального блока
        *(reinterpret_cast<void**>(left_buddy + 1) + 1) = reinterpret_cast<void*>(right_buddy); // у левого двойника следующим блоком становится правый двойник
        *reinterpret_cast<void**>(right_buddy + 1) = reinterpret_cast<void*>(left_buddy); // у правого двойника предыдущим блоком, становится его левый двойник

        // TODO: THE LOGIC NEEDS TO BE REVIEWED AGAIN
    }

    next_to_target_block = get_next_available_block(target_block);
    previous_to_target_block = get_previous_available_block(target_block);

    // TODO: process cases when previous_target_blank == null ptr and next_target_block == nullptr

    auto final_allocated_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(unsigned char));


    trace_with_guard("ALLOCATOR_BUDDIES_SYSTEM: memory allocate has finished");

    return final_allocated_block;
}

void* allocator_buddies_system::get_start_allocated_memory_address() noexcept
{
    return reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_space_size());
}

void* allocator_buddies_system::get_buddy(void* target_block_first_buddy, size_t target_block_size_first_buddy) noexcept
{
    auto* allocated_memory_address = get_start_allocated_memory_address();

    auto address = reinterpret_cast<unsigned char*>(target_block_first_buddy) - reinterpret_cast<unsigned char*>(allocated_memory_address);
    auto buddies = address ^ target_block_size_first_buddy;

    auto result = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(allocated_memory_address) + buddies);

    return result;
}

bool allocator_buddies_system::check_free_block(void* target_block) const
{
    return *reinterpret_cast<unsigned char*>(target_block) == 0;
}

void allocator_buddies_system::deallocate(void *at) // область памяти, которую нужно освободить
{
    std::mutex* mutex_buddy_system = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_buddy_system);
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->trace("ALLOCATOR_BUDDIES_SYSTEM: memory deallocate has started");
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

    auto* target_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(at) - 1);

    size_t target_block_size = get_available_block_size(target_block);
    if (target_block_size < sizeof(block_pointer_t) + sizeof(block_size_t))
    {
        target_block_size = sizeof(block_pointer_t) + sizeof(block_size_t);
        if (log != nullptr)
        {
            log->warning("ALLOCATOR_BUDDIES_SYSTEM: request space size was changed");
        }
    }

    void* previous_block = nullptr;
    void* next_block = get_first_available_block();

    auto buddies_address = get_buddy(target_block, target_block_size);

    while (buddies_address != nullptr && check_free_block(buddies_address) &&
        get_available_block_size(buddies_address) == get_available_block_size(target_block))
    {
        if (buddies_address < target_block)
        {
            auto temp = buddies_address;
            buddies_address = target_block;
            target_block = temp;
        }

        auto* previous_available_block_for_buddies = get_previous_available_block(buddies_address);
        auto* next_available_block_for_buddies = get_next_available_block(buddies_address);

        if (check_free_block(target_block))
        {

        }



        buddies_address = get_buddy(target_block, target_block_size);
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

inline allocator *allocator_buddies_system::get_allocator() const
{
    return *reinterpret_cast<allocator**>(reinterpret_cast<unsigned char *>(_trusted_memory));
}

std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info() const noexcept
{
    std::vector<allocator_test_utils::block_info> blocks_info;
    return blocks_info;
}

inline logger *allocator_buddies_system::get_logger() const
{
    return *reinterpret_cast<logger **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *));
}

inline std::string allocator_buddies_system::get_typename() const noexcept
{
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->debug("ALLOCATOR_BUDDIES_SYSTEM: method get_typename start");
    }

    // TODO: need to write something

    if (log != nullptr)
    {
        log->debug("ALLOCATOR_BUDDIES_SYSTEM: method get_typename finish");
    }
}

std::mutex* allocator_buddies_system::get_mutex() noexcept
{
    return reinterpret_cast<std::mutex*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
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
