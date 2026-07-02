#pragma once

#include <spdlog/spdlog.h>

#define FireLog(...) spdlog::info(__VA_ARGS__)
