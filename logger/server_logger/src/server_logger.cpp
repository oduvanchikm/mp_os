#include "../include/server_logger.h"
#define SIZE 10
#include <mqueue.h>
#include <string>
#include <ostream>
#include <iostream>

#ifdef _LINUX_

std::map<std::string, std::pair<mqd_t, int>> server_logger::_queue =
        std::map<std::string, std::pair<mqd_t, int>>();

#elif _WINDOWS_

std::map<std::string, std::pair<mqd_t, int>> server_logger::_queue =
        std::map<std::string, std::pair<mqd_t, int>>(); // change for windows

#endif


server_logger::server_logger(server_logger const &other) :
        _queues_streams(other._queues_streams)
{
    for (auto &map : _queue)
    {
        map.second.second++;
    }
}

server_logger &server_logger::operator=(server_logger const &other)
{
    if (this != &other)
    {
        _queues_streams = other._queues_streams;

        for (auto &map : _queues_streams)
        {
            _queue[map.first].second++;
        }
    }
    return *this;
}

server_logger::server_logger(server_logger &&other) noexcept :
        _queues_streams(other._queues_streams)
{

}

server_logger &server_logger::operator=(server_logger &&other) noexcept
{
    if (this != &other)
    {
        _queues_streams = std::move(other._queues_streams);
    }
    return *this;
}

#ifdef _LINUX_

server_logger::server_logger(std::map<std::string, std::set<severity>> const &other)
{
    mqd_t descriptor;
    struct mq_attr attrib_queue;

    attrib_queue.mq_flags = 0;
    attrib_queue.mq_maxmsg = 10;
    attrib_queue.mq_msgsize = 1024;
    attrib_queue.mq_curmsgs = 0;

    for (auto i = other.begin(); i != other.end(); ++i)
    {
        auto queue_stream = _queue.find(i->first);

        if (queue_stream == _queue.end())
        {
            descriptor = mq_open(i->first.c_str(), O_CREAT | O_WRONLY, 0644, &attrib_queue);
            if (descriptor == (mqd_t) - 1)
            {
                std::cerr << "Error creating message queue. Errno: " << errno << std::endl;
                throw std::runtime_error("It is impossible to create a queue");

            }

            _queue.insert(std::make_pair(i->first, std::make_pair(descriptor, 1)));
        }
        else
        {
            descriptor = queue_stream->second.first;
            queue_stream->second.second++;
        }

        auto queue_in_streams = _queues_streams.find(i->first);

        if (queue_in_streams == _queues_streams.end())
        {
            _queues_streams.insert(std::make_pair(i->first, std::make_pair(descriptor, i->second)));
        }
    }
}

server_logger::~server_logger() noexcept
{
    for (auto &q : _queues_streams)
    {
        auto global_logger = _queue[q.first];
        global_logger.second--;

        if(!global_logger.second)
        {
            mq_close(q.second.first);
        }
        _queue.erase(q.first);
    }
}

logger const *server_logger::log(const std::string &message, logger::severity severity) const noexcept
{
    int id = 0;

    for (auto &queue : _queues_streams)
    {
        if (queue.second.second.find(severity) != queue.second.second.end())
        {
            int message_size = message.size();
//            int count = (message_size + SIZE - 1) / SIZE;
            int count = message.size() / message_size + 1;


            for (int i = 0; i < count; ++i)
            {
                auto string_date_time = current_datetime_to_string();
                auto string_severity = severity_to_string(severity);

                std::string connected_string = "[" + string_date_time + "][" + string_severity + "]" + message;

                const char* connected_string_char = connected_string.c_str();

                try
                {
                    if ((mq_send(queue.second.first, connected_string_char, (sizeof(connected_string_char) + 1),
                                 0)) == -1)
                    {
                        perror("mq_send");
                        throw std::runtime_error("It is impossible to send message");
                    }
                }
                catch (const std::runtime_error &e)
                {
                    std::cerr << "Error: " << e.what() << std::endl;
                }

            }
            id++;
        }
    }
    return this;
}

#elif _WIN

#endif