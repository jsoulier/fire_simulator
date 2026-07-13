#include <SDL3/SDL.h>
#include <cpl_http.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

#include "http.hpp"

std::optional<std::string> HttpGet(const std::string& url)
{
    CPLHTTPResult* result = CPLHTTPFetch(url.c_str(), nullptr);
    if (!result || result->nStatus != 0 || result->pszErrBuf || result->nDataLen <= 0)
    {
        spdlog::error("Failed to fetch {}: {}", url, result && result->pszErrBuf ? result->pszErrBuf : "nullptr");
        CPLHTTPDestroyResult(result);
        return std::nullopt;
    }
    else
    {
        std::string data(reinterpret_cast<const char*>(result->pabyData), result->nDataLen);
        CPLHTTPDestroyResult(result);
        return data;
    }
}

std::optional<std::string> HttpGetAndCache(const std::string& url)
{
    static constexpr std::string_view kBadChars ="<>:\"/\\|?*";
    std::string name = url;
    for (char& character : name)
    {
        unsigned char value = character;
        if (value < 32 || value > 126 || kBadChars.contains(character))
        {
            character = '_';
        }
    }
    std::filesystem::path basePath = SDL_GetBasePath();
    std::filesystem::path filePath = basePath / name;
    if (std::filesystem::exists(filePath))
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file)
        {
            spdlog::error("Failed to open cache {}", filePath.string());
            return std::nullopt;
        }
        std::string response(std::istreambuf_iterator<char>(file), {});
        if ((!file.good() && !file.eof()) || response.empty())
        {
            spdlog::error("Failed to read cache {}", filePath.string());
            return std::nullopt;
        }
        return response;
    }
    std::optional<std::string> response = HttpGet(url);
    if (response)
    {
        std::ofstream file(filePath, std::ios::binary);
        if (!file)
        {
            spdlog::error("Failed to open cache {}", filePath.string());
            return std::nullopt;
        }
        file.write(response->data(), static_cast<std::streamsize>(response->size()));
        if (!file)
        {
            spdlog::error("Failed to write cache {}", filePath.string());
            return std::nullopt;
        }
    }
    return response;
}
