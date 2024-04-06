#include <gtest/gtest.h>
#include <server_logger.h>
#include <server_logger_builder.h>



int main(
    int argc,
    char *argv[])
{
//    testing::InitGoogleTest(&argc, argv);
    logger_builder* builder = new server_logger_builder();

    logger* logger_1 = builder
            ->add_file_stream("file_7.txt", logger::severity::warning)
            ->build();

//    return RUN_ALL_TESTS();
    return 0;
}
//
//int main()
//{
//    logger_builder* builder = new client_logger_builder();
//
//    logger* logger_1 = builder
//            ->add_file_stream("file_1.txt", logger::severity::trace)
//            ->add_console_stream(logger::severity::debug)
//            ->build();
//
//    logger_1->log("ggggggggggg", logger::severity::debug);
//
//    delete builder;
//    delete logger_1;
//
//    return 0;
//}