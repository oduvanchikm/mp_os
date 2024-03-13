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
    if (_builder_streams.find(stream_file_path) != _builder_streams.end())
    {
        _builder_streams[stream_file_path].insert(severity);
    }
    else
    {
        _builder_streams.insert({stream_file_path, {severity}});
    }

    return this;
}

logger_builder *client_logger_builder::add_console_stream(logger::severity severity)
{
    if (_builder_streams.find("console") != _builder_streams.end())
    {
        _builder_streams["console"].insert(severity);
    }
    else
    {
        _builder_streams.insert({"console", {severity}});
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

    auto info = nlohmann::json::parse(file);

//    std::cout << configuration_file_path << std::endl;

    auto pairs = info[configuration_path];

//    std::cout << pairs << std::endl;

    for(auto& elem : pairs)
    {
        _builder_streams[elem.value("file", "Not found!")].insert({elem["severity"]});
    }

    return this;
}

logger_builder *client_logger_builder::clear()
{
    _builder_streams.clear();

    return this;
}

logger *client_logger_builder::build() const
{
    return new client_logger(_builder_streams);
}