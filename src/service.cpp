#include <SDL3/SDL.h>
#include <gdal.h>
#include <gdal_utils.h>
#include <spdlog/spdlog.h>

#include <cassert>
#include <filesystem>
#include <format>
#include <string>
#include <utility>
#include <vector>

#include "service.hpp"

static const std::filesystem::path kBasePath = SDL_GetBasePath();

static bool IsCategory(ServiceSampleType type)
{
    return type == ServiceSampleType::FuelModel;
}

Service::Raster::Raster()
    : Width(0)
    , Height(0)
    , GeoTransform{}
    , InverseGeoTransform{}
{
}

void Service::Download(ServiceSampleType types, const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, double resolution)
{
    SDL_assert((int(types) & ~int(GetSupportedTypes())) == 0);
    GDALAllRegister();
    ////////////////////////////////////////////////////////////////////////////
    // Download and cache the GeoTIFF. Use a VRT to assemble multiple tiles and clip them to the desired region
    std::filesystem::path filePath = kBasePath / std::format("{}_{}.{}_{}.{}.tif",
        GetName(),
        minLatLong.x,
        minLatLong.y,
        maxLatLong.x,
        maxLatLong.y);
    if (!std::filesystem::exists(filePath))
    {
        std::vector<std::string> sources = GetSourceURLs(minLatLong, maxLatLong);
        if (sources.empty())
        {
            spdlog::error("Failed to get source URLs for {}", GetName());
            return;
        }
        std::vector<const char*> sourceNames;
        sourceNames.reserve(sources.size());
        for (const std::string& source : sources)
        {
            sourceNames.push_back(source.c_str());
        }
        const char* vrtPath = "/vsimem/service_download.vrt";
        GDALDatasetH vrt = GDALBuildVRT(
            vrtPath,
            sourceNames.size(),
            nullptr,
            sourceNames.data(),
            nullptr,
            nullptr);
        if (!vrt)
        {
            spdlog::error("Failed to build VRT for {}: {}", GetName(), CPLGetLastErrorMsg());
            return;
        }
        const std::string minX = std::format("{}", minLatLong.y);
        const std::string minY = std::format("{}", maxLatLong.x);
        const std::string maxX = std::format("{}", maxLatLong.y);
        const std::string maxY = std::format("{}", minLatLong.x);
        const char* args[] =
        {
            "-projwin", minX.c_str(), minY.c_str(), maxX.c_str(), maxY.c_str(),
            "-projwin_srs", "EPSG:4326",
            nullptr
        };
        GDALTranslateOptions* options = GDALTranslateOptionsNew(const_cast<char**>(args), nullptr);
        GDALDatasetH dataset = GDALTranslate(filePath.string().c_str(), vrt, options, nullptr);
        if (!dataset)
        {
            spdlog::error("Failed to translate to {} for {}: {}", filePath.string(), GetName(), CPLGetLastErrorMsg());
        }
        else
        {
            GDALClose(dataset);
        }
        GDALTranslateOptionsFree(options);
        GDALClose(vrt);
        VSIUnlink(vrtPath);
    }
    ////////////////////////////////////////////////////////////////////////////
    // Downsample the high resolution GeoTIFF into a low resolution tile per sample type. Extract into a vector per band
    GDALDatasetH highResolution = GDALOpen(filePath.string().c_str(), GA_ReadOnly);
    if (!highResolution)
    {
        spdlog::error("Failed to open {}: {}", filePath.string(), CPLGetLastErrorMsg());
        return;
    }
    const std::string resolutionString = std::format("{}", resolution);
    for (int i = 0; i < 32; i++)
    {
        const int typeBit = 1 << i;
        if ((int(types) & typeBit) == 0)
        {
            continue;
        }
        const ServiceSampleType type = ServiceSampleType(typeBit);
        std::filesystem::path lowResolutionFilePath = kBasePath / std::format("{}_{}.{}_{}.{}_{}_{}.tif",
            GetName(),
            minLatLong.x,
            minLatLong.y,
            maxLatLong.x,
            maxLatLong.y,
            resolution,
            int(type));
        if (!std::filesystem::exists(lowResolutionFilePath))
        {
            const std::string bandString = std::format("{}", GetBand(type));
            const std::string algorithmString = IsCategory(type) ? "mode" : "average";
            const char* args[] =
            {
                "-b", bandString.c_str(),
                "-tr", resolutionString.c_str(), resolutionString.c_str(),
                "-r", algorithmString.c_str(),
                nullptr
            };
            GDALTranslateOptions* options = GDALTranslateOptionsNew(const_cast<char**>(args), nullptr);
            GDALDatasetH lowResolution = GDALTranslate(lowResolutionFilePath.string().c_str(), highResolution, options, nullptr);
            if (!lowResolution)
            {
                spdlog::error("Failed to downsample band {} to {}: {}", bandString, lowResolutionFilePath.string(), CPLGetLastErrorMsg());
            }
            else
            {
                GDALClose(lowResolution);
            }
            GDALTranslateOptionsFree(options);
        }
        GDALDatasetH lowResolution = GDALOpen(lowResolutionFilePath.string().c_str(), GA_ReadOnly);
        if (!lowResolution)
        {
            spdlog::error("Failed to open {}: {}", lowResolutionFilePath.string(), CPLGetLastErrorMsg());
            continue;
        }
        Raster raster;
        raster.Width = GDALGetRasterXSize(lowResolution);
        raster.Height = GDALGetRasterYSize(lowResolution);
        if (GDALGetGeoTransform(lowResolution, raster.GeoTransform) != CE_None ||
            GDALInvGeoTransform(raster.GeoTransform, raster.InverseGeoTransform) == FALSE)
        {
            spdlog::error("Failed to get GeoTransform or InvGeoTransform for {}", lowResolutionFilePath.string());
            GDALClose(lowResolution);
            continue;
        }
        raster.Wkt = GDALGetProjectionRef(lowResolution);
        raster.Pixels.resize(size_t(raster.Width) * size_t(raster.Height));
        CPLErr status = GDALRasterIO(
            GDALGetRasterBand(lowResolution, 1), GF_Read,
            0, 0, raster.Width, raster.Height,
            raster.Pixels.data(), raster.Width, raster.Height,
            IsCategory(type) ? GDT_UInt32 : GDT_Float32, 0, 0);
        GDALClose(lowResolution);
        if (status != CE_None)
        {
            spdlog::error("Failed to read {}: {}", lowResolutionFilePath.string(), CPLGetLastErrorMsg());
            continue;
        }
        Rasters[type] = std::move(raster);
    }
    GDALClose(highResolution);
}
