//#include <not_implemented.h>

#include "../include/server_logger_builder.h"
#include "../include/server_logger.h"

server_logger_builder::server_logger_builder()
{
//    throw not_implemented("server_logger_builder::server_logger_builder()", "your code should be here...");
}

server_logger_builder::~server_logger_builder() noexcept
{
//    throw not_implemented("server_logger_builder::~server_logger_builder() noexcept", "your code should be here...");
}

logger_builder *server_logger_builder::add_file_stream(
    std::string const &stream_file_path,
    logger::severity severity)
{
    if (_streams_in_builder.find(stream_file_path) != _streams_in_builder.end())
    {
        _streams_in_builder[stream_file_path].insert(severity);
    }
    else
    {
        _streams_in_builder.insert({stream_file_path, {severity}});
    }

    return this;
}

logger_builder *server_logger_builder::add_console_stream(
    logger::severity severity)
{
    if (_streams_in_builder.find("console") != _streams_in_builder.end())
    {
        _streams_in_builder["console"].insert(severity);
    }
    else
    {
        _streams_in_builder.insert({"console", {severity}});
    }}

logger_builder* server_logger_builder::transform_with_configuration(
    std::string const &configuration_file_path,
    std::string const &configuration_path)
{
    return this;
}

logger_builder *server_logger_builder::clear()
{
    _streams_in_builder.clear();

    return this;
}

logger *server_logger_builder::build() const
{
    return new server_logger(_streams_in_builder);
}