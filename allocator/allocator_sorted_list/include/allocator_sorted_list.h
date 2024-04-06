#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_SORTED_LIST_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_SORTED_LIST_H

#include <allocator_guardant.h>
#include "../../../logger/client_logger/include/client_logger.h"
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <logger_guardant.h>
#include <typename_holder.h>
#include <mutex>
#include <sstream>
#include <thread>

class allocator_sorted_list final:
        private allocator_guardant,
        public allocator_test_utils,
        public allocator_with_fit_mode,
        private logger_guardant,
        private typename_holder
{

private:

    void *_trusted_memory;

public:

    ~allocator_sorted_list() override;

    allocator_sorted_list(allocator_sorted_list const &other) = delete;

    allocator_sorted_list &operator=(allocator_sorted_list const &other) = delete;

    allocator_sorted_list(allocator_sorted_list &&other) noexcept;

    allocator_sorted_list &operator=(allocator_sorted_list &&other) noexcept;

public:

    explicit allocator_sorted_list(
            size_t space_size,
            allocator *parent_allocator = nullptr,
            logger *logger = nullptr,
            allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);


public:

    [[nodiscard]] void *allocate(
            size_t value_size,
            size_t values_count) override;

    void deallocate(void *at) override;

public:

    inline void set_fit_mode(allocator_with_fit_mode::fit_mode mode) override;

private:

    inline allocator *get_allocator() const override;

public:

    std::vector<allocator_test_utils::block_info> get_blocks_info() const noexcept override;

private:

    inline logger *get_logger() const override;

private:

    inline std::string get_typename() const noexcept override;


private:

    size_t get_ancillary_space_size() const noexcept;

    allocator_with_fit_mode::fit_mode get_fit_mode() const noexcept;

    void *get_first_aviable_block() const noexcept;

    std::mutex *get_mutex() const noexcept;

private:

    void *get_aviable_block_next_block_address(void *block_address) noexcept;

    size_t get_small_metadata() const noexcept;

    bool is_block_occupied(void* block) const noexcept;

    size_t get_aviable_block_size(void *block_address) const noexcept;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_SORTED_LIST_H