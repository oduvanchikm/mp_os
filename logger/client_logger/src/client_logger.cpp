#include "../include/client_logger.h"

std::map<std::string, std::pair<std::ofstream*, size_t>> client_logger::_global_streams =
        std::map<std::string, std::pair<std::ofstream*, size_t>>();

client_logger::client_logger(std::map<std::string, std::set<severity>> const &builder)
{
    for (auto &builder_stream : builder)
    {
        auto global_stream = _global_streams.find(builder_stream.first);

        std::ofstream* stream = nullptr;

        if (global_stream == _global_streams.end())
        {
            if (global_stream->first != "console")
            {
                stream = new std::ofstream;
                stream->open(builder_stream.first);
            }

            _global_streams.insert(std::make_pair(builder_stream.first, std::make_pair(stream, 1)));
        }
        else
        {
            stream = global_stream->second.first;
            global_stream->second.second++;
        }

        _all_streams.insert(std::make_pair(builder_stream.first, std::make_pair(stream, builder_stream.second)));
    }
}

client_logger::~client_logger() noexcept
{
    for (auto &stream : _all_streams)
    {
        auto global_logger = _global_streams[stream.first];

        global_logger.second--;

        if(!global_logger.second)
        {
            if (global_logger.first != nullptr)
            {
                if (global_logger.first->is_open())
                {
                    global_logger.first->close();
                }
                delete global_logger.first;
            }
            _global_streams.erase(stream.first);
        }
    }
}

logger const *client_logger::log(const std::string &text, logger::severity severity) const noexcept
{
    auto string_severity = severity_to_string(severity);
    auto string_time = current_datetime_to_string();

    for (auto &stream : _all_streams)
    {
        if (stream.second.second.find(severity) != stream.second.second.end())
        {
            if (stream.second.first == nullptr)
            {
                std::cout << "[" << string_time << "][" << string_severity << "]" << text << std::endl;
            }
            else
            {
                *(stream.second.first) << "[" << string_time << "][" << string_severity << "]" << text << std::endl;
            }
        }
    }
    return this;
}