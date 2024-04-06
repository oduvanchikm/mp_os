#include "../include/server_logger_builder.h"
#include "../include/server_logger.h"

logger_builder* server_logger_builder::format_of_string(std::string const &format_log_string)
{
    return this;
}

server_logger_builder::server_logger_builder()
{

}

server_logger_builder::~server_logger_builder() noexcept
{

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
    }
    return this;
}

logger_builder* server_logger_builder::transform_with_configuration(
    std::string const &configuration_file_path,
    std::string const &configuration_path)
{
//    std::ifstream file(configuration_file_path);
//
//    if (!(file.is_open()))
//    {
//        throw std::runtime_error("can't open file");
//    }
//
//    if (file.peek() == EOF)
//    {
//        throw std::runtime_error("file is empty!!!");
//    }
//
//    auto info_json = nlohmann::json::parse(file);
//
//    auto pairs_json = info_json[configuration_path];
//
//    for(auto& item : pairs_json)
//    {
//        _streams_in_builder[item.value("file", "error")].insert({item["severity"]});
//    }

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