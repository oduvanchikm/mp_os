#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H

#include <logger.h>
//#include "../../logger/include/logger.h"
#include <map>
#include <unordered_map>
#include <set>
#include <iostream>
#include <fstream>
#include <cstdarg>
#include <sstream>
#include <mqueue.h>
#include <cstring>
#define CLIENT_LOGGER
//#define SENDING_TO_SERVER_UNIX
#define SENDING_TO_SERVER_WINDOWS


class client_logger final:
        public logger
{

private:

    std::string _format_log_string;

private:

    std::map<std::string, std::pair<std::ofstream*, std::set<logger::severity>>> _all_streams;

private:

    static std::map<std::string, std::pair<std::ofstream*, size_t>> _global_streams;

public:

    client_logger(std::map<std::string, std::set<logger::severity>> const &stream, std::string const &format_log_string);

    client_logger &operator=(
            client_logger const &other) = delete;

    client_logger(
            client_logger &&other) noexcept = delete;

    client_logger &operator=(
            client_logger &&other) noexcept = delete;

    ~client_logger() noexcept final;

public:

    [[nodiscard]] logger const *log(
            const std::string &message, logger::severity severity) const noexcept override;

public:

    struct message
    {
        int count_of_messages;
        int size_of_message;
        std::string text;
        std::string file_path;
        logger::severity severity;
    };

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H