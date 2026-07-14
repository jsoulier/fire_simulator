#include <SDL3/SDL.h>
#include <imgui.h>

#include <memory>
#include <string>
#include <vector>

#include "service.hpp"

class ServiceCustom : public Service
{
public:
    ServiceCustom()
    {
        for (int index = 0; index < 32; index++)
        {
            const ServiceSampleType type = ServiceSampleType(1 << index);
            if ((ServiceSampleType::All & type) == ServiceSampleType{})
            {
                continue;
            }
            ServiceSampleTypeValue value{};
            switch (type)
            {
            case ServiceSampleType::FuelModel:
                value.U32 = kFireFuelModelGR2;
                break;
            case ServiceSampleType::Elevation:
            case ServiceSampleType::Slope:
            case ServiceSampleType::Aspect:
            case ServiceSampleType::CanopyCover:
            case ServiceSampleType::CanopyHeight:
            case ServiceSampleType::WindDirection:
                value.F32 = 0.0f;
                break;
            case ServiceSampleType::CrownRatio:
                value.F32 = 0.5f;
                break;
            case ServiceSampleType::WindSpeed:
                value.F32 = 5.0f;
                break;
            case ServiceSampleType::MoistureOneHour:
                value.F32 = 8.0f;
                break;
            case ServiceSampleType::MoistureTenHour:
                value.F32 = 9.0f;
                break;
            case ServiceSampleType::MoistureHundredHour:
                value.F32 = 10.0f;
                break;
            case ServiceSampleType::MoistureLiveHerbaceous:
                value.F32 = 60.0f;
                break;
            case ServiceSampleType::MoistureLiveWoody:
                value.F32 = 90.0f;
                break;
            case ServiceSampleType::Temperature:
                value.F32 = 20.0f;
                break;
            case ServiceSampleType::RelativeHumidity:
                value.F32 = 30.0f;
                break;
            case ServiceSampleType::Precipitation:
                value.F32 = 0.0f;
                break;
            default:
                SDL_assert(false);
                break;
            }
            if (ServiceSampleTypeToTime(type) == ServiceSampleTypeTime::Dynamic)
            {
                DynamicSampleData& data = DynamicData[type];
                data.Start = 0.0f;
                data.Resolution = 1.0f;
                data.Samples.push_back(value);
            }
            else
            {
                StaticData[type].Pixels.push_back(value);
            }
        }
    }

    const char* GetName() const override
    {
        return "custom";
    }

    const char* GetDisplayName() const override
    {
        return "Custom";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::All;
    }

    void RenderImGui() override
    {
        for (int index = 0; index < 32; index++)
        {
            const ServiceSampleType type = ServiceSampleType(1 << index);
            if ((ServiceSampleType::All & type) == ServiceSampleType{})
            {
                continue;
            }
            ServiceSampleTypeValue* value = nullptr;
            if (ServiceSampleTypeToTime(type) == ServiceSampleTypeTime::Dynamic)
            {
                value = &DynamicData.at(type).Samples.front();
            }
            else
            {
                value = &StaticData.at(type).Pixels.front();
            }
            std::string label = std::format("{}##Custom", ServiceSampleTypeToString(type));
            if (ServiceSampleTypeToFormat(type) == ServiceSampleTypeFormat::U32)
            {
                ImGui::InputScalar(label.data(), ImGuiDataType_U32, &value->U32);
            }
            else if (ServiceSampleTypeToFormat(type) == ServiceSampleTypeFormat::F32)
            {
                ImGui::InputFloat(label.data(), &value->F32);
            }
            else
            {
                SDL_assert(false);
            }
        }
    }

    void Download(
        ServiceSampleType types,
        const glm::dvec2& minLatLong,
        const glm::dvec2& maxLatLong,
        float tileResolution,
        float timeResolution,
        const Date& startDate,
        const Date& endDate,
        const std::filesystem::path& directory) override
    {
    }
};

std::unique_ptr<Service> ServiceCreateCustom()
{
    return std::make_unique<ServiceCustom>();
}
