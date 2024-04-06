#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_H

#include <cstddef>

class allocator
{

public:
<<<<<<< HEAD

    typedef size_t block_size_t; // размер блока
    typedef void *block_pointer_t; // указатель на блок памяти
=======
    
    typedef size_t block_size_t;
    
    typedef void *block_pointer_t;
>>>>>>> 5ade9435e0702eaa7d8713a809c05debdb627456

public:
    
    virtual ~allocator() noexcept = default;

public:
<<<<<<< HEAD
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

=======
    
    template<
        typename T,
        typename ...args>
    inline static void construct(
        T *at,
        args... constructor_arguments);
    
    template<
        typename T>
    inline static void destruct(
        T *at);

public:
    
    [[nodiscard]] virtual void *allocate(
        size_t value_size,
        size_t values_count) = 0;
    
    virtual void deallocate(
        void *at) = 0;
    
>>>>>>> 5ade9435e0702eaa7d8713a809c05debdb627456
};

template<typename T, typename ...args>
inline void allocator::construct( T *at, args... constructor_arguments)
{
    new(at) T(constructor_arguments...);
}

<<<<<<< HEAD
template<typename T>
inline void allocator::destruct(T* at)
=======
template<
    typename T>
inline void allocator::destruct(
    T *at)
>>>>>>> 5ade9435e0702eaa7d8713a809c05debdb627456
{
    at->~T();
}

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_H