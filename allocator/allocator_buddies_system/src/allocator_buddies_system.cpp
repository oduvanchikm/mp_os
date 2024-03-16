#include <not_implemented.h>

#include "../include/allocator_buddies_system.h"

// метаданные =
// размер аллокатора + размер логгера + размер всей памяти + размер указателя на первый блок + указатель на объект синхронизации (эт потом будет)
size_t allocator_buddies_system::get_ancillary_space_size() const noexcept
{
    return sizeof(logger*) + sizeof(allocator*) + sizeof(size_t) + sizeof(void*) + sizeof(allocator_with_fit_mode::fit_mode);
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
    if (space_size < sizeof(block_size_t) + sizeof(block_pointer_t)) // если размер запрошенной памяти меньше, чем размер для хранения одного блока памяти
    {
        if (logger != nullptr)
        {
            logger->error("ERROR");
        }

        throw std::logic_error("can't initialize allocator instance");
    }
    // находим ближайшее большее число степени двойки
    auto new_space_size = closest_power_of_two(space_size);
    // считаем общий размер памяти с метаданными
    auto common_size = new_space_size + get_ancillary_space_size();
    try
    {
        _trusted_memory = parent_allocator == nullptr
                          ? ::operator new(common_size) // запрашиваем размер
                          : parent_allocator->allocate(common_size, 1);
    }
    catch (std::bad_alloc const &ex)
    {
        logger->error("ERROR");
    }

    // проинициализировали аллокатор
    auto** parent_allocator_space_address = reinterpret_cast<allocator**>(_trusted_memory);
    *parent_allocator_space_address = parent_allocator;

    // проинициализировали логгер
    auto** logger_space_address = reinterpret_cast<class logger**>(parent_allocator_space_address + 1);
    *logger_space_address = logger;

    // записали размер памяти, которую хотим выделить
    size_t* space_size_space_address = reinterpret_cast<size_t*>(logger_space_address + 1);
    *space_size_space_address = space_size;

    // указатель на первый блок памяти в выделенном пространстве
    void** first_block_address = reinterpret_cast<void**>(space_size_space_address + 1);
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
        logger->trace("allocator is created\n");
    }
}

allocator_with_fit_mode::fit_mode allocator_buddies_system::get_fit_mode() const noexcept
{
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t));
}

void* allocator_buddies_system::get_first_available_block() const noexcept
{
    return reinterpret_cast<void**>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode))
}

allocator::block_size_t allocator_buddies_system::get_first_available_block_size(void* block_address) noexcept
{
    return reinterpret_cast<allocator::block_size_t>(block_address);
}

[[nodiscard]] void *allocator_buddies_system::allocate(size_t value_size, size_t values_count)
{
    auto requested_size = value_size * values_count; // перемножаем, чтобы понять, сколько нужно
    // думаю,что здесь

    allocator_with_fit_mode::fit_mode fit_mode = get_fit_mode();
    void* current_block = get_first_available_block();
    size_t current_block_size = get_first_available_block_size(current_block);

    {
        while (current_block != nullptr)
        {

        }
    }



    return this;
}

void allocator_buddies_system::deallocate(void *at)
{
    throw not_implemented("void allocator_buddies_system::deallocate(void *)", "your code should be here...");
}

inline void allocator_buddies_system::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    throw not_implemented("inline void allocator_buddies_system::set_fit_mode(allocator_with_fit_mode::fit_mode)", "your code should be here...");
}

inline allocator *allocator_buddies_system::get_allocator() const
{
    return *reinterpret_cast<allocator**>(reinterpret_cast<unsigned char *>(_trusted_memory));
}

std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info() const noexcept
{
    throw not_implemented("std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info() const noexcept", "your code should be here...");
}

inline logger *allocator_buddies_system::get_logger() const
{
    return *reinterpret_cast<logger **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *));
}

inline std::string allocator_buddies_system::get_typename() const noexcept
{
    throw not_implemented("inline std::string allocator_buddies_system::get_typename() const noexcept", "your code should be here...");
}

allocator_buddies_system::~allocator_buddies_system()
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
        logger->trace("allocator is deleted\n");
    }
}
