#include <SDL3/SDL.h>
#include <imgui.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "service.hpp"
#include "service_context.hpp"
#include "version.hpp"

class ServiceCustom : public Service
{
    SAVEPOINT_POLY(ServiceCustom)

public:
    ServiceCustom()
    {
        Values.resize(SDL_arraysize(kServiceSampleTypeStrings));
        for (int index = 0; index < kServiceSampleTypeMax; index++)
        {
            const ServiceSampleType type = ServiceSampleTypeFromIndex(index);
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
            case ServiceSampleType::SolarRadiation:
            case ServiceSampleType::Snowfall:
            case ServiceSampleType::SnowDepth:
                value.F32 = 0.0f;
                break;
            default:
                SDL_assert(false);
                break;
            }
            Values[ServiceSampleTypeToIndex(type)] = value;
        }
    }

    void Visit(SavepointVisitor& visitor) override
    {
        Service::Visit(visitor);
        visitor(Values, {0, 0, 1});
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

    void RenderImGui(ServiceContext& context) override
    {
        for (int index = 0; index < kServiceSampleTypeMax; index++)
        {
            const ServiceSampleType type = ServiceSampleTypeFromIndex(index);
            if ((ServiceSampleType::All & type) == ServiceSampleType{})
            {
                continue;
            }
            ServiceSampleTypeValue* value = &Values[ServiceSampleTypeToIndex(type)];
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
        ServiceContext& context,
        ServiceSampleType types,
        const glm::dvec2& minLatLong,
        const glm::dvec2& maxLatLong,
        float tileResolution,
        float timeResolution,
        const Date& startDate,
        const Date& endDate,
        const std::filesystem::path& directory) override
    {
        for (int index = 0; index < kServiceSampleTypeMax; index++)
        {
            const ServiceSampleType type = ServiceSampleTypeFromIndex(index);
            if ((types & type) == ServiceSampleType{})
            {
                continue;
            }
            const ServiceSampleTypeValue value = Values[ServiceSampleTypeToIndex(type)];
            if ((ServiceSampleType::Dynamic & type) != ServiceSampleType{})
            {
                ServiceContextDynamicData data;
                data.Start = 0.0f;
                data.Resolution = timeResolution;
                data.Samples.push_back(value);
                context[type] = std::move(data);
            }
            else
            {
                ServiceContextStaticData data;
                data.Pixels.push_back(value);
                context[type] = std::move(data);
            }
        }
    }

    std::vector<ServiceSampleTypeValue> Values;
};

std::unique_ptr<Service> ServiceCreateCustom()
{
    return std::make_unique<ServiceCustom>();
}
