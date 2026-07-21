#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include <filesystem>
#include <memory>
#include <utility>

#include "math.hpp"
#include "service_manager.hpp"
#include "version.hpp"

static const std::filesystem::path kBasePath = SDL_GetBasePath();
static const std::filesystem::path kSavepointPath = kBasePath / "fire_simulator_results.csv";

ServiceManager::ServiceManager()
    : ReferenceIndex{0}
    , TileResolution{0.001f}
    , TimeResolution{1.0f}
{
    Simulator.SetCoordinatorType(FireSimulatorCoordinatorType::EventDriven);
    Services.emplace_back(ServiceCreateNRCan());
    Services.emplace_back(ServiceCreateESAWorldCover());
    Services.emplace_back(ServiceCreateOpenTopography());
    Services.emplace_back(ServiceCreateCustom());
    Services.emplace_back(ServiceCreateLandfireFuelModel());
    Services.emplace_back(ServiceCreateLandfireElevation());
    Services.emplace_back(ServiceCreateLandfireSlope());
    Services.emplace_back(ServiceCreateLandfireAspect());
    Services.emplace_back(ServiceCreateLandfireCanopyCover());
    Services.emplace_back(ServiceCreateLandfireCanopyHeight());
    Services.emplace_back(ServiceCreateOpenMeteo());
    ServiceIndices.fill(3);
    ServiceIndices[ServiceSampleTypeToIndex(ServiceSampleType::FuelModel)] = 0;
    ServiceIndices[ServiceSampleTypeToIndex(ServiceSampleType::Elevation)] = 2;
    ServiceIndices[ServiceSampleTypeToIndex(ServiceSampleType::Slope)] = 2;
    ServiceIndices[ServiceSampleTypeToIndex(ServiceSampleType::Aspect)] = 2;
    ServiceIndices[ServiceSampleTypeToIndex(ServiceSampleType::Temperature)] = 10;
    ServiceIndices[ServiceSampleTypeToIndex(ServiceSampleType::RelativeHumidity)] = 10;
    ServiceIndices[ServiceSampleTypeToIndex(ServiceSampleType::Precipitation)] = 10;
    References.emplace_back(ReferenceCreateFIRMS());
    References.emplace_back(ReferenceCreateEONET());
}

void ServiceManager::Visit(SavepointVisitor& visitor)
{
    visitor(Services, {0, 0, 1});
    visitor(ServiceIndices, {0, 0, 1});
    visitor(References, {0, 0, 1});
    visitor(ReferenceIndex, {0, 0, 1});
    visitor(Database, {0, 0, 1});
    visitor(TileResolution, {0, 0, 1});
    visitor(TimeResolution, {0, 0, 1});
}

std::unique_ptr<Reference>& ServiceManager::GetReference()
{
    return References[ReferenceIndex];
}

ServiceSampleTypeValue ServiceManager::GetValue(ServiceSampleType type, const glm::dvec2& latLong, float time) const
{
    return Context.GetValue(type, latLong, time);
}

ServiceSampleTypeValue ServiceManager::GetValue(ServiceSampleType type, int x, int y, float time) const
{
    return Context.GetValue(type, x, y, time);
}

glm::ivec2 ServiceManager::GetSize(ServiceSampleType type) const
{
    return Context.GetSize(type);
}

ImTextureRef ServiceManager::GetTextureRef(ServiceSampleType type) const
{
    return Context.GetTextureRef(type);
}

void ServiceManager::RenderImGui(Worker& worker)
{
    ImGui::BeginDisabled(worker.IsRunning());
    Database.RenderImGui();
    ImGui::InputFloat("Tile Resolution (Degrees)", &TileResolution);
    ImGui::InputFloat("Time Resolution (Hours)", &TimeResolution);
    if (ImGui::BeginCombo("Reference", References[ReferenceIndex]->GetDisplayName()))
    {
        for (int i = 0; i < References.size(); i++)
        {
            if (ImGui::Selectable(References[i]->GetDisplayName(), ReferenceIndex == i))
            {
                ReferenceIndex = i;
            }
        }
        ImGui::EndCombo();
    }
    for (uint32_t typeIndex = 0; typeIndex < ServiceIndices.size(); typeIndex++)
    {
        ServiceSampleType type = ServiceSampleTypeFromIndex(typeIndex);
        int& index = ServiceIndices[typeIndex];
        if (ImGui::BeginCombo(ServiceSampleTypeToString(type), Services[index]->GetDisplayName()))
        {
            for (int i = 0; i < Services.size(); i++)
            {
                if ((Services[i]->GetSupportedTypes() & type) == ServiceSampleType{})
                {
                    continue;
                }
                std::string selectableLabel = std::format("{}##{}",
                    Services[i]->GetDisplayName(),
                    ServiceSampleTypeToString(type));
                if (ImGui::Selectable(selectableLabel.c_str(), index == i))
                {
                    index = i;
                }
            }
            ImGui::EndCombo();
        }
    }
    for (std::unique_ptr<Service>& service : Services)
    {
        if (ImGui::TreeNode(service->GetDisplayName()))
        {
            service->RenderImGui(Context);
            ImGui::TreePop();
        }
    }
    ImGui::EndDisabled();
}

void ServiceManager::Download(Worker& worker)
{
    Context.Clear();
    ServiceIndicesToTypes.clear();
    for (uint32_t typeIndex = 0; typeIndex < ServiceIndices.size(); typeIndex++)
    {
        ServiceSampleType type = ServiceSampleTypeFromIndex(typeIndex);
        int index = ServiceIndices[typeIndex];
        ServiceIndicesToTypes[index] |= type;
    }
    worker.Submit([this, downloads = ServiceIndicesToTypes]()
    {
        for (const auto& [index, types] : downloads)
        {
            Services[index]->Download(
                Context,
                types,
                Database.GetMinLatLong(),
                Database.GetMaxLatLong(),
                TileResolution,
                TimeResolution,
                Database.GetStartDate(),
                Database.GetEndDate(),
                SDL_GetBasePath());
        }
        Context.PostDownload();
    });
}

Future<FireResults> ServiceManager::Simulate(Worker& worker, ankerl::unordered_dense::set<glm::ivec2> selected)
{
    return worker.Submit([this, selected = std::move(selected)]()
    {
        FireResults results;
        const auto getStaticPixel = [this](ServiceSampleType type) -> std::function<float(int, int)>
        {
            return [this, type](int x, int y)
            {
                return Context.GetValue(type, x, y, 0.0f).F32;
            };
        };
        const auto getDynamicPixel = [this](ServiceSampleType type) -> std::function<float(int, int, float)>
        {
            return [this, type](int x, int y, float time)
            {
                return Context.GetValue(type, x, y, time).F32;
            };
        };
        glm::ivec2 size = Context.GetSize(ServiceSampleType::FuelModel);
        glm::dvec2 min = Database.GetMinLatLong();
        glm::dvec2 max = Database.GetMaxLatLong();
        SDL_assert(size.x > 0 && size.y > 0);
        Simulator.SetSize(size.x, size.y);
        Simulator.SetResolution(TileResolution * kMathMetersPerDegree);
        Simulator.SetIgniting([selected = std::move(selected)](int x, int y)
        {
            return selected.contains(glm::ivec2{x, y});
        });
        Simulator.SetFuelModel([this](int x, int y)
        {
            return FireFuelModelType(Context.GetValue(ServiceSampleType::FuelModel, x, y, 0.0f).U32);
        });
        Simulator.SetLongitude([min, max, size](int x, int)
        {
            return min.y + (x + 0.5) / size.x * (max.y - min.y);
        });
        Simulator.SetLatitude([min, max, size](int, int y)
        {
            return max.x - (y + 0.5) / size.y * (max.x - min.x);
        });
        Simulator.SetElevation(getStaticPixel(ServiceSampleType::Elevation));
        Simulator.SetSlope(getStaticPixel(ServiceSampleType::Slope));
        Simulator.SetAspect(getStaticPixel(ServiceSampleType::Aspect));
        Simulator.SetCanopyCover(getStaticPixel(ServiceSampleType::CanopyCover));
        Simulator.SetCanopyHeight(getStaticPixel(ServiceSampleType::CanopyHeight));
        Simulator.SetCrownRatio(getStaticPixel(ServiceSampleType::CrownRatio));
        Simulator.SetWindSpeed(getDynamicPixel(ServiceSampleType::WindSpeed));
        Simulator.SetWindDirection(getDynamicPixel(ServiceSampleType::WindDirection));
        Simulator.SetMoistureOneHour(getDynamicPixel(ServiceSampleType::MoistureOneHour));
        Simulator.SetMoistureTenHour(getDynamicPixel(ServiceSampleType::MoistureTenHour));
        Simulator.SetMoistureHundredHour(getDynamicPixel(ServiceSampleType::MoistureHundredHour));
        Simulator.SetMoistureLiveHerbaceous(getDynamicPixel(ServiceSampleType::MoistureLiveHerbaceous));
        Simulator.SetMoistureLiveWoody(getDynamicPixel(ServiceSampleType::MoistureLiveWoody));
        Simulator.SetOutPath(kSavepointPath.string());
        if (!Simulator.Simulate())
        {
            return results;
        }
        results.Load(kSavepointPath.string(), size);
        return results;
    });
}

Future<FireResults> ServiceManager::Fetch(Worker& worker)
{
    return GetReference()->Fetch(worker, Database, TileResolution);
}
