#include "../include/client_logger_builder.h"

logger_builder* client_logger_builder::format_of_string(std::string const &format_log_string)
{
    _format_log_string = format_log_string;
    return this;
}

logger_builder *client_logger_builder::add_file_stream(std::string const &stream_file_path, logger::severity severity)
{

    if (stream_file_path.empty())
    {
        _streams_in_builder.insert({stream_file_path, {severity}});
    }

    _streams_in_builder[stream_file_path].insert(severity);

    return this;
}

logger_builder *client_logger_builder::add_console_stream(logger::severity severity)
{
    add_file_stream("console", severity);

    return this;
}

logger_builder* client_logger_builder::transform_with_configuration(
        std::string const &configuration_file_path,
        std::string const &configuration_path)
{
    std::ifstream file(configuration_file_path);

    if (!(file.is_open()))
    {
        throw std::runtime_error("can't open file\n");
    }

    if (file.peek() == EOF)
    {
        throw std::runtime_error("file is empty!!!\n");
    }

    auto info_json = nlohmann::json::parse(file);

    auto pairs_json = info_json[configuration_path];
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
    return new client_logger(_streams_in_builder, _format_log_string);
}

client_logger_builder::client_logger_builder()
{
    _format_log_string = "%d %t %s %m";
}

client_logger_builder::~client_logger_builder() noexcept
{

}