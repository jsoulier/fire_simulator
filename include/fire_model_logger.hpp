#pragma once

#include <cadmium/core/logger/spdlog.hpp>

class FireModelLogger : public cadmium::SpdlogLogger
{
public:
    using cadmium::SpdlogLogger::SpdlogLogger;
    void start() override;
};
