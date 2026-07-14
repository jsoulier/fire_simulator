// https://open-meteo.com/en/docs

#include <SDL3/SDL_assert.h>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "service.hpp"

class ServiceOpenMeteo : public Service
{
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
        return ServiceSampleType::WindSpeed | ServiceSampleType::WindDirection | ServiceSampleType::Temperature | ServiceSampleType::RelativeHumidity | ServiceSampleType::Precipitation;
    }

    ServiceSampleType GetRequiredSampleTypes(ServiceSampleType types) const override
    {
        return ServiceSampleType::Temperature | ServiceSampleType::RelativeHumidity | ServiceSampleType::Precipitation;
    }

    std::vector<std::string> GetURLs(const glm::dvec2& min, const glm::dvec2& max, const Date& start, const Date& end) const override
    {
        return
        {
            std::format(
                "https://archive-api.open-meteo.com/v1/archive?latitude={:.6f}&longitude={:.6f}&start_date={}&end_date={}&hourly=wind_speed_10m,wind_direction_10m,temperature_2m,relative_humidity_2m,precipitation&wind_speed_unit=mph&timezone=UTC",
                (min.x + max.x) * 0.5,
                (min.y + max.y) * 0.5,
                start.ToString(),
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
        for (size_t index = 0; index < time.size(); index++)
        {
            ServiceSampleTypeDynamicValue value{};
            value.Time = Date(time[index].get_ref<const std::string&>()).ToEpoch() / 60.0f;
            value.Value.F32 = sample[index].get<float>();
            values.push_back(value);
        }
        return values;
    }
};

std::unique_ptr<Service> ServiceCreateOpenMeteo()
{
    return std::make_unique<ServiceOpenMeteo>();
}
