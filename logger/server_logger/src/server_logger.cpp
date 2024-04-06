//#include <not_implemented.h>
#include "../include/server_logger.h"

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
    attrib_queue.mq_msgsize = 10;
    attrib_queue.mq_curmsgs = 0;

    for (auto i = other.begin(); i != other.end(); ++i)
    {
        auto queue_stream = _queue.find(i->first);

        if (queue_stream == _queue.end())
        {
            descriptor = mq_open(i->first.c_str(), O_CREAT | O_WRONLY, 0644, &attrib_queue);
            if (descriptor == (mqd_t) -1)
            {
                throw std::runtime_error("It is impossible to create a queue\n");
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
            mq_close(_queue[q.first].first);
        }
        _queue.erase(q.first);
    }
}

logger const *server_logger::log(const std::string &message, logger::severity severity) const noexcept
{

}


#elif _WIN

#endif

