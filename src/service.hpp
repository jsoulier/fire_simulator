#pragma once

#include <gdal_fwd.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <savepoint/savepoint.hpp>

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
    SolarRadiation = 1 << 17,
    Snowfall = 1 << 18,
    SnowDepth = 1 << 19,
    Static =
        FuelModel |
        Elevation |
        Slope |
        Aspect |
        CanopyCover |
        CanopyHeight |
        CrownRatio,
    Dynamic =
        WindSpeed |
        WindDirection |
        MoistureOneHour |
        MoistureTenHour |
        MoistureHundredHour |
        MoistureLiveHerbaceous |
        MoistureLiveWoody |
        Temperature |
        RelativeHumidity |
        Precipitation |
        SolarRadiation |
        Snowfall |
        SnowDepth,
    All = Static | Dynamic,
};

static constexpr int kServiceSampleTypeMax = 20;

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

static constexpr const char* kServiceSampleTypeStrings[kServiceSampleTypeMax] =
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
    "Solar Radiation",
    "Snowfall",
    "Snow Depth",
};

static constexpr ServiceSampleTypeFormat kServiceSampleTypeFormats[kServiceSampleTypeMax] =
{
    ServiceSampleTypeFormat::U32, // fuel model
    ServiceSampleTypeFormat::F32, // elevation
    ServiceSampleTypeFormat::F32, // slope
    ServiceSampleTypeFormat::F32, // aspect
    ServiceSampleTypeFormat::F32, // canopy cover
    ServiceSampleTypeFormat::F32, // canopy height
    ServiceSampleTypeFormat::F32, // crown ratio
    ServiceSampleTypeFormat::F32, // wind speed
    ServiceSampleTypeFormat::F32, // wind direction
    ServiceSampleTypeFormat::F32, // 1h moisture
    ServiceSampleTypeFormat::F32, // 10h moisture
    ServiceSampleTypeFormat::F32, // 100h moisture
    ServiceSampleTypeFormat::F32, // live herbaceous moisture
    ServiceSampleTypeFormat::F32, // live woody moisture
    ServiceSampleTypeFormat::F32, // temperature
    ServiceSampleTypeFormat::F32, // relative humidity
    ServiceSampleTypeFormat::F32, // precipitation
    ServiceSampleTypeFormat::F32, // solar radiation
    ServiceSampleTypeFormat::F32, // snowfall
    ServiceSampleTypeFormat::F32, // snow depth
};

uint32_t ServiceSampleTypeToIndex(ServiceSampleType type);
ServiceSampleType ServiceSampleTypeFromIndex(uint32_t index);
const char* ServiceSampleTypeToString(ServiceSampleType type);
ServiceSampleTypeFormat ServiceSampleTypeToFormat(ServiceSampleType type);

class ServiceContext;

class Service : public SavepointPoly
{
public:
    virtual ~Service() = default;
    virtual void Visit(SavepointVisitor& visitor) {}
    virtual const char* GetName() const = 0;
    virtual const char* GetDisplayName() const = 0;
    virtual ServiceSampleType GetSupportedTypes() const = 0;
    virtual ServiceSampleType GetRequiredSampleTypes(ServiceSampleType types) const { return {}; }
    virtual ServiceSampleType GetDerivedSampleTypes() const { return {}; }
    virtual void RenderImGui(ServiceContext& context);
    virtual void Download(
        ServiceContext& context,
        ServiceSampleType types,
        const glm::dvec2& minLatLong,
        const glm::dvec2& maxLatLong,
        float tileResolution,
        float timeResolution,
        const Date& startDate,
        const Date& endDate,
        const std::filesystem::path& directory);

protected:
    virtual std::vector<std::string> GetURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, const Date& startDate, const Date& endDate) const { return {}; }
    virtual std::vector<ServiceSampleTypeDynamicValue> GetDynamicValues(const std::string& response, ServiceSampleType type) const { return {}; }
    virtual void DeriveDynamicData(ServiceContext& context, ServiceSampleType type, const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, const Date& startDate) {}
    virtual void PostProcessDynamicData(ServiceContext& context) {}
    virtual int GetBand(ServiceSampleType type) const { return 0; }
    virtual void DeriveStaticData(ServiceSampleType inType, ServiceSampleType outType, GDALDatasetH lowResolution, const std::string& directory) {}
    virtual void PostProcessStaticData(ServiceSampleType type, std::vector<ServiceSampleTypeValue>& pixels) {}
    void DEMProcessing(GDALDatasetH elevation, const std::string& directory, ServiceSampleType type);

private:
    float Time = 0.0f;
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
