#include <gtest/gtest.h>
#include <client_logger.h>
#include <client_logger_builder.h>



//int main(
//    int argc,
//    char *argv[])
//{
//    testing::InitGoogleTest(&argc, argv);
//
//    return RUN_ALL_TESTS();
//}

int main()
{
    logger_builder* builder = new client_logger_builder();

    logger* logger_1 = builder
            ->add_file_stream("file_1.txt", logger::severity::trace)
            ->add_console_stream(logger::severity::debug)
            ->transform_with_configuration("json_logger.json", "logger")
            ->build();

    logger_1->debug("debug");
    logger_1->trace("trace");

    delete builder;
    delete logger_1;

    return 0;
}