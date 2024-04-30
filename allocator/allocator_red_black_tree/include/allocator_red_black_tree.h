#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H
#include <allocator_guardant.h>
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <logger_guardant.h>
#include <typename_holder.h>
#include <iostream>
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
            allocator_red_black_tree &&other) noexcept = delete;

    allocator_red_black_tree &operator=(
            allocator_red_black_tree &&other) noexcept = delete;

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

private:

    size_t get_ancillary_space_size() const noexcept;

    allocator_with_fit_mode::fit_mode get_fit_mode() const noexcept;

    void *get_root() const noexcept;

    std::mutex *get_mutex() const noexcept;

    size_t get_meta_size_aviable_block() const noexcept;

    size_t get_meta_size_occupied_block() const noexcept;

    void* get_previous_block(void* block) const;

    void* get_next_block(void* block) const;

    void* get_parent(void* block) const;

    void* get_left_subtree_block(void* block) const;

    void* get_right_subtree_block(void* block) const;

    size_t get_size_aviable_block(void* block) const;

    bool is_color_red(void* block) const;

    bool is_occupied_block(void* block) const;

    static void change_color(void* block);

    void* get_uncle(void* block);

    void* get_worst_fit(size_t requested_size);

    void* get_best_fit(size_t requested_size);

    void* get_first_fit(size_t requested_size);

    void fix_red_black_tree_after_insert(void* start_block, bool color_parent_block);

    void insert_block_in_red_black_tree(void* block, size_t size);

    void dispose_block_from_red_black_tree(void* block);

    void left_rotate(void* block);

    void right_rotate(void* block);

    void fix_red_black_tree_after_dispose(void* block, bool);

    size_t get_all_size() const;

public:

    std::vector<allocator_test_utils::block_info> get_blocks_info() const noexcept override;

private:

    inline logger *get_logger() const override;

private:

    inline std::string get_typename() const noexcept override;

private:

    void log_with_guard_my(
            std::string const &message,
            logger::severity severity) const;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H