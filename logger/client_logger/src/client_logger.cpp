//#include <not_implemented.h>
#include "../include/client_logger.h"

std::map<std::string, std::pair<std::ofstream*, size_t>> client_logger::_streams_ =
        std::map<std::string, std::pair<std::ofstream*, size_t>>();

client_logger::client_logger(std::map<std::string, std::vector<severity>> const &other)
{
    for (auto item : other) // проходимся по парам файл-северити
    {
        auto global_stream = _streams_.find(item.first); // поиск

        std::ofstream *stream = nullptr; // создаем поток для записи файла

        if (global_stream == _streams_.end()) // если нет файла
        {
            if (!item.first.empty())
            {
                stream = new std::ofstream; // создание новго потока вывода
                stream->open(item.first); // открытие его
            }

            _streams_.insert(std::make_pair(item.first, std::make_pair(stream, 1)));
        }
        else
        {
            stream = global_stream->second.first; // указатель на объект файла
            global_stream->second.second++; //  увеличение количества использований потока вывода
        }

        _streams_.insert(std::make_pair(item.first, std::make_pair(stream, 1)));
    }
}

client_logger::~client_logger() noexcept
{
    for (auto &logger : _streams_loggers_)
    {
        auto& global_logger = _streams_[logger.first];
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
            _streams_.erase(logger.first);
        }
    }
}

logger const *client_logger::log(const std::string &text, logger::severity severity) const noexcept
{
    std::string data_and_time_string = current_datetime_to_string();
    std::string severity_string = severity_to_string(severity);

    for (auto &logger : _streams_loggers_)
    {
        if (logger.second.first == nullptr) // если файл закрыт
        {
            // то в консольку
            std::cout << "[" << data_and_time_string << "][" << severity_string << "]" << text << std::endl;
        }
        else
        {
            // то в файлик
            *(logger.second.first) << "[" << data_and_time_string << "][" << severity_string << "]" << text << std::endl;
        }
    }

    return this;
}