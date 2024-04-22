#include "../include/allocator_boundary_tags.h"

allocator_boundary_tags::~allocator_boundary_tags()
{
    delete get_mutex();

    auto* allocator = get_allocator();

    if (allocator == nullptr)
    {
        ::operator delete(_trusted_memory);
    }
    else
    {
        allocator->deallocate(_trusted_memory);
    }
}

allocator_boundary_tags::allocator_boundary_tags(allocator_boundary_tags &&other) noexcept :
        _trusted_memory(other._trusted_memory)
{
    other._trusted_memory = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(allocator_boundary_tags &&other) noexcept
{
    if (this != &other)
    {
        allocator* all = get_allocator();

        if (all != nullptr)
        {
            all->deallocate(_trusted_memory);
        }
        else
        {
            ::operator delete(_trusted_memory);
        }

        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }
    return *this;
}

size_t allocator_boundary_tags::get_small_meta_size() const noexcept
{
    return sizeof(void*) + sizeof(void*) + sizeof(size_t);
}

size_t allocator_boundary_tags::get_ancillary_space_size() const noexcept
{
    return sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t) + sizeof(void*);
}

inline allocator *allocator_boundary_tags::get_allocator() const noexcept
{
    return *reinterpret_cast<allocator**>(_trusted_memory);
}

inline logger* allocator_boundary_tags::get_logger() const
{
    return *reinterpret_cast<logger**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*));
}

inline size_t allocator_boundary_tags::get_size_of_allocator() const noexcept
{
    return *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*));
}

inline allocator_with_fit_mode::fit_mode allocator_boundary_tags::get_fit_mode() const noexcept
{
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t));
}

inline std::mutex* allocator_boundary_tags::get_mutex() const noexcept
{
    return *reinterpret_cast<std::mutex**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
}

inline void* allocator_boundary_tags::get_first_occupied_block() const noexcept
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
}

size_t allocator_boundary_tags::get_size_occupied_block(void* block_address) const noexcept
{
    return *reinterpret_cast<size_t*>(block_address);
}

inline void* allocator_boundary_tags::get_previous_occupied_block(void* block_address) const noexcept
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block_address) + sizeof(size_t));
}

inline void* allocator_boundary_tags::get_next_occupied_block(void* block_address) const noexcept
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block_address) + sizeof(size_t) + sizeof(void*));
}

allocator_boundary_tags::allocator_boundary_tags(
        size_t space_size,
        allocator *parent_allocator,
        logger *logger,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
//    if (logger != nullptr)
//    {
//        my_logger(get_typename() + "the beginning of the constructor's work, allocator start creating", logger::severity::trace);
//        trace_with_guard(get_typename() + "the beginning of the constructor's work, allocator start creating");
//    }

    if (space_size < get_small_meta_size())
    {
        if (logger != nullptr)
        {
            logger->error(get_typename() + "can't initialize allocator instance");
        }
        throw std::logic_error(get_typename() + "can't initialize allocator instance");
    }

    size_t common_size = space_size + get_ancillary_space_size();

    try
    {
        _trusted_memory = parent_allocator == nullptr
                          ? ::operator new(common_size)
                          : parent_allocator->allocate(common_size, 1);
    }
    catch (std::bad_alloc const &ex)
    {
        logger->error(get_typename() + "error with allocate memory");
    }

    // meta data allocator

    allocator** parent_allocator_space_address = reinterpret_cast<allocator**>(_trusted_memory);
    *parent_allocator_space_address = parent_allocator;

    class logger** logger_space_address = reinterpret_cast<class logger**>(parent_allocator_space_address + 1);
    *logger_space_address = logger;

    size_t* space_size_space_address = reinterpret_cast<size_t*>(logger_space_address + 1);
    *space_size_space_address = space_size;

    allocator_with_fit_mode::fit_mode* fit_mode = reinterpret_cast<allocator_with_fit_mode::fit_mode*>(space_size_space_address + 1);
    *fit_mode = allocate_fit_mode;

    std::mutex** mutex_space_size = reinterpret_cast<std::mutex**>(fit_mode + 1);
    auto* mutex_boundary_tags = new std::mutex;
    *mutex_space_size = mutex_boundary_tags;

    size_t* size_available_blocks = reinterpret_cast<size_t*>(mutex_space_size + 1);
    *size_available_blocks = space_size;

    void** first_occupied_block = reinterpret_cast<void**>(size_available_blocks + 1);
    *first_occupied_block = first_occupied_block + 1;

//    if (logger != nullptr)
//    {
//        logger->trace(get_typename() + "allocator created");
//    }
}

unsigned char* allocator_boundary_tags::get_end_of_block(void* block_address) const noexcept
{
    size_t size_of_occupied_block = *reinterpret_cast<size_t*>(block_address);

    size_t variable = size_of_occupied_block + get_small_meta_size();

    return reinterpret_cast<unsigned char*>(block_address) + variable;
}

unsigned char* allocator_boundary_tags::get_start_of_block(void* block_address) const noexcept
{
    return reinterpret_cast<unsigned char*>(block_address);
}

[[nodiscard]] void *allocator_boundary_tags::allocate(size_t value_size, size_t values_count)
{
    std::mutex* mutex_boundary_tags = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_boundary_tags);

//    trace_with_guard(get_typename() + "method of allocate has started");

    size_t requested_size = value_size * values_count + get_small_meta_size();

    allocator_with_fit_mode::fit_mode mode = get_fit_mode();

    void* previous_to_target_block = nullptr;
    void* target_block = nullptr;
    void* next_to_target_block = nullptr;

    size_t size = 0;

    {
        void* current_block = get_first_occupied_block();
        void* previous_block = nullptr;

        if (current_block == nullptr)
        {
            unsigned char* end_of_previous_block = reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_space_size();
            unsigned char* start_of_current_block = reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_space_size() + get_size_of_allocator();

            size_t size_of_block = start_of_current_block - end_of_previous_block;

            if (size_of_block >= requested_size)
            {
                target_block = reinterpret_cast<void*>(end_of_previous_block);
                size = size_of_block;
            }
        }
        else
        {
            while (current_block != nullptr)
            {
                unsigned char *end_of_previous_block = nullptr;
                unsigned char *start_of_current_block = reinterpret_cast<unsigned char*>(current_block);

                if (previous_block == nullptr)
                {
                    end_of_previous_block =
                            reinterpret_cast<unsigned char *>(_trusted_memory) + get_ancillary_space_size();
                }
                else
                {
                    end_of_previous_block = get_end_of_block(previous_block);
                }

                size_t current_block_size = start_of_current_block - end_of_previous_block;

                if (current_block_size >= requested_size &&
                    (mode == allocator_with_fit_mode::fit_mode::first_fit ||
                     mode == allocator_with_fit_mode::fit_mode::the_best_fit &&
                     (target_block == nullptr || size > current_block_size) ||
                     mode == allocator_with_fit_mode::fit_mode::the_worst_fit &&
                     (target_block == nullptr || size < current_block_size)))
                {
                    previous_to_target_block = previous_block;
                    target_block = reinterpret_cast<void*>(end_of_previous_block);
                    size = current_block_size;
                    next_to_target_block = get_next_occupied_block(current_block);

                    if (mode == allocator_with_fit_mode::fit_mode::first_fit)
                    {
                        break;
                    }
                }

                previous_block = current_block;
                current_block = get_next_occupied_block(current_block);
            }

            unsigned char* end_of_previous_block = reinterpret_cast<unsigned char*>(previous_block) + get_small_meta_size() + *reinterpret_cast<size_t*>(previous_block);
            unsigned char* start_of_current_block = reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_space_size() + get_size_of_allocator();

            size_t current_block_size = start_of_current_block - end_of_previous_block;

            if (current_block_size >= requested_size &&
                (mode == allocator_with_fit_mode::fit_mode::first_fit ||
                 mode == allocator_with_fit_mode::fit_mode::the_best_fit &&
                 (target_block == nullptr || size > current_block_size) ||
                 mode == allocator_with_fit_mode::fit_mode::the_worst_fit &&
                 (target_block == nullptr || size < current_block_size)))
            {
                previous_to_target_block = previous_block;
                size = current_block_size;
//                target_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(previous_block) + *reinterpret_cast<size_t*>(previous_block) + get_small_meta_size());
                target_block = reinterpret_cast<void*>(end_of_previous_block);
                next_to_target_block = nullptr;
            }
        }
    }

    if (target_block == nullptr)
    {
        error_with_guard(get_typename() + "memory allocation error")
                ->trace_with_guard(get_typename() + "method of allocate has finished");
        throw std::bad_alloc();
    }

    size_t* size_available_blocks_allocator = reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));

    void* first_block = get_first_occupied_block();

    if (first_block == nullptr)
    {
        *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) +
                                   sizeof(allocator *) + sizeof(logger *) +
                                   sizeof(size_t) +
                                   sizeof(allocator_with_fit_mode::fit_mode) +
                                   sizeof(std::mutex *) + sizeof(size_t)) = target_block;

        *reinterpret_cast<size_t*>(target_block) = requested_size - get_small_meta_size();
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(size_t)) = nullptr;
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(size_t) + sizeof(void*)) = nullptr;
    }
    else
    {
        size_t size_without_meta_data = requested_size;

        *reinterpret_cast<size_t*>(target_block) = requested_size - get_small_meta_size();

        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(size_t)) = previous_to_target_block;

        if (previous_to_target_block == nullptr)
        {
            void **first_occupied_block = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) +
                                                                    sizeof(allocator *) + sizeof(logger *) +
                                                                    sizeof(size_t) +
                                                                    sizeof(allocator_with_fit_mode::fit_mode) +
                                                                    sizeof(std::mutex *) + sizeof(size_t));
            *first_occupied_block = target_block;

        }
        else
        {
            void **next_to_previous_to_target_block = reinterpret_cast<void **>(
                    reinterpret_cast<unsigned char *>(previous_to_target_block) + sizeof(size_t) +
                    sizeof(void *));

            *next_to_previous_to_target_block = target_block;
        }

        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(size_t) + sizeof(void*)) = next_to_target_block;

        if (next_to_target_block != nullptr)
        {
            void **previous_block_to_next_block = reinterpret_cast<void **>(
                    reinterpret_cast<unsigned char *>(target_block) + sizeof(size_t));
            *previous_block_to_next_block = target_block;
        }
    }

    (*size_available_blocks_allocator) -= requested_size;

    std::string string_with_info;
    std::vector<allocator_test_utils::block_info> blocks_information = get_blocks_info();

    for (block_info information : blocks_information)
    {
        std::string free_or_occupied = information.is_block_occupied ? "occupied" : "free";
        string_with_info += (free_or_occupied + "  " + std::to_string(information.block_size) + " | ");
    }

    return reinterpret_cast<unsigned char*>(target_block) + get_small_meta_size();
}

void allocator_boundary_tags::deallocate(void *at)
{
    std::mutex* mutex_boundary_tags = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_boundary_tags);

    void* target_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(at) - sizeof(size_t) - 2 * sizeof(void*));

    size_t target_block_size = get_size_occupied_block(target_block) + get_small_meta_size();

    void* first_occupied_block = get_first_occupied_block();

    std::string result;
    auto* bytes = reinterpret_cast<unsigned char*>(at);

    size_t size_object = get_size_occupied_block(target_block);

    for (int i = 0; i < size_object; i++)
    {
        result += std::to_string(static_cast<int>(bytes[i])) + " ";
    }
//    log_with_guard_my("state block:" + result, logger::severity::debug);

    size_t* size_available_blocks_allocator = reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*));

    if (first_occupied_block == nullptr)
    {
        throw std::logic_error(get_typename() + "error with deallocate memory");
    }
    else
    {
        void* previous_to_target_block = get_previous_occupied_block(target_block);
        void* next_to_target_block = get_next_occupied_block(target_block);

        if (get_first_occupied_block() == target_block)
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t)) = target_block;
        }

        if (previous_to_target_block != nullptr)
        {
            void** next_block_for_previous_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(previous_to_target_block) + sizeof(size_t) + sizeof(void*));
            *next_block_for_previous_block = next_to_target_block;
        }

        if (next_to_target_block != nullptr)
        {
            void** previous_to_next_to_target_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(next_to_target_block) + sizeof(size_t));
            *previous_to_next_to_target_block = previous_to_target_block;
        }
    }

    (*size_available_blocks_allocator) += (target_block_size + get_small_meta_size());

    std::string string_with_info;
    std::vector<allocator_test_utils::block_info> blocks_information = get_blocks_info();

    for (block_info information : blocks_information)
    {
        std::string free_or_occupied = information.is_block_occupied ? "occupied" : "free";
        string_with_info += (free_or_occupied + "  " + std::to_string(information.block_size) + " | ");
    }

}

inline void allocator_boundary_tags::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    std::mutex* mutex_boundary_tags = get_mutex();
    std::lock_guard<std::mutex> lock(*mutex_boundary_tags);

//    trace_with_guard(get_typename() + "set fit mode method has started");

    *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator**) + sizeof(logger*) + sizeof(size_t)) = mode;

//    trace_with_guard(get_typename() + "set fir mode method has finished");
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const noexcept
{
    std::vector<allocator_test_utils::block_info> data;

    void** first_occupied_block_l = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex*) + sizeof(size_t));
    void* first_occupied_block = reinterpret_cast<void **>(first_occupied_block_l  + 1);


    void* current_occupied_block = get_first_occupied_block();

    if (current_occupied_block == nullptr)
    {
        allocator_test_utils::block_info value_1;

        value_1.block_size = get_size_of_allocator();
        value_1.is_block_occupied = 0;

        data.push_back(std::move(value_1));

        return data;
    }
    else
    {
        void* previous_block = nullptr;
        allocator_test_utils::block_info value_2;

        if (current_occupied_block == first_occupied_block)
        {
//            value_2.block_size = *reinterpret_cast<size_t*>(current_occupied_block);
//            value_2.is_block_occupied = 1;
//
//            previous_block = current_occupied_block;
//            current_occupied_block = get_next_occupied_block(current_occupied_block);
//
//            data.push_back(std::move(value_2));
//            std::cout << "kkkkkkkkkkkkkkkkkkkk" << std::endl;
//            unsigned char* after_meta_data = reinterpret_cast<unsigned char *>(_trusted_memory) + get_ancillary_space_size();
//
//            if ((reinterpret_cast<unsigned char*>(current_occupied_block) - after_meta_data) > 0)
//            {
//                size_t size_free_block = reinterpret_cast<unsigned char *>(current_occupied_block) - after_meta_data;
//
//                std::cout << "sizee " << size_free_block << std::endl;
//
//                value_2.block_size = size_free_block;
//                value_2.is_block_occupied = 0;
//
//                data.push_back(std::move(value_2));
//            }
//            else
//            {
//                allocator_test_utils::block_info value_3;
//
//                size_t size_occupied_block = get_size_occupied_block(current_occupied_block);
//
//                value_3.block_size = size_occupied_block;
//                value_3.is_block_occupied = 1;
//
//                data.push_back(std::move(value_3));
//            }
//        }
//        else
//        {
//            void *previous_to_current_block = get_previous_occupied_block(current_occupied_block);
//            unsigned char *end_of_previous_occupied_block = get_end_of_block(previous_to_current_block);
//
//            if ((reinterpret_cast<unsigned char *>(current_occupied_block) - end_of_previous_occupied_block) > 0)
//            {
//                allocator_test_utils::block_info value_4;
//
//                size_t size_free_block = reinterpret_cast<unsigned char *>(current_occupied_block) - end_of_previous_occupied_block;
//
//                value_4.block_size = size_free_block;
//                value_4.is_block_occupied = 0;
//
//                data.push_back(std::move(value_4));
//            }
//
//
//            else
//            {
//                allocator_test_utils::block_info value_5;
//
//                size_t size_occupied_block = get_size_occupied_block(current_occupied_block);
//
//                value_5.block_size = size_occupied_block;
//                value_5.is_block_occupied = 1;
//
//                data.push_back(std::move(value_5));
//            }




            allocator_test_utils::block_info value_2;
            unsigned char* end_of_block = reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_space_size();
            unsigned char* start_of_block = reinterpret_cast<unsigned char*>(current_occupied_block);

            if (start_of_block - end_of_block > 0)
            {
                value_2.is_block_occupied = 0;
                value_2.block_size = start_of_block - end_of_block;
                data.push_back(value_2);
            }

            allocator_test_utils::block_info obj2;

            obj2.block_size = *reinterpret_cast<size_t*>(current_occupied_block);
            obj2.is_block_occupied = 1;

            data.push_back(std::move(obj2));

            previous_block = current_occupied_block;
            current_occupied_block = get_next_occupied_block(current_occupied_block);
        }
        while (current_occupied_block != nullptr)
        {
            allocator_test_utils::block_info obj1;
            unsigned char* end_of_block = reinterpret_cast<unsigned char*>(previous_block) + get_small_meta_size() + *reinterpret_cast<size_t*>(previous_block);
            unsigned char* start_of_block = reinterpret_cast<unsigned char*>(current_occupied_block);

            if (start_of_block - end_of_block > 0)
            {
                obj1.is_block_occupied = 0;
                obj1.block_size = start_of_block - end_of_block;
                data.push_back(std::move(obj1));
            }

            allocator_test_utils::block_info obj2;

            obj2.block_size = *reinterpret_cast<size_t*>(current_occupied_block);
            obj2.is_block_occupied = 1;

            data.push_back(obj2);

            previous_block = current_occupied_block;
            current_occupied_block = get_next_occupied_block(current_occupied_block);
        }

        unsigned char* end =  reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_space_size() + get_size_of_allocator();
        unsigned char* start = reinterpret_cast<unsigned char*>(previous_block) + get_small_meta_size() + *reinterpret_cast<size_t*>(previous_block);

        if (start != end)
        {
            allocator_test_utils::block_info obj;
            obj.block_size = end - start;
            obj.is_block_occupied = 0;

            data.push_back(std::move(obj));
        }
    }
    return data;
}

inline std::string allocator_boundary_tags::get_typename() const noexcept
{
    return "ALLOCATOR_BOUNDARY_TAGS: ";
}

void allocator_boundary_tags::my_logger(std::string const &message, logger::severity severity) const
{
    logger *got_logger = get_logger();

    if (got_logger != nullptr)
    {
        got_logger->log(message, severity);
    }
}