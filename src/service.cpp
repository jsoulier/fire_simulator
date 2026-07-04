#include <SDL3/SDL.h>
#include <gdal.h>
#include <gdal_utils.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <format>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "service.hpp"

static const std::filesystem::path kBasePath = SDL_GetBasePath();

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
            const std::string algorithmString = type == ServiceSampleType::FuelModel ? "mode" : "average";
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
            type == ServiceSampleType::FuelModel ? GDT_UInt32 : GDT_Float32, 0, 0);
        GDALClose(lowResolution);
        if (status != CE_None)
        {
            spdlog::error("Failed to read {}: {}", lowResolutionFilePath.string(), CPLGetLastErrorMsg());
            continue;
        }
        PostProcess(type, raster.Pixels);
        Rasters[type] = std::move(raster);
    }
    GDALClose(highResolution);
    ////////////////////////////////////////////////////////////////////////////
    // Create ImTextures for each band
    for (auto& [type, raster] : Rasters)
    {
        ImTextureData* texture = IM_NEW(ImTextureData)();
        texture->Create(ImTextureFormat_RGBA32, raster.Width, raster.Height);
        uint32_t* texels = reinterpret_cast<uint32_t*>(texture->GetPixels());
        if (type == ServiceSampleType::FuelModel)
        {
            for (size_t i = 0; i < raster.Pixels.size(); i++)
            {
                texels[i] = FireFuelModelTypeGetColor(FireFuelModelType(raster.Pixels[i].U32));
            }
        }
        else
        {
            float minValue = std::numeric_limits<float>::max();
            float maxValue = std::numeric_limits<float>::lowest();
            for (const Pixel& pixel : raster.Pixels)
            {
                minValue = std::min(minValue, pixel.F32);
                maxValue = std::max(maxValue, pixel.F32);
            }
            const float range = maxValue - minValue;
            for (size_t i = 0; i < raster.Pixels.size(); i++)
            {
                uint8_t gray = 0;
                if (range > 0.0f)
                {
                    gray = (raster.Pixels[i].F32 - minValue) / range * 255.0f;
                }
                texels[i] = IM_COL32(gray, gray, gray, 255);
            }
        }
        ImGui::GetPlatformIO().Textures.push_back(texture);
        raster.Texture = texture->GetTexRef();
    }
}

void Service::SetSample(const glm::dvec2& latLong, ServiceSample& sample, ServiceSampleType type)
{
}

ImTextureRef Service::GetTextureRef(ServiceSampleType type)
{
    return Rasters.at(type).Texture;
}
