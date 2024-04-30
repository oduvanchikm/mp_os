#include <gtest/gtest.h>
#include <logger.h>
#include <logger_builder.h>
#include <client_logger_builder.h>
#include <list>

#include "../include/allocator_red_black_tree.h"

logger *create_logger(
        std::vector<std::pair<std::string, logger::severity>> const &output_file_streams_setup,
        bool use_console_stream = true,
        logger::severity console_stream_severity = logger::severity::debug)
{
    logger_builder *builder = new client_logger_builder();

    if (use_console_stream)
    {
        builder->add_console_stream(console_stream_severity);
    }

    for (auto &output_file_stream_setup: output_file_streams_setup)
    {
        builder->add_file_stream(output_file_stream_setup.first, output_file_stream_setup.second);
    }

    logger *built_logger = builder->build();

    delete builder;

    return built_logger;
}

TEST(positive_tests, test1)
{
    //TODO: logger
    logger *logger = create_logger(std::vector<std::pair<std::string, logger::severity>>
                                           {
                                                   {"allocator_sorted_list_tests_logs_negative_test_2.txt",
                                                    logger::severity::debug}
                                           });

    allocator *alloc = new allocator_red_black_tree(3000, nullptr, logger, allocator_with_fit_mode::fit_mode::the_best_fit);


    auto first_block = reinterpret_cast<int *>(alloc->allocate(sizeof(int), 250));

//    auto second_block = reinterpret_cast<char *>(alloc->allocate(sizeof(int), 25));

    alloc->deallocate(first_block);

    // first_block = reinterpret_cast<int *>(alloc->allocate(sizeof(int), 245));


//     alloc->deallocate(second_block);
    // alloc->deallocate(first_block);

    // //TODO: Проверка

    // delete alloc;
}

int main(
        int argc,
        char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}