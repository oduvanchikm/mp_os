#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_H

#include <cstddef>

class allocator
{

public:

    typedef size_t block_size_t; // размер блока
    typedef void *block_pointer_t; // указатель на блок памяти

public:

    virtual ~allocator() noexcept = default;

public:
//    методы для операции вызова конструктора объекта, который находится на уровне блока памяти

    template<typename T, typename ...args>
    inline static void construct(T *at, args... constructor_arguments);

    template<typename T>
    inline static void destruct(T* at);

public:
    //   выделить память, value_size - размер значения, которое мы хотим выделить в байтах,
    //   values_count - количество
    [[nodiscard]] virtual void *allocate(size_t value_size, size_t values_count) = 0;

    //   освободить память
    virtual void deallocate(void *at) = 0;

};

template<typename T, typename ...args>
inline void allocator::construct( T *at, args... constructor_arguments)
{
    new (at) T(constructor_arguments...);
}

template<typename T>
inline void allocator::destruct(T* at)
{
    at->~T();
}

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_H