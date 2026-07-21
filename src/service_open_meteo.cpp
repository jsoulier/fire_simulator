// https://open-meteo.com/en/docs
// NFDRS: https://research.fs.usda.gov/download/treesearch/68223.pdf

#include <SDL3/SDL.h>
#include <imgui.h>
#include <nfdrs4.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "fire_fuel_model.hpp"
#include "math.hpp"
#include "service.hpp"
#include "service_context.hpp"

static constexpr int kPrefixDays = 56; // extra days required to compute moisture levels

class ServiceOpenMeteo : public Service
{
    SAVEPOINT_POLY(ServiceOpenMeteo)

public:
    const char* GetName() const override
    {
        return "open_meteo";
    }

    const char* GetDisplayName() const override
    {
        return "Open-Meteo";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::WindSpeed |
            ServiceSampleType::WindDirection |
            ServiceSampleType::MoistureOneHour |
            ServiceSampleType::MoistureTenHour |
            ServiceSampleType::MoistureHundredHour |
            ServiceSampleType::MoistureLiveHerbaceous |
            ServiceSampleType::MoistureLiveWoody |
            ServiceSampleType::Temperature |
            ServiceSampleType::RelativeHumidity |
            ServiceSampleType::Precipitation |
            ServiceSampleType::SolarRadiation |
            ServiceSampleType::Snowfall |
            ServiceSampleType::SnowDepth;
    }

    ServiceSampleType GetRequiredSampleTypes(ServiceSampleType types) const override
    {
        static constexpr ServiceSampleType kTypes =
            ServiceSampleType::MoistureOneHour |
            ServiceSampleType::MoistureTenHour |
            ServiceSampleType::MoistureHundredHour |
            ServiceSampleType::MoistureLiveHerbaceous |
            ServiceSampleType::MoistureLiveWoody;
        if ((types & kTypes) != ServiceSampleType{})
        {
            return ServiceSampleType::Temperature |
                ServiceSampleType::RelativeHumidity |
                ServiceSampleType::Precipitation |
                ServiceSampleType::SolarRadiation |
                ServiceSampleType::WindSpeed |
                ServiceSampleType::Snowfall |
                ServiceSampleType::SnowDepth;
        }
        else
        {
            return ServiceSampleType{};
        }
    }

    std::vector<std::string> GetURLs(const glm::dvec2& min, const glm::dvec2& max, const Date& start, const Date& end) const override
    {
        return
        {
            std::format(
                "https://archive-api.open-meteo.com/v1/archive?latitude={:.6f}&longitude={:.6f}&start_date={}&end_date={}&"
                "hourly=wind_speed_10m,wind_direction_10m,temperature_2m,relative_humidity_2m,precipitation,shortwave_radiation,snowfall,snow_depth&"
                "wind_speed_unit=mph&timezone=auto",
                (min.x + max.x) * 0.5,
                (min.y + max.y) * 0.5,
                start.AddDays(-kPrefixDays).ToString(),
                end.ToString()),
        };
    }

    std::vector<ServiceSampleTypeDynamicValue> GetDynamicValues(const std::string& response, ServiceSampleType type) const override
    {
        nlohmann::json json = nlohmann::json::parse(response, nullptr, false);
        const char* name = nullptr;
        switch (type)
        {
        case ServiceSampleType::WindSpeed:
            name = "wind_speed_10m";
            break;
        case ServiceSampleType::WindDirection:
            name = "wind_direction_10m";
            break;
        case ServiceSampleType::Temperature:
            name = "temperature_2m";
            break;
        case ServiceSampleType::RelativeHumidity:
            name = "relative_humidity_2m";
            break;
        case ServiceSampleType::Precipitation:
            name = "precipitation";
            break;
        case ServiceSampleType::SolarRadiation:
            name = "shortwave_radiation";
            break;
        case ServiceSampleType::Snowfall:
            name = "snowfall";
            break;
        case ServiceSampleType::SnowDepth:
            name = "snow_depth";
            break;
        case ServiceSampleType::MoistureOneHour:
        case ServiceSampleType::MoistureTenHour:
        case ServiceSampleType::MoistureHundredHour:
        case ServiceSampleType::MoistureLiveHerbaceous:
        case ServiceSampleType::MoistureLiveWoody:
            return {};
        default:
            SDL_assert(false);
            return {};
        }
        if (json.is_discarded() || !json.is_object() || !json.contains("hourly") || !json["hourly"].is_object())
        {
            spdlog::warn("Failed to load {}: {}", name, GetName());
            return {};
        }
        nlohmann::json& hourly = json["hourly"];
        if (!hourly.contains("time") || !hourly["time"].is_array() || hourly["time"].empty() || !hourly.contains(name) ||
            !hourly[name].is_array() || hourly[name].size() != hourly["time"].size() ||
            !std::all_of(hourly["time"].begin(), hourly["time"].end(), [](const nlohmann::json& json) { return json.is_string(); }) ||
            !std::all_of(hourly[name].begin(), hourly[name].end(), [](const nlohmann::json& json) { return json.is_number(); }))
        {
            spdlog::warn("Failed to load {}: {}", name, GetName());
            return {};
        }
        const nlohmann::json& time = hourly["time"];
        const nlohmann::json& sample = hourly[name];
        std::vector<ServiceSampleTypeDynamicValue> values;
        values.reserve(time.size());
        for (size_t i = 0; i < time.size(); i++)
        {
            ServiceSampleTypeDynamicValue value{};
            value.Time = Date(time[i]).ToEpoch() / 60.0f;
            value.Value.F32 = sample[i];
            values.push_back(value);
        }
        return values;
    }

    void DeriveDynamicData(
        ServiceContext& context,
        ServiceSampleType type,
        const glm::dvec2& minLatLong,
        const glm::dvec2& maxLatLong,
        const Date& startDate) override
    {
        if (type != ServiceSampleType::Temperature ||
            !context.Contains(ServiceSampleType::RelativeHumidity) ||
            !context.Contains(ServiceSampleType::Precipitation) ||
            !context.Contains(ServiceSampleType::SolarRadiation) ||
            !context.Contains(ServiceSampleType::WindSpeed) ||
            !context.Contains(ServiceSampleType::Snowfall))
        {
            return;
        }
        const ServiceContextStaticData& fuelModels = std::get<ServiceContextStaticData>(context.At(ServiceSampleType::FuelModel));
        const ServiceContextDynamicData& temperature = std::get<ServiceContextDynamicData>(context.At(ServiceSampleType::Temperature));
        const ServiceContextDynamicData& humidity = std::get<ServiceContextDynamicData>(context.At(ServiceSampleType::RelativeHumidity));
        const ServiceContextDynamicData& precipitation = std::get<ServiceContextDynamicData>(context.At(ServiceSampleType::Precipitation));
        const ServiceContextDynamicData& solarRadiation = std::get<ServiceContextDynamicData>(context.At(ServiceSampleType::SolarRadiation));
        const ServiceContextDynamicData& windSpeed = std::get<ServiceContextDynamicData>(context.At(ServiceSampleType::WindSpeed));
        const ServiceContextDynamicData& snowfall = std::get<ServiceContextDynamicData>(context.At(ServiceSampleType::Snowfall));
        const ServiceContextDynamicData* snowDepth = context.Contains(ServiceSampleType::SnowDepth) ? &std::get<ServiceContextDynamicData>(context.At(ServiceSampleType::SnowDepth)) : nullptr;
        SDL_assert(temperature.Samples.size() == humidity.Samples.size());
        SDL_assert(temperature.Samples.size() == precipitation.Samples.size());
        SDL_assert(temperature.Samples.size() == solarRadiation.Samples.size());
        SDL_assert(temperature.Samples.size() == windSpeed.Samples.size());
        SDL_assert(temperature.Samples.size() == snowfall.Samples.size());
        SDL_assert(!snowDepth || temperature.Samples.size() == snowDepth->Samples.size());
        const int resolution = std::lround(temperature.Resolution);
        SDL_assert(resolution >= 1 && std::abs(temperature.Resolution - resolution) < 0.001f);
        const double latitude = (minLatLong.x + maxLatLong.x) * 0.5;
        std::array<int, 256> fuelModelCounts{};
        for (const ServiceSampleTypeValue& fuelModel : fuelModels.Pixels)
        {
            const FireFuelModelType value = fuelModel.U32;
            if (FireFuelModelTypeIsBurnable(value))
            {
                fuelModelCounts[value]++;
            }
        }
        // TODO: remove averages when we support per-cell weather calculations
        const auto mode = std::max_element(fuelModelCounts.begin(), fuelModelCounts.end());
        SDL_assert(mode != fuelModelCounts.end() && *mode > 0);
        const FireFuelModelType modeFuelModel = std::distance(fuelModelCounts.begin(), mode);
        const char fuelModel = FireFuelModelToNFDRS(modeFuelModel);
        NFDRS4 nfdrs;
        nfdrs.Init(latitude, fuelModel, 1, 0.0, true, true, false, 100, 13);
        // TODO: move to ImGui parameters (see ServiceCustom)
        nfdrs.SetHerbGSIparams(
            1.0,         // maximum GSI
            0.2,         // greenup threshold
            -2.0,        // minimum temperature lower limit (celsius)
            5.0,         // minimum temperature upper limit (celsius)
            900.0,       // vapor pressure deficit lower limit (pascals)
            4100.0,      // vapor pressure deficit upper limit (pascals)
            36000.0,     // daylight lower limit (seconds)
            39600.0,     // daylight upper limit (seconds)
            28,          // moving average period (days) (number of days to smooth GSI)
            false,       // use vapor pressure deficit average
            28,          // precipitation (days)
            0.0,         // running total precipitation lower limit (inches)
            10.0 / 25.4, // running total precipitation upper limit (inches)
            true,        // use running total precipitation
            30.0,        // minimum herbaceous fuel moisture (percent)
            250.0);      // maximum herbaceous fuel moisture (percent) 
        nfdrs.SetWoodyGSIparams(
            1.0,         // maximum GSI
            0.2,         // greenup threshold
            -2.0,        // minimum temperature lower limit (celsius)
            5.0,         // minimum temperature upper limit (celsius)
            900.0,       // vpd lower limit (pascals)
            4100.0,      // vpd upper limit (pascals)
            36000.0,     // daylight lower limit (seconds)
            39600.0,     // daylight upper limit (seconds)
            28,          // moving average period (days) (number of days to smooth GSI)
            false,       // use vapor pressure deficit average
            28,          // precipitation (days)
            0.0,         // running total precipitation lower limit (inches)
            10.0 / 25.4, // running total precipitation upper limit (inches)
            true,        // use running total precipitation
            60.0,        // minimum woody fuel moisture (percent)
            200.0);      // maximum woody fuel moisture (percent)
        std::vector<ServiceSampleTypeValue> oneHour;
        std::vector<ServiceSampleTypeValue> tenHour;
        std::vector<ServiceSampleTypeValue> hundredHour;
        std::vector<ServiceSampleTypeValue> liveHerbaceous;
        std::vector<ServiceSampleTypeValue> liveWoody;
        oneHour.reserve(temperature.Samples.size());
        tenHour.reserve(temperature.Samples.size());
        hundredHour.reserve(temperature.Samples.size());
        liveHerbaceous.reserve(temperature.Samples.size());
        liveWoody.reserve(temperature.Samples.size());
        const auto update = [&](int index, const Date& date)
        {
            const std::tm time = date.ToTm();
            nfdrs.Update(
                time.tm_year + 1900,
                time.tm_mon + 1,
                time.tm_mday,
                time.tm_hour,
                MathCelsiusToFahrenheit(temperature.Samples[index].F32),
                humidity.Samples[index].F32,
                precipitation.Samples[index].F32 / 25.4,
                solarRadiation.Samples[index].F32,
                windSpeed.Samples[index].F32,
                (snowDepth && snowDepth->Samples[index].F32 > 0.0f) || snowfall.Samples[index].F32 > 0.0f);
        };
        const Date date = startDate.AddHours(temperature.Start);
        update(0, date);
        for (int i = 0; i < temperature.Samples.size(); i++)
        {
            // NFDRS needs to be updated hourly
            for (auto hour = 1; hour < resolution; hour++)
            {
                update(i - 1, date.AddHours(hour - resolution));
            }
            oneHour.push_back({.F32 = float(nfdrs.MC1)});
            tenHour.push_back({.F32 = float(nfdrs.MC10)});
            hundredHour.push_back({.F32 = float(nfdrs.MC100)});
            liveHerbaceous.push_back({.F32 = float(nfdrs.MCHERB)});
            liveWoody.push_back({.F32 = float(nfdrs.MCWOOD)});
        }
        auto setMoisture = [&](ServiceSampleType type, std::vector<ServiceSampleTypeValue> moisture)
        {
            ServiceContextDynamicData data;
            data.Start = temperature.Start;
            data.Resolution = temperature.Resolution;
            data.Samples = std::move(moisture);
            context[type] = std::move(data);
        };
        setMoisture(ServiceSampleType::MoistureOneHour, std::move(oneHour));
        setMoisture(ServiceSampleType::MoistureTenHour, std::move(tenHour));
        setMoisture(ServiceSampleType::MoistureHundredHour, std::move(hundredHour));
        setMoisture(ServiceSampleType::MoistureLiveHerbaceous, std::move(liveHerbaceous));
        setMoisture(ServiceSampleType::MoistureLiveWoody, std::move(liveWoody));
    }

    void PostProcessDynamicData(ServiceContext& context) override
    {
        for (auto& [type, value] : context)
        {
            if ((GetSupportedTypes() & type) == ServiceSampleType{} || !std::holds_alternative<ServiceContextDynamicData>(value))
            {
                continue;
            }
            ServiceContextDynamicData& data = std::get<ServiceContextDynamicData>(value);
            if (data.Start >= 0.0f)
            {
                continue;
            }
            // strip kPrefixDays from the data
            int index = std::ceil(-data.Start / data.Resolution);
            SDL_assert(data.Samples.size() >= index);
            data.Samples.erase(data.Samples.begin(), data.Samples.begin() + index);
            data.Start += index * data.Resolution;
        }
    }
};

std::unique_ptr<Service> ServiceCreateOpenMeteo()
{
    return std::make_unique<ServiceOpenMeteo>();
}
