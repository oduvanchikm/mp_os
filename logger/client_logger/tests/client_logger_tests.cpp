#include <gtest/gtest.h>
#include <client_logger.h>
#include <client_logger_builder.h>
//#include "../../client_logger/include/client_logger.h"
//#include "../../client_logger/include/client_logger_builder.h"

int main(
    int argc,
    char *argv[])
{
    testing::InitGoogleTest(&argc, argv);

    logger_builder* builder = new client_logger_builder();

    logger* logger_1 = builder
            ->add_file_stream("file5.txt", logger::severity::information)
            ->add_file_stream("file7.txt", logger::severity::debug)
            ->add_console_stream(logger::severity::debug)
            ->transform_with_configuration("json_logger.json", "logger")
//            ->add_file_stream("file1.txt", logger::severity::debug)
//            ->add_file_stream("file2.txt", logger::severity::warning)
//            ->add_console_stream(logger::severity::warning)
//            ->add_console_stream(logger::severity::debug)
            ->format_of_string("%t %s %m %d %d %d %d %d message")


            ->build();

//    logger_1->debug("debug");
//    logger_1->trace("trace");
//    logger_1->error("ererererererere");

    logger_1->log("eeriugierq", logger::severity::debug);
//    logger_1->log("eerrgewiugierq", logger::severity::warning);
    logger_1->log("eeriw3r3r24ugierq", logger::severity::information);

    delete builder;
    delete logger_1;

    return RUN_ALL_TESTS();
}