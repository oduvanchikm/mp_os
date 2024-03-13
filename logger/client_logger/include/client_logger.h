#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H

#include <logger.h>
#include <map>
#include <set>
#include <iostream>
#include <fstream>


class client_logger final : public logger
{

private:

    std::map<std::string, std::pair<std::ofstream*, std::set<logger::severity>>> _streams;

private:

    static std::map<std::string, std::pair<std::ofstream*, size_t>> _global_streams;


public:

    client_logger(std::map<std::string, std::set<logger::severity>> const &stream);

    client_logger &operator=(client_logger const &other) = delete;

    client_logger(client_logger &&other) noexcept = delete;

    client_logger &operator=(client_logger &&other) noexcept = delete;

    ~client_logger() noexcept final;

public:

    [[nodiscard]] logger const *log(const std::string &message, logger::severity severity) const noexcept override;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H