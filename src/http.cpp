#include <cpl_http.h>
#include <spdlog/spdlog.h>

#include <optional>
#include <string>

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
    std::string data(reinterpret_cast<const char*>(result->pabyData), result->nDataLen);
    CPLHTTPDestroyResult(result);
    return data;
}
