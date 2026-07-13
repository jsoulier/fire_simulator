#include <SDL3/SDL.h>
#include <ankerl/unordered_dense.h>
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
        Values[ServiceSampleType::FuelModel].U32 = kFireFuelModelGR2;
        Values[ServiceSampleType::Elevation].F32 = 0.0f;
        Values[ServiceSampleType::Slope].F32 = 0.0f;
        Values[ServiceSampleType::Aspect].F32 = 0.0f;
        Values[ServiceSampleType::CanopyCover].F32 = 0.0f;
        Values[ServiceSampleType::CanopyHeight].F32 = 0.0f;
        Values[ServiceSampleType::CrownRatio].F32 = 0.5f;
        Values[ServiceSampleType::WindSpeed].F32 = 5.0f;
        Values[ServiceSampleType::WindDirection].F32 = 0.0f;
        Values[ServiceSampleType::MoistureOneHour].F32 = 8.0f;
        Values[ServiceSampleType::MoistureTenHour].F32 = 9.0f;
        Values[ServiceSampleType::MoistureHundredHour].F32 = 10.0f;
        Values[ServiceSampleType::MoistureLiveHerbaceous].F32 = 60.0f;
        Values[ServiceSampleType::MoistureLiveWoody].F32 = 90.0f;
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
        for (auto& [type, value] : Values)
        {
            std::string sliderLabel = std::format("{}##Custom", ServiceSampleTypeToString(type));
            if (ServiceSampleTypeToFormat(type) == ServiceSampleTypeFormat::U32)
            {
                ImGui::InputScalar(sliderLabel.c_str(), ImGuiDataType_U32, &value.U32);
            }
            else if (ServiceSampleTypeToFormat(type) == ServiceSampleTypeFormat::F32)
            {
                ImGui::InputFloat(sliderLabel.c_str(), &value.F32);
            }
            else
            {
                SDL_assert(false);
            }
        }
    }

    ServiceSampleTypeValue GetValue(ServiceSampleType type, const glm::dvec2& latLong, float time) const override
    {
        return Values.at(type);
    }

    ServiceSampleTypeValue GetValue(ServiceSampleType type, int x, int y, float time) const override
    {
        return GetValue(type, glm::dvec2{}, time);
    }

    ankerl::unordered_dense::map<ServiceSampleType, ServiceSampleTypeValue> Values;
};

std::unique_ptr<Service> ServiceCreateCustom()
{
    return std::make_unique<ServiceCustom>();
}
