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

struct queue_message
{
    std::string data_and_time;
    char text[100];
    char pid_queue[20];
    logger::severity severity_for_logger;
    int id;
    int number_of_packets;
    int count_of_packets;
};

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
    attrib_queue.mq_msgsize = sizeof(queue_message) * 2;
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
    auto string_date_time = current_datetime_to_string();

    int id = 0;

    for (auto &queue : _queues_streams)
    {
        if (queue.second.second.find(severity) != queue.second.second.end())
        {
            int message_size = std::strlen(message.c_str());
            int count = (message_size + SIZE - 1) / SIZE;

            std::cout << "count of packets: " << count << std::endl;
            std::cout << "message size: " << message_size << std::endl;

            for (int i = 0; i < count; ++i)
            {
                queue_message message_queue;

                char packet[SIZE + 1];

                int start_pos = i * SIZE;

                int end_pos = std::min(start_pos + SIZE, message_size);

                memcpy(packet, &message[start_pos], end_pos - start_pos);

                packet[end_pos - start_pos] = '\0';

                strcpy(message_queue.text, packet);

                message_queue.severity_for_logger = severity;
                message_queue.data_and_time = string_date_time;

                pid_t pid = getpid();
                std::string get_pid_string = std::to_string(pid);
                strncpy(message_queue.pid_queue, get_pid_string.c_str(), 20);

                message_queue.id = id;
                message_queue.number_of_packets = i + 1;
                message_queue.count_of_packets = count;

                try {
                    if ((mq_send(queue.second.first, reinterpret_cast<char *>(&message_queue), sizeof(queue_message),
                                 0)) == -1) {
                        perror("mq_send");
                        throw std::runtime_error("It is impossible to send message");
                    }
                }
                catch (const std::runtime_error &e) {
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

