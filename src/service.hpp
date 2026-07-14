#pragma once

#include <ankerl/unordered_dense.h>
#include <gdal_fwd.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "fire_fuel_model.hpp"
#include "date.hpp"

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
    Temperature = 1 << 14,
    RelativeHumidity = 1 << 15,
    Precipitation = 1 << 16,
    All =
        FuelModel |
        Elevation |
        Slope |
        Aspect |
        CanopyCover |
        CanopyHeight |
        CrownRatio |
        WindSpeed |
        WindDirection |
        MoistureOneHour |
        MoistureTenHour |
        MoistureHundredHour |
        MoistureLiveHerbaceous |
        MoistureLiveWoody |
        Temperature |
        RelativeHumidity |
        Precipitation,
};

constexpr ServiceSampleType operator|(ServiceSampleType a, ServiceSampleType b)
{
    return ServiceSampleType(int(a) | int(b));
}

constexpr ServiceSampleType operator&(ServiceSampleType a, ServiceSampleType b)
{
    return ServiceSampleType(int(a) & int(b));
}

constexpr ServiceSampleType operator~(ServiceSampleType a)
{
    return ServiceSampleType(~int(a));
}

constexpr ServiceSampleType& operator|=(ServiceSampleType& a, ServiceSampleType b)
{
    return a = a | b;
}

enum class ServiceSampleTypeFormat
{
    F32,
    U32,
};

union ServiceSampleTypeValue
{
    float F32;
    uint32_t U32;
};

struct ServiceSampleTypeDynamicValue
{
    ServiceSampleTypeValue Value;
    float Time; // epoch hours
};

enum class ServiceSampleTypeTime
{
    Static,
    Dynamic,
};

static constexpr const char* kServiceSampleTypeStrings[] =
{
    "Fuel Model",
    "Elevation",
    "Slope",
    "Aspect",
    "Canopy Cover",
    "Canopy Height",
    "Crown Ratio",
    "Wind Speed",
    "Wind Direction",
    "Moisture 1 Hour",
    "Moisture 10 Hour",
    "Moisture 100 Hour",
    "Moisture Live Herbaceous",
    "Moisture Live Woody",
    "Temperature",
    "Relative Humidity",
    "Precipitation",
};

static constexpr ServiceSampleTypeFormat kServiceSampleTypeFormats[] =
{
    ServiceSampleTypeFormat::U32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
    ServiceSampleTypeFormat::F32,
};

static constexpr ServiceSampleTypeTime kServiceSampleTypeTimes[] =
{
    ServiceSampleTypeTime::Static,
    ServiceSampleTypeTime::Static,
    ServiceSampleTypeTime::Static,
    ServiceSampleTypeTime::Static,
    ServiceSampleTypeTime::Static,
    ServiceSampleTypeTime::Static,
    ServiceSampleTypeTime::Static,
    ServiceSampleTypeTime::Dynamic,
    ServiceSampleTypeTime::Dynamic,
    ServiceSampleTypeTime::Dynamic,
    ServiceSampleTypeTime::Dynamic,
    ServiceSampleTypeTime::Dynamic,
    ServiceSampleTypeTime::Dynamic,
    ServiceSampleTypeTime::Dynamic,
    ServiceSampleTypeTime::Dynamic,
    ServiceSampleTypeTime::Dynamic,
    ServiceSampleTypeTime::Dynamic,
};

uint32_t ServiceSampleTypeToIndex(ServiceSampleType type);
ServiceSampleType ServiceSampleTypeFromIndex(uint32_t index);
const char* ServiceSampleTypeToString(ServiceSampleType type);
ServiceSampleTypeTime ServiceSampleTypeToTime(ServiceSampleType type);
ServiceSampleTypeFormat ServiceSampleTypeToFormat(ServiceSampleType type);

class Service
{
protected:
    struct StaticSampleData;
    struct DynamicSampleData;

public:
    virtual ~Service() = default;
    virtual const char* GetName() const = 0;
    virtual const char* GetDisplayName() const = 0;
    virtual ServiceSampleType GetSupportedTypes() const = 0;
    virtual ServiceSampleType GetRequiredSampleTypes(ServiceSampleType types) const { return {}; }
    virtual void RenderImGui();
    virtual void Download(
        ServiceSampleType types,
        const glm::dvec2& minLatLong,
        const glm::dvec2& maxLatLong,
        float tileResolution,
        float timeResolution,
        const Date& startDate,
        const Date& endDate,
        const std::filesystem::path& directory);
    ServiceSampleTypeValue GetValue(ServiceSampleType type, const glm::dvec2& latLong, float time) const;
    ServiceSampleTypeValue GetValue(ServiceSampleType type, int x, int y, float time) const;
    glm::ivec2 GetSize(ServiceSampleType type) const;
    ImTextureRef GetTextureRef(ServiceSampleType type);

protected:
    virtual std::vector<std::string> GetURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, const Date& startDate, const Date& endDate) const { return {}; }
    virtual std::vector<ServiceSampleTypeDynamicValue> GetDynamicValues(const std::string& response, ServiceSampleType type) const { return {}; }
    virtual void DeriveDynamicData(ServiceSampleType type) {}
    virtual int GetBand(ServiceSampleType type) const { return 0; }
    virtual void DeriveStaticData(ServiceSampleType type, GDALDatasetH lowResolution, const std::string& directory) {}
    virtual void PostProcess(ServiceSampleType type, std::vector<ServiceSampleTypeValue>& pixels) {}
    ServiceSampleTypeValue GetDynamicValue(ServiceSampleType type, float time) const;
    void DEMProcessing(GDALDatasetH elevation, const std::string& directory, ServiceSampleType type);

    struct StaticSampleData
    {
        StaticSampleData();

        int Width;
        int Height;
        // [0] upper-left corner X
        // [1] pixel width
        // [2] row rotation (0 for north-up)
        // [3] upper-left corner Y
        // [4] pixel height (usually negative)
        // [5] column rotation (0 for north-up)
        double GeoTransform[6];
        double InverseGeoTransform[6];
        std::string Wkt;
        std::vector<ServiceSampleTypeValue> Pixels;
        ImTextureRef Texture;
    };

    struct DynamicSampleData
    {
        DynamicSampleData();

        float Start;      // hours
        float Resolution; // hours per sample
        std::vector<ServiceSampleTypeValue> Samples;
    };

    ankerl::unordered_dense::map<ServiceSampleType, StaticSampleData> StaticData;
    ankerl::unordered_dense::map<ServiceSampleType, DynamicSampleData> DynamicData;
    float DynamicTime = 0.0f;
};

std::unique_ptr<Service> ServiceCreateESAWorldCover();
std::unique_ptr<Service> ServiceCreateNRCan();
std::unique_ptr<Service> ServiceCreateOpenTopography();
std::unique_ptr<Service> ServiceCreateCustom();
std::unique_ptr<Service> ServiceCreateLandfireFuelModel();
std::unique_ptr<Service> ServiceCreateLandfireElevation();
std::unique_ptr<Service> ServiceCreateLandfireSlope();
std::unique_ptr<Service> ServiceCreateLandfireAspect();
std::unique_ptr<Service> ServiceCreateLandfireCanopyCover();
std::unique_ptr<Service> ServiceCreateLandfireCanopyHeight();
std::unique_ptr<Service> ServiceCreateOpenMeteo();
