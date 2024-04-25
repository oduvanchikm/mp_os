#include "../include/client_logger.h"

std::map<std::string, std::pair<std::ofstream*, size_t>> client_logger::_global_streams =
        std::map<std::string, std::pair<std::ofstream*, size_t>>();

client_logger::client_logger(std::map<std::string, std::set<severity>> const &builder, std::string const &format_log_string)
{
    _format_log_string = format_log_string;
    std::ofstream* stream = nullptr;

    for (auto i = builder.begin(); i != builder.end(); ++i)
    {
        auto global_stream = _global_streams.find(i->first);

        if (global_stream == _global_streams.end())
        {
            if (global_stream->first != "console")
            {
                stream = new std::ofstream(i->first, std::ios::out);
            }
            else
            {
                stream = nullptr;
            }

            _global_streams.insert(std::make_pair(i->first, std::make_pair(stream, 1)));
        }
        else
        {
            stream = global_stream->second.first;
            global_stream->second.second++;
        }

        auto streams_in_streams = _all_streams.find(i->first);

        if (streams_in_streams == _all_streams.end())
        {
            _all_streams.insert(std::make_pair(i->first, std::make_pair(stream, i->second)));
        }
    }
}

client_logger::client_logger(client_logger const &other) :
    _format_log_string(other._format_log_string), _all_streams(other._all_streams)
{
    for (auto &map : _global_streams)
    {
        map.second.second++;
    }
}

client_logger &client_logger::operator=(client_logger const &other)
{
    if (this != &other)
    {
        _format_log_string = other._format_log_string;
        _all_streams = other._all_streams;

        for (auto &map : _all_streams)
        {
            _global_streams[map.first].second++;
        }

    }
    return *this;
}

client_logger::client_logger(client_logger &&other) noexcept :
    _format_log_string(std::move(other._format_log_string)), _all_streams(std::move(other._all_streams))
{

}

client_logger &client_logger::operator=(client_logger &&other) noexcept
{
    if (this != &other)
    {
        _format_log_string = std::move(other._format_log_string);
        _all_streams = std::move(other._all_streams);
    }
    return *this;
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
    auto string_date_time = current_datetime_to_string();

    std::string date_string, time_string;
    std::stringstream ss(string_date_time);
    std::getline(ss, date_string, ' ');
    std::getline(ss, time_string);

    int len_string = _format_log_string.length();

    for (auto &stream : _all_streams)
    {
        if (stream.second.second.find(severity) != stream.second.second.end())
        {
            for (int i = 0; i < len_string - 1; i++)
            {
                if (_format_log_string[i] == '%' && _format_log_string[i + 1] == 'd')
                {
                    if (stream.second.first == nullptr)
                    {
                        std::cout << "[" << date_string << "]" << std::endl;
                    }
                    else
                    {
                        *(stream.second.first) << "[" << date_string << "]";
                    }
                }
                else if (_format_log_string[i] == '%' && _format_log_string[i + 1] == 't')
                {
                    if (stream.second.first == nullptr)
                    {
                        std::cout << "[" << time_string << "]" << std::endl;
                    }
                    else
                    {
                        *(stream.second.first) << "[" << time_string << "]";
                    }
                }
                else if (_format_log_string[i] == '%' && _format_log_string[i + 1] == 's')
                {
                    if (stream.second.first == nullptr)
                    {
                        std::cout << "[" << string_severity << "]" << std::endl;
                    }
                    else
                    {
                        *(stream.second.first) << "[" << string_severity << "]";
                    }
                }
                else if (_format_log_string[i] == '%' && _format_log_string[i + 1] == 'm')
                {
                    if (stream.second.first == nullptr)
                    {
                        std::cout << "[" << text << "]" << std::endl;
                    }
                    else
                    {
                        *(stream.second.first) << "[" << text << "]";
                    }
                }
            }
        }
    }
    return this;
}



