#include <SDL3/SDL.h>
#include <cpl_conv.h>
#include <gdal.h>
#include <gdal_utils.h>
#include <gdalwarper.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <ogr_srs_api.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "service.hpp"
#include "timer.hpp"

uint32_t ServiceSampleTypeToIndex(ServiceSampleType type)
{
    return std::countr_zero(uint32_t(type));
}

ServiceSampleType ServiceSampleTypeFromIndex(uint32_t index)
{
    return ServiceSampleType(1 << index);
}

const char* ServiceSampleTypeToString(ServiceSampleType type)
{
    return kServiceSampleTypeStrings[ServiceSampleTypeToIndex(type)];
}

ServicePixelType ServiceSampleTypeToPixelType(ServiceSampleType type)
{
    return type == ServiceSampleType::FuelModel ? ServicePixelType::U32 : ServicePixelType::F32;
}

Service::Raster::Raster()
    : Width(0)
    , Height(0)
    , GeoTransform{}
    , InverseGeoTransform{}
    , Texture{}
{
}

void Service::Download(ServiceSampleType types, const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, double resolution)
{
    TimerBlock(std::format("{} download", GetName()));
    types |= GetRequiredSampleTypes(types);
    SDL_assert((types & ~GetSupportedTypes()) == ServiceSampleType{});
    GDALAllRegister();
    const char* projPath[] = { SDL_GetBasePath(), nullptr };
    OSRSetPROJSearchPaths(projPath);
    ////////////////////////////////////////////////////////////////////////////
    // Download and cache the GeoTIFF. Use a VRT to assemble multiple tiles and clip them to the desired region
    std::filesystem::path basePath = SDL_GetBasePath();
    std::filesystem::path filePath = basePath / std::format("{}_{}.{}_{}.{}.tif",
        GetName(),
        minLatLong.x,
        minLatLong.y,
        maxLatLong.x,
        maxLatLong.y);
    if (!std::filesystem::exists(filePath))
    {
        TimerBlock(std::format("{} vrt", GetName()));
        std::vector<std::string> sources = GetURLs(minLatLong, maxLatLong);
        if (sources.empty())
        {
            return;
        }
        std::vector<const char*> sourceNames;
        sourceNames.reserve(sources.size());
        for (const std::string& source : sources)
        {
            sourceNames.push_back(source.c_str());
        }
        const char* vrtPath = "/vsimem/service.vrt";
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
        ////////////////////////////////////////////////////////////////////////
        // Check if the tile is in EPSG:4326 (WGS 84) (uses lat/long). If not, convert it to EPSG:4326
        bool isWgs84 = false;
        const char* sourceWkt = GDALGetProjectionRef(vrt);
        if (sourceWkt && sourceWkt[0])
        {
            OGRSpatialReferenceH wgs84 = OSRNewSpatialReference(nullptr);
            OSRImportFromEPSG(wgs84, 4326);
            OSRSetAxisMappingStrategy(wgs84, OAMS_TRADITIONAL_GIS_ORDER);
            OGRSpatialReferenceH sourceSrs = OSRNewSpatialReference(sourceWkt);
            isWgs84 = OSRIsSame(sourceSrs, wgs84);
            OSRDestroySpatialReference(sourceSrs);
            OSRDestroySpatialReference(wgs84);
        }
        const std::string west = std::format("{}", minLatLong.y);
        const std::string south = std::format("{}", minLatLong.x);
        const std::string east = std::format("{}", maxLatLong.y);
        const std::string north = std::format("{}", maxLatLong.x);
        if (isWgs84)
        {
            TimerBlock(std::format("{} clip", GetName()));
            const char* args[] =
            {
                "-projwin", west.c_str(), north.c_str(), east.c_str(), south.c_str(),
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
        }
        else
        {
            TimerBlock(std::format("{} clip and reproject", GetName()));
            const char* args[] =
            {
                "-t_srs", "EPSG:4326",
                "-te", west.c_str(), south.c_str(), east.c_str(), north.c_str(),
                "-r", "near",
                nullptr
            };
            GDALWarpAppOptions* options = GDALWarpAppOptionsNew(const_cast<char**>(args), nullptr);
            GDALDatasetH dataset = GDALWarp(filePath.string().c_str(), nullptr, 1, &vrt, options, nullptr);
            if (!dataset)
            {
                spdlog::error("Failed to warp {} to {}: {}", GetName(), filePath.string(), CPLGetLastErrorMsg());
            }
            else
            {
                GDALClose(dataset);
            }
            GDALWarpAppOptionsFree(options);
        }
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
    const std::string lowResolutionBasePath = (basePath / std::format("{}_{}.{}_{}.{}_{}",
        GetName(),
        minLatLong.x,
        minLatLong.y,
        maxLatLong.x,
        maxLatLong.y,
        resolution)).string();
    for (int i = 0; i < 32; i++)
    {
        const ServiceSampleType type = ServiceSampleType(1 << i);
        if ((types & type) == ServiceSampleType{})
        {
            continue;
        }
        std::filesystem::path lowResolutionFilePath = std::format("{}_{}.tif", lowResolutionBasePath, int(type));
        if (!std::filesystem::exists(lowResolutionFilePath))
        {
            TimerBlock(std::format("{} {} downsample", GetName(), ServiceSampleTypeToString(type)));
            const std::string bandString = std::format("{}", GetBand(type));
            const std::string algorithmString = ServiceSampleTypeToPixelType(type) == ServicePixelType::U32 ? "mode" : "average";
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
        {
            TimerBlock(std::format("{} {} derive", GetName(), ServiceSampleTypeToString(type)));
            Derive(type, lowResolution, lowResolutionBasePath);
        }
        Raster raster;
        {
            TimerBlock(std::format("{} {} raster", GetName(), ServiceSampleTypeToString(type)));
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
                ServiceSampleTypeToPixelType(type) == ServicePixelType::U32 ? GDT_UInt32 : GDT_Float32, 0, 0);
            GDALClose(lowResolution);
            if (status != CE_None)
            {
                spdlog::error("Failed to read {}: {}", lowResolutionFilePath.string(), CPLGetLastErrorMsg());
                continue;
            }
        }
        {
            TimerBlock(std::format("{} {} post process", GetName(), ServiceSampleTypeToString(type)));
            PostProcess(type, raster.Pixels);
        }
        Rasters[type] = std::move(raster);
    }
    GDALClose(highResolution);
    ////////////////////////////////////////////////////////////////////////////
    // Create ImTextures for each band
    {
        for (auto& [type, raster] : Rasters)
        {
            TimerBlock(std::format("{} {} texture", GetName(), ServiceSampleTypeToString(type)));
            ImTextureData* texture = IM_NEW(ImTextureData)();
            texture->Create(ImTextureFormat_RGBA32, raster.Width, raster.Height);
            uint32_t* texels = reinterpret_cast<uint32_t*>(texture->GetPixels());
            if (type == ServiceSampleType::FuelModel)
            {
                SDL_assert(ServiceSampleTypeToPixelType(type) == ServicePixelType::U32);
                for (size_t i = 0; i < raster.Pixels.size(); i++)
                {
                    texels[i] = FireFuelModelTypeGetColor(FireFuelModelType(raster.Pixels[i].U32));
                }
            }
            else
            {
                SDL_assert(ServiceSampleTypeToPixelType(type) == ServicePixelType::F32);
                static constexpr float kNoData = -9999.0f;
                float minValue = std::numeric_limits<float>::max();
                float maxValue = std::numeric_limits<float>::lowest();
                for (const ServicePixel& pixel : raster.Pixels)
                {
                    if (pixel.F32 == kNoData)
                    {
                        continue;
                    }
                    minValue = std::min(minValue, pixel.F32);
                    maxValue = std::max(maxValue, pixel.F32);
                }
                const float range = maxValue - minValue;
                for (size_t i = 0; i < raster.Pixels.size(); i++)
                {
                    uint8_t gray = 0;
                    if (raster.Pixels[i].F32 != kNoData && range > 0.0f)
                    {
                        gray = (raster.Pixels[i].F32 - minValue) / range * 255.0f;
                    }
                    texels[i] = IM_COL32(gray, gray, gray, 255);
                }
            }
            ImGui::RegisterUserTexture(texture);
            raster.Texture = texture->GetTexRef();
        }
    }
}

ServicePixel Service::GetPixel(ServiceSampleType type, const glm::dvec2& latLong) const
{
    const auto it = Rasters.find(type);
    if (it == Rasters.end())
    {
        return ServicePixel{};
    }
    const double* transform = it->second.InverseGeoTransform;
    int x = int(transform[0] + latLong.y * transform[1] + latLong.x * transform[2]);
    int y = int(transform[3] + latLong.y * transform[4] + latLong.x * transform[5]);
    return GetPixel(type, x, y);
}

ServicePixel Service::GetPixel(ServiceSampleType type, int x, int y) const
{
    const auto it = Rasters.find(type);
    if (it == Rasters.end())
    {
        return ServicePixel{};
    }
    const Raster& raster = it->second;
    if (x < 0 || y < 0 || x >= raster.Width || y >= raster.Height)
    {
        return ServicePixel{};
    }
    return raster.Pixels[size_t(y) * raster.Width + x];
}

glm::ivec2 Service::GetSize(ServiceSampleType type) const
{
    const auto it = Rasters.find(type);
    if (it != Rasters.end())
    {
        return glm::ivec2(it->second.Width, it->second.Height);
    }
    else
    {
        return glm::ivec2(0);
    }
}

ImTextureRef Service::GetTextureRef(ServiceSampleType type)
{
    const auto it = Rasters.find(type);
    if (it != Rasters.end())
    {
        return it->second.Texture;
    } 
    else
    {
        return ImTextureRef();
    }
}

void Service::DEMProcessing(GDALDatasetH elevation, const std::string& basePath, ServiceSampleType type)
{
    std::string path = std::format("{}_{}.tif", basePath, int(type));
    if (std::filesystem::exists(path))
    {
        return;
    }
    const char* processing;
    std::vector<const char*> args = { "-of", "GTiff", "-compute_edges" };
    if (type == ServiceSampleType::Slope)
    {
        static constexpr const char* kDegreesToMetres = "111120";
        processing = "slope";
        args.push_back("-s");
        args.push_back(kDegreesToMetres);
    }
    else if (type == ServiceSampleType::Aspect)
    {
        processing = "aspect";
        args.push_back("-zero_for_flat");
    }
    else
    {
        SDL_assert(false);
    }
    args.push_back(nullptr);
    GDALDEMProcessingOptions* options = GDALDEMProcessingOptionsNew(const_cast<char**>(args.data()), nullptr);
    GDALDatasetH result = GDALDEMProcessing(path.c_str(), elevation, processing, nullptr, options, nullptr);
    if (!result)
    {
        spdlog::error("Failed to derive {} to {}: {}", processing, path, CPLGetLastErrorMsg());
    }
    else
    {
        GDALClose(result);
    }
    GDALDEMProcessingOptionsFree(options);
}
