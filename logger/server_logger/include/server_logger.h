#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_SERVER_LOGGER_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_SERVER_LOGGER_H
#include "../../logger/include/logger.h"
#include "server_logger_builder.h"
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <sstream>
#include <mqueue.h>
#include <cstring>
#include <unistd.h>

#define _LINUX_
//#define _WINDOWS_

class server_logger final : public logger
{

private:

#ifdef _LINUX_

    std::map<std::string, std::pair<mqd_t, std::set<logger::severity>>> _queues_streams;

    static std::map<std::string, std::pair<mqd_t, int>> _queue;

#elif _WINDOWS_

    std::map<std::string, std::pair<...., std::set<logger::severity>>> _queues_streams; // for windows

    static std::map<std::string, std::pair<...., int>> _queue;

#endif

public:

#ifdef _LINUX_

    server_logger(std::map<std::string, std::pair<mqd_t, int>> const &builder);

#elif _WIN

    server_logger(std::map<std::string, std::pair<mqd_t, int>> const &builder); // change for windows

#endif

    server_logger(std::map<std::string, std::set<severity>> const &other);

    server_logger(server_logger const &other);

    server_logger &operator=(server_logger const &other);

    server_logger(server_logger &&other) noexcept;

    server_logger &operator=(server_logger &&other) noexcept;

    ~server_logger() noexcept final;

public:

    [[nodiscard]] logger const *log(const std::string &message, logger::severity severity) const noexcept override;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_SERVER_LOGGER_H