#include "../include/allocator_guardant.h"

void *allocator_guardant::allocate_with_guard(
    size_t value_size,
    size_t values_count) const
{
    allocator *target_allocator = get_allocator(); // запросили аллокатор
    return target_allocator == nullptr
        ? ::operator new(value_size * values_count) // если нет, то выделит кусочек памяти
        : target_allocator->allocate(value_size, values_count); // если есть, то запрашиваем блок
}

void allocator_guardant::deallocate_with_guard(void *at) const
{
    allocator *target_allocator = get_allocator();
    return target_allocator == nullptr
        ? ::operator delete(at)
        : target_allocator->deallocate(at);
}