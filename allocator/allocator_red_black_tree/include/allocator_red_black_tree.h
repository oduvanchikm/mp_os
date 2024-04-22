#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H

#include <allocator_guardant.h>
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <logger_guardant.h>
#include <typename_holder.h>
#include <mutex>

class allocator_red_black_tree final:
    private allocator_guardant,
    public allocator_test_utils,
    public allocator_with_fit_mode,
    private logger_guardant,
    private typename_holder
{

private:
    
    void *_trusted_memory;

public:
    
    ~allocator_red_black_tree() override;
    
    allocator_red_black_tree(
        allocator_red_black_tree const &other) = delete;
    
    allocator_red_black_tree &operator=(
        allocator_red_black_tree const &other) = delete;
    
    allocator_red_black_tree(
        allocator_red_black_tree &&other) noexcept;
    
    allocator_red_black_tree &operator=(
        allocator_red_black_tree &&other) noexcept;

public:
    
    explicit allocator_red_black_tree(
        size_t space_size,
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

    inline size_t get_ancillary_space_size() const noexcept;

    std::mutex *get_mutex() const noexcept;

    size_t get_common_size_of_allocator() const;

    size_t get_free_size_of_allocator() const;

    allocator_with_fit_mode::fit_mode get_fit_mode() const noexcept;

    void *get_root() const noexcept;

    void delete_block(void* block_address) noexcept;

    unsigned char get_is_occupied_block(void* block_address) const noexcept;

    unsigned char get_colour_block(void* block_address) const noexcept;

    size_t get_small_free_metadata() const noexcept;

    void* get_next_free_block(void* block_address) const noexcept;

    void* get_previous_free_block(void* block_address) const noexcept;

    size_t get_size_block(void* block_address) const noexcept;

    void* get_parent_block(void* block_address) const noexcept;

    void* get_left_subtree_block(void* block_address) const noexcept;

    void* get_right_subtree_block(void* block_address) const noexcept;

    void small_right_rotation(void* address_block) noexcept;

    void big_right_rotation(void* address_block) noexcept;

    void small_left_rotation(void* address_block) noexcept;

    void big_left_rotation(void* address_block) noexcept;

    void* get_best_fit(size_t size) const noexcept;

    void* get_worst_fit(size_t size) const noexcept;

    void* get_first_fit(size_t size) const noexcept;

    void insert(void* target_block) noexcept;

    void fix_red_black_tree(void* target_block) noexcept;
    
};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H