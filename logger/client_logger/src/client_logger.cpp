#include "../include/client_logger.h"

std::map<std::string, std::pair<std::ofstream*, size_t>> client_logger::_global_streams =
        std::map<std::string, std::pair<std::ofstream*, size_t>>();

client_logger::client_logger(std::map<std::string, std::set<severity>> const &builder, std::string const &format_log_string)
{
    _format_log_string = format_log_string;
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

#if defined(CLIENT_LOGGER)

logger const *client_logger::log(const std::string &text, logger::severity severity) const noexcept
{
    auto string_severity = severity_to_string(severity);
    auto string_date_time = current_datetime_to_string();

    std::string date_string, time_string;
    std::stringstream ss(string_date_time);
    std::getline(ss, date_string, ' ');
    std::getline(ss, time_string);

    for (auto &stream : _all_streams)
    {
        if (stream.second.second.find(severity) != stream.second.second.end())
        {
            if (stream.second.first == nullptr)
            {
                for (int i = 0; i < _format_log_string.length() - 1; i++)
                {
                    if (_format_log_string[i] == '%')
                    {
                        if (_format_log_string[i + 1] == 'd')
                        {
                            std::cout << "[" << date_string << "]" << std::endl;
                        }
                        else if (_format_log_string[i + 1] == 't')
                        {
                            std::cout << "[" << time_string << "]" << std::endl;
                        }
                        else if (_format_log_string[i + 1] == 's')
                        {
                            std::cout << "[" << string_severity << "]" << std::endl;
                        }
                        else if (_format_log_string[i + 1] == 'm')
                        {
                            std::cout << text << std::endl;
                        }
                    }
                }
            }
            else
            {
                for (int i = 0; i < _format_log_string.length() - 1; i++)
                {
                    if (_format_log_string[i] == '%')
                    {
                        if (_format_log_string[i + 1] == 'd')
                        {
                            *(stream.second.first) << "[" << date_string << "]";
                        }
                        else if (_format_log_string[i + 1] == 't')
                        {
                            *(stream.second.first) << "[" << time_string << "]";
                        }
                        else if (_format_log_string[i + 1] == 's')
                        {
                            *(stream.second.first) << "[" << string_severity << "]";
                        }
                        else if (_format_log_string[i + 1] == 'm')
                        {
                            *(stream.second.first) << text;
                        }
                    }
                }
            }
        }
    }
    return this;
}

#elif defined(SENDING_TO_SERVER_UNIX)

logger const *client_logger::log(const std::string &text, logger::severity severity) const noexcept
{
    mqd_t mq;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 10;
    attr.mq_curmsgs = 0;

    // Создаем очередь с именем "/my_queue". O_CREAT: очередь должна быть создана
    // O_WRONLY: очередь будет открыта только для записи

    mq = mq_open("/my_queue", O_CREAT | O_WRONLY, 0644, &attr);
    if (mq == -1)
    {
        perror("mq_open");
        exit(1);
    }

    int count = 0; // подсчета количества сообщений
    std::vector<message> messages; // для хранения всех сообщений

    for (auto &stream : _all_streams)
    {
        if (stream.second.second.find(severity) != stream.second.second.end())
        {
            message msg;
            int size = text.size();

            if (stream.second.first == nullptr) // если в консоль
            {
                msg.file_path = "console";
            }
            else
            {
                msg.file_path = stream.first;
            }

            msg.severity = severity;
            msg.text = text;
            msg.size_of_message = size;

            messages.push_back(msg); // Добавляем сообщение в вектор
            count++; // Увеличиваем счетчик сообщений


//            std::cout << "Sending message:" << std::endl;
//            std::cout << "File path: " << msg.file_path << std::endl;
//            std::cout << "Severity: " << static_cast<int>(msg.severity) << std::endl;
//            std::cout << "Text: " << msg.text << std::endl;
//            std::cout << "Size of message: " << msg.size_of_message << std::endl;
//            std::cout << "---------------------" << std::endl;
        }
    }

    // отправляем все сообщения из вектора в очередь одним вызовом
    if (mq_send(mq, reinterpret_cast<const char*>(messages.data()), messages.size() * sizeof(message), 0) == -1)
    {
        perror("mq_send");
        exit(1);
    }

//    std::cout << "message go to the window" << std::endl;

    mq_close(mq);
    return this;
}





#else

#endif


