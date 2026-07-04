#pragma once

#include <ankerl/unordered_dense.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "fire_fuel_model.hpp"

enum class ServiceSampleType
{
    FuelModel = 1 << 0,
    Elevation = 1 << 1,
    Slope = 1 << 2,
    Aspect = 1 << 3,
    CanopyCover = 1 << 4,
    CanopyHeight = 1 << 5,
    CrownRatio = 1 << 6,
    WindSpeed = 1 << 7,
    WindDirection = 1 << 8,
    MoistureOneHour = 1 << 9,
    MoistureTenHour = 1 << 10,
    MoistureHundredHour = 1 << 11,
    MoistureLiveHerbaceous = 1 << 12,
    MoistureLiveWoody = 1 << 13,
};

struct ServiceSample
{
    FireFuelModelType FuelModel;
    float Elevation;
    float Slope;
    float Aspect;
    float CanopyCover;
    float CanopyHeight;
    float CrownRatio;
    float WindSpeed;
    float WindDirection;
    float MoistureOneHour;
    float MoistureTenHour;
    float MoistureHundredHour;
    float MoistureLiveHerbaceous;
    float MoistureLiveWoody;
};

class Service
{
public:
    virtual ~Service() = default;
    virtual std::string GetName() const = 0;
    virtual ServiceSampleType GetSupportedTypes() const = 0;
    void Download(ServiceSampleType types, const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, double resolution);
    void SetSample(const glm::dvec2& latLong, ServiceSample& sample, ServiceSampleType type);
    ImTextureRef GetTextureRef(ServiceSampleType type);
    
protected:
    union Pixel
    {
        uint32_t U32;
        float F32;
    };

private:
    virtual std::vector<std::string> GetSourceURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong) const = 0;
    virtual int GetBand(ServiceSampleType type) const = 0;
    virtual void PostProcess(ServiceSampleType type, std::vector<Pixel>& pixels) {}

    struct Raster
    {
        Raster();

        int Width;
        int Height;
        double GeoTransform[6];
        double InverseGeoTransform[6];
        std::string Wkt;
        std::vector<Pixel> Pixels;
        ImTextureRef Texture;
    };

    ankerl::unordered_dense::map<ServiceSampleType, Raster> Rasters;
};

std::unique_ptr<Service> ServiceCreateESAWorldCover();
