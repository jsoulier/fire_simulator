#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "auth.hpp"

std::string AuthGetKey(const std::string& fileName)
{
    const char* home = SDL_GetUserFolder(SDL_FOLDER_HOME);
    if (!home)
    {
        spdlog::error("Failed to find home directory for key {}: {}", fileName, SDL_GetError());
        return {};
    }
    std::filesystem::path path = std::filesystem::path(home) / fileName;
    std::ifstream file(path);
    if (!file)
    {
        spdlog::error("Failed to open key file {}", path.string());
        return {};
    }
    std::string key((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    while (!key.empty() && std::isspace(key.back()))
    {
        key.pop_back();
    }
    return key;
}
