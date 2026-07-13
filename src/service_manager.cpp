#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include <filesystem>
#include <memory>

#include "service_manager.hpp"

static const std::filesystem::path kBasePath = SDL_GetBasePath();
static const std::filesystem::path kFireSimulatorPath = kBasePath / "fire_simulator_results.csv";
static constexpr double kMetresPerDegree = 111320.0;

ServiceManager::ServiceManager()
    : TileResolution{0.001f}
    , TimeResolution{1.0f}
{
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
    for (int i = 0; i < 32; i++)
    {
        const ServiceSampleType type = ServiceSampleType(1 << i);
        if ((ServiceSampleType::All & type) != ServiceSampleType{})
        {
            ServiceIndices[type] = 3;
        }
    }
    ServiceIndices[ServiceSampleType::FuelModel] = 0;
    ServiceIndices[ServiceSampleType::Elevation] = 2;
    ServiceIndices[ServiceSampleType::Slope] = 2;
    ServiceIndices[ServiceSampleType::Aspect] = 2;
    References.emplace_back(ReferenceCreateFIRMS());
    References.emplace_back(ReferenceCreateEONET());
}

std::unique_ptr<Service>& ServiceManager::GetService(ServiceSampleType type)
{
    return Services[ServiceIndices.at(type)];
}

std::unique_ptr<Reference>& ServiceManager::GetReference()
{
    return References[ReferenceIndex];
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
    for (auto& [type, index] : ServiceIndices)
    {
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
            service->RenderImGui();
            ImGui::TreePop();
        }
    }
    ImGui::EndDisabled();
}

void ServiceManager::Download(Worker& worker)
{
    ServiceIndicesToTypes.clear();
    for (const auto& [type, index] : ServiceIndices)
    {
        ServiceIndicesToTypes[index] |= type;
    }
    for (const auto& [index, types] : ServiceIndicesToTypes)
    {
        worker.Submit([&]()
        {
            Services[index]->Download(
                types,
                Database.GetMinLatLong(),
                Database.GetMaxLatLong(),
                TileResolution,
                TimeResolution,
                Database.GetStartDate(),
                Database.GetEndDate());
        });
    }
}

Future<FireResults> ServiceManager::Simulate(Worker& worker, FireSimulatorParams& params)
{
    return worker.Submit([this, inParams = params]()
    {
        FireResults results;
        FireSimulatorParams params = inParams;
        const auto getStaticPixel = [this](ServiceSampleType type) -> std::function<float(int, int)>
        {
            const Service* service = Services[ServiceIndices.at(type)].get();
            return [service, type](int x, int y)
            {
                return service->GetValue(type, x, y, 0.0f).F32;
            };
        };
        const auto getDynamicPixel = [this](ServiceSampleType type) -> std::function<float(int, int, float)>
        {
            const Service* service = Services[ServiceIndices.at(type)].get();
            return [service, type](int x, int y, float time)
            {
                return service->GetValue(type, x, y, time).F32;
            };
        };
        const Service* fuelService = Services[ServiceIndices.at(ServiceSampleType::FuelModel)].get();
        glm::ivec2 size = fuelService->GetSize(ServiceSampleType::FuelModel);
        glm::dvec2 min = Database.GetMinLatLong();
        glm::dvec2 max = Database.GetMaxLatLong();
        SDL_assert(size.x > 0 && size.y > 0);
        params.Width = size.x;
        params.Height = size.y;
        params.Resolution = TileResolution * kMetresPerDegree;
        params.FuelModel = [fuelService](int x, int y)
        {
            return FireFuelModelType(fuelService->GetValue(ServiceSampleType::FuelModel, x, y, 0.0f).U32);
        };
        params.Longitude = [min, max, size](int x, int)
        {
            return min.y + (x + 0.5) / size.x * (max.y - min.y);
        };
        params.Latitude = [min, max, size](int, int y)
        {
            return max.x - (y + 0.5) / size.y * (max.x - min.x);
        };
        params.Elevation = getStaticPixel(ServiceSampleType::Elevation);
        params.Slope = getStaticPixel(ServiceSampleType::Slope);
        params.Aspect = getStaticPixel(ServiceSampleType::Aspect);
        params.CanopyCover = getStaticPixel(ServiceSampleType::CanopyCover);
        params.CanopyHeight = getStaticPixel(ServiceSampleType::CanopyHeight);
        params.CrownRatio = getStaticPixel(ServiceSampleType::CrownRatio);
        params.WindSpeed = getDynamicPixel(ServiceSampleType::WindSpeed);
        params.WindDirection = getDynamicPixel(ServiceSampleType::WindDirection);
        params.MoistureOneHour = getDynamicPixel(ServiceSampleType::MoistureOneHour);
        params.MoistureTenHour = getDynamicPixel(ServiceSampleType::MoistureTenHour);
        params.MoistureHundredHour = getDynamicPixel(ServiceSampleType::MoistureHundredHour);
        params.MoistureLiveHerbaceous = getDynamicPixel(ServiceSampleType::MoistureLiveHerbaceous);
        params.MoistureLiveWoody = getDynamicPixel(ServiceSampleType::MoistureLiveWoody);
        params.OutPath = kFireSimulatorPath.string();
        if (!FireSimulatorRun(params))
        {
            return results;
        }
        results.Load(params.OutPath, {params.Width, params.Height});
        return results;
    });
}

Future<FireResults> ServiceManager::Fetch(Worker& worker)
{
    return GetReference()->Fetch(worker, Database, TileResolution);
}
