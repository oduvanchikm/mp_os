//#include <not_implemented.h>
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

void allocator_buddies_system::set_next_available_block(void* previous_block, void* next_block) noexcept
{
    // получаем указатель на следующий свободный блок после previous_block
    void* previous_next_ptr = *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(previous_block) + sizeof(void*));

    // устанавливаем указатель на след блок
    previous_next_ptr = next_block;
}

[[nodiscard]] void *allocator_buddies_system::allocate(size_t value_size, size_t values_count)
{
    auto requested_size_mult = value_size * values_count; // перемножаем, чтобы понять, сколько нужно

    auto requested_size = closest_power_of_two(requested_size_mult); // наверное, нужно так сделать, потому что живем в мире степени двойки

    if (requested_size < sizeof(block_pointer_t) + sizeof(block_size_t))
    {
        requested_size = sizeof(block_pointer_t) + sizeof(block_size_t);
        warning_with_guard("request space size was changed\n");
    }

    // 1. начинаем поиск подходящего свободного блока

    allocator_with_fit_mode::fit_mode fit_mode = get_fit_mode();
    void* target_block = nullptr; // указатель на блок, который мы выделяем

    {
        void* next_block = nullptr; // указатель на следующий свободный блок
        void* current_block = get_first_available_block(); // указатель на текущий свободный блок

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
                    target_block = current_block; // сохраняем указатель на найденный блок
                }
            }

            next_block = get_next_available_block(current_block); // получаем указатель на следующий свободный блок
            current_block = next_block; // переходим к следующему блоку
        }

        if (target_block == nullptr) // если не нашли свободный блок, то выбрасываем еррор
        {
            error_with_guard("can't allocate\n");
            throw std::bad_alloc();
        }

        _allocated_block = target_block; // сохраняем адрес подходящего блока, хз зачем

        // 2. удаление из списка найденного свободного блока во избежание дублирования

        void* previous_block = get_previous_available_block(target_block); // получаем адрес предыдущего блока
        next_block = get_next_available_block(target_block);
        set_next_available_block(previous_block, next_block); // обновляем указатель

        size_t target_block_size = get_available_block_size(target_block);

//        int j = log2(block_size); // Находим j
//        if (get_(j) == target_block)
//        {
//            set_first_available_block_of_size(j, get_next_available_block(target_block));
//        }

        // имеем однобитовое поле tag: если блок свободен = 1, если занят = 0
        *reinterpret_cast<unsigned char*>(target_block) |= 1 ;

        // 3. проверяем, требуется ли разделение блоков
        while ((target_block_size << 1) >= (requested_size + get_ancillary_space_size()))
        {
            // 4. разделяем блоки пополам




        }
    }
}

void allocator_buddies_system::deallocate(void *at)
{

}

inline void allocator_buddies_system::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{

}

inline allocator *allocator_buddies_system::get_allocator() const
{
    return *reinterpret_cast<allocator**>(reinterpret_cast<unsigned char *>(_trusted_memory));
}

std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info() const noexcept
{

}

inline logger *allocator_buddies_system::get_logger() const
{
    return *reinterpret_cast<logger **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *));
}

inline std::string allocator_buddies_system::get_typename() const noexcept
{
    std::string message = "allocator buddies system";
    return message;
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
