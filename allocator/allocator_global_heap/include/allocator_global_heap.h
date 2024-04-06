#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_GLOBAL_HEAP_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_GLOBAL_HEAP_H

#include <allocator.h>
#include <logger.h>
#include <logger_guardant.h>
#include <typename_holder.h>
#include <logger_builder.h>
#include "../../../logger/client_logger/include/client_logger.h"
//#include "../../../logger/client_logger/include/client_logger_builder.h"
//#include <client_logger.h>
//#include <client_logger_builder.h>

class allocator_global_heap final:
    public allocator,
    private logger_guardant,
    private typename_holder
{

private:
    
    logger *_logger;

public:
    
    explicit allocator_global_heap(
        logger *logger = nullptr);
    
    ~allocator_global_heap() override;
    
    allocator_global_heap(
        allocator_global_heap const &other) = delete;
    
    allocator_global_heap &operator=(
        allocator_global_heap const &other) = delete;
    
    allocator_global_heap(
<<<<<<< HEAD
        allocator_global_heap &&other) noexcept = delete;

=======
        allocator_global_heap &&other) noexcept;
    
>>>>>>> 5ade9435e0702eaa7d8713a809c05debdb627456
    allocator_global_heap &operator=(
        allocator_global_heap &&other) noexcept = delete;

public:
    
    [[nodiscard]] void *allocate(
        size_t value_size,
        size_t values_count) override;
    
    void deallocate(
        void *at) override;

public:
    
    void foo()
    {};

private:
    
    inline logger *get_logger() const override;

private:
    
    inline std::string get_typename() const noexcept override;

<<<<<<< HEAD
    std::string get_block_of_memory_state(void *at) const;
=======
public:
>>>>>>> 5ade9435e0702eaa7d8713a809c05debdb627456

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_GLOBAL_HEAP_H