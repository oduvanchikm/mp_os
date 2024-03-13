//#include <not_implemented.h>

#include "../include/client_logger_builder.h"

logger_builder *client_logger_builder::add_file_stream(std::string const &stream_file_path, logger::severity severity)
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

logger_builder *client_logger_builder::add_console_stream(logger::severity severity)
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

logger_builder* client_logger_builder::transform_with_configuration(
        std::string const &configuration_file_path,
        std::string const &configuration_path)
{
//    std::cout << configuration_file_path << std::endl;

    std::ifstream file(configuration_file_path);

//    std::cout << configuration_file_path << std::endl;

    auto info_json = nlohmann::json::parse(file);

//    std::cout << configuration_file_path << std::endl;

    auto pairs_json = info_json[configuration_path];

//    std::cout << pairs << std::endl;

    for(auto& item : pairs_json)
    {
        _streams_in_builder[item.value("file", "error")].insert({item["severity"]});
    }

    return this;
}

logger_builder *client_logger_builder::clear()
{
    _streams_in_builder.clear();

    return this;
}

logger *client_logger_builder::build() const
{
    return new client_logger(_streams_in_builder);
}

client_logger_builder::client_logger_builder()
{

}

client_logger_builder::~client_logger_builder() noexcept
{

}