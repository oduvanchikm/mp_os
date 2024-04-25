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



class client_logger final:
        public logger
{

    friend class client_logger_builder;

private:

    std::string _format_log_string;

private:

    std::map<std::string, std::pair<std::ofstream*, std::set<logger::severity>>> _all_streams;

private:

    static std::map<std::string, std::pair<std::ofstream*, size_t>> _global_streams;

    client_logger(std::map<std::string, std::set<logger::severity>> const &stream, std::string const &format_log_string);

public:

    client_logger(client_logger const &other);

    client_logger &operator=(client_logger const &other);

    client_logger(client_logger &&other) noexcept;

    client_logger &operator=(client_logger &&other) noexcept;

    ~client_logger() noexcept final;

public:

    [[nodiscard]] logger const *log(
            const std::string &message, logger::severity severity) const noexcept override;
};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H