#include <SDL3/SDL.h>
#include <gdal.h>
#include <gdal_utils.h>

#include <filesystem>
#include <format>
#include <string>
#include <vector>

#include "service.hpp"

static const std::filesystem::path kBasePath = SDL_GetBasePath();

void Service::Download(ServiceSampleType types, const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, double resolution)
{
    GDALAllRegister();
    std::filesystem::path filePath = kBasePath / std::format("{}_{}.{}_{}.{}.tif",
        GetName(),
        minLatLong.x,
        minLatLong.y,
        maxLatLong.x,
        maxLatLong.y);
    if (!std::filesystem::exists(filePath))
    {
        std::vector<std::string> sources = GetSourceURLs(minLatLong, maxLatLong);
        std::vector<const char*> sourceNames;
        sourceNames.reserve(sources.size());
        for (const std::string& source : sources)
        {
            sourceNames.push_back(source.c_str());
        }
        GDALDatasetH vrt = GDALBuildVRT(
            "/vsimem/service_download.vrt",
            sourceNames.size(),
            nullptr,
            sourceNames.data(),
            nullptr,
            nullptr);
        std::string minX = std::format("{}", minLatLong.y);
        std::string minY = std::format("{}", maxLatLong.x);
        std::string maxX = std::format("{}", maxLatLong.y);
        std::string maxY = std::format("{}", minLatLong.x);
        char* args[] =
        {
            "-projwin", minX.c_str(), minY.c_str(), maxX.c_str(), maxY.c_str(),
            "-projwin_srs", "EPSG:4326",
            nullptr
        };
        GDALTranslateOptions* options = GDALTranslateOptionsNew(args, nullptr);
        GDALDatasetH dataset = GDALTranslate(filePath.string().c_str(), vrt, options, nullptr);
        GDALClose(dataset);
        GDALClose(vrt);
        GDALTranslateOptionsFree(options);
        VSIUnlink("/vsimem/service_download.vrt");
    }
}

virtual void Service::SetSample(const glm::dvec2& latLong, ServiceSample& sample, ServiceSampleType type)
{

}
