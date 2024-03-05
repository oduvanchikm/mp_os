//#include <not_implemented.h>

#include "../include/client_logger_builder.h"

client_logger_builder::client_logger_builder()
{

}

client_logger_builder::~client_logger_builder() noexcept
{

}

logger_builder *client_logger_builder::add_file_stream(std::string const &stream_file_path, logger::severity severity)
{
    data[stream_file_path].push_back(severity);

    return this;
}

logger_builder *client_logger_builder::add_console_stream(logger::severity severity)
{
    data["console"].push_back(severity);

    return this;
}

logger_builder* client_logger_builder::transform_with_configuration(std::string const &configuration_file_path,
                                                                    std::string const &configuration_path)
{
    //throw not_implemented("logger_builder* client_logger_builder::transform_with_configuration(std::string const &configuration_file_path, std::string const &configuration_path)", "your code should be here...");
}

logger_builder *client_logger_builder::clear()
{
//    data.clear();
//
//    return this;
}

logger *client_logger_builder::build() const
{
    return new client_logger(data);
}