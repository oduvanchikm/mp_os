#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BUDDIES_SYSTEM_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BUDDIES_SYSTEM_H

#include <allocator_guardant.h>
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <logger_guardant.h>
#include <typename_holder.h>
#include <cmath>
#include <mutex>

class allocator_buddies_system final:
    private allocator_guardant,
    public allocator_test_utils,
    public allocator_with_fit_mode,
    private logger_guardant,
    private typename_holder
{

private:

    void *_trusted_memory; // указатель на выделенную память
//    std::mutex _buddies_system_mutex; // мьютекс

public:

    ~allocator_buddies_system() override;

    allocator_buddies_system(
        allocator_buddies_system const &other) = delete;

    allocator_buddies_system &operator=(
        allocator_buddies_system const &other) = delete;

    allocator_buddies_system(
        allocator_buddies_system &&other) noexcept = delete;

    allocator_buddies_system &operator=(
        allocator_buddies_system &&other) noexcept = delete;

public:

    explicit allocator_buddies_system(
        size_t space_size_power_of_two,
        allocator *parent_allocator = nullptr,
        logger *logger = nullptr,
        allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);

public:

    [[nodiscard]] void *allocate(
        size_t value_size,
        size_t values_count) override;

    void deallocate(
        void *at) override;

public:

    inline void set_fit_mode(
        allocator_with_fit_mode::fit_mode mode) override;

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

private:

    size_t closest_power_of_two(size_t number);

private:

    allocator_with_fit_mode::fit_mode get_fit_mode() const noexcept;
    void* get_first_available_block() const noexcept;

private:

    static block_size_t get_available_block_size(void* block_address) noexcept;

    void* get_next_available_block(void* block_address) noexcept;

    void* get_previous_available_block(void* block) noexcept;

    void* get_buddy(void* target_block_first_buddy, size_t target_block_size_first_buddy) noexcept;

    std::mutex* get_mutex() noexcept;

    void* get_start_allocated_memory_address() noexcept;

    bool check_free_block(void* target_block) const;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BUDDIES_SYSTEM_H
