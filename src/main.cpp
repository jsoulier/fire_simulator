#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <ankerl/unordered_dense.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_internal.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <fstream>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "console.hpp"
#include "fire_model.hpp"
#include "fire_results.hpp"
#include "fire_simulator.hpp"
#include "service.hpp"

static const std::filesystem::path kBasePath = SDL_GetBasePath();
static const std::filesystem::path kFireSimulatorPath = kBasePath / "fire_simulator_results.csv";
static constexpr double kMetresPerDegree = 111320.0;

struct State
{
    State()
        : MinLatLong{45.30, -75.85}
        , MaxLatLong{45.50, -75.55}
        , Resolution{0.001f}
        , CoordinatorType{FireSimulatorCoordinatorType::EventDriven}
        , Ignition{0, 0}
        , ImageDebugSampleType{ServiceSampleType::FuelModel}
    {
        Services.emplace_back(ServiceCreateESAWorldCover());
        Services.emplace_back(ServiceCreateOpenTopography());
        Services.emplace_back(ServiceCreateCustom());
        for (int i = 0; i < 32; i++)
        {
            const ServiceSampleType type = ServiceSampleType(1 << i);
            if ((ServiceSampleType::All & type) != ServiceSampleType{})
            {
                ServiceIndices[type] = 2;
            }
        }
        ServiceIndices[ServiceSampleType::FuelModel] = 0;
        ServiceIndices[ServiceSampleType::Elevation] = 1;
        ServiceIndices[ServiceSampleType::Slope] = 1;
        ServiceIndices[ServiceSampleType::Aspect] = 1;
    }

    std::vector<std::unique_ptr<Service>> Services;
    ankerl::unordered_dense::map<ServiceSampleType, int> ServiceIndices;
    glm::dvec2 MinLatLong;
    glm::dvec2 MaxLatLong;
    double Resolution;
    FireSimulatorCoordinatorType CoordinatorType;
    glm::ivec2 Ignition;
    ServiceSampleType ImageDebugSampleType;
};

static SDL_Window* window;
static SDL_Renderer* renderer;
static Console console;
static State state;
static FireResults results;
static float resultsTime;

static bool Init()
{
    SDL_SetAppMetadata("FireSimulator", nullptr, nullptr);
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
        return false;
    }
    if (!SDL_CreateWindowAndRenderer("fire_simulator", 960, 720, SDL_WINDOW_RESIZABLE, &window, &renderer))
    {
        spdlog::error("Failed to create window and renderer: {}", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(renderer, 1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    return true;
}

static void Quit()
{
    state = {};
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static bool Poll()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            return false;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_F11)
            {
                if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)
                {
                    SDL_SetWindowFullscreen(window, false);
                }
                else
                {
                    SDL_SetWindowFullscreen(window, true);
                }
            }
            break;
        }
    }
    return true;
}

static void Download()
{
    ankerl::unordered_dense::map<int, ServiceSampleType> serviceIndicesToTypes;
    for (const auto& [type, index] : state.ServiceIndices)
    {
        serviceIndicesToTypes[index] |= type;
    }
    for (const auto& [index, types] : serviceIndicesToTypes)
    {
        state.Services[index]->Download(types, state.MinLatLong, state.MaxLatLong, state.Resolution);
    }
}

static void Simulate()
{
    auto getPixel = [](ServiceSampleType type) -> std::function<float(int, int)>
    {
        int index = state.ServiceIndices.at(type);
        Service* service = state.Services[index].get();
        return [service, type](int x, int y)
        {
            return service->GetPixel(type, x, y).F32;
        };
    };
    int fuelIndex = state.ServiceIndices.at(ServiceSampleType::FuelModel);
    std::unique_ptr<Service>& fuelService = state.Services[fuelIndex];
    glm::ivec2 size = fuelService->GetSize(ServiceSampleType::FuelModel);
    SDL_assert(size.x > 0 && size.y > 0);
    FireSimulatorParams params;
    params.Width = size.x;
    params.Height = size.y;
    params.Resolution = float(state.Resolution * kMetresPerDegree);
    params.CoordinatorType = state.CoordinatorType;
    const glm::dvec2 min = state.MinLatLong;
    const glm::dvec2 max = state.MaxLatLong;
    params.Igniting = [ignition = state.Ignition](int x, int y)
    {
        return x == ignition.x && y == ignition.y;
    };
    params.FuelModel = [&fuelService](int x, int y)
    {
        return FireFuelModelType(fuelService->GetPixel(ServiceSampleType::FuelModel, x, y).U32);
    };
    params.Longitude = [min, max, size](int x, int)
    {
        return min.y + (x + 0.5) / size.x * (max.y - min.y);
    };
    params.Latitude = [min, max, size](int, int y)
    {
        return max.x - (y + 0.5) / size.y * (max.x - min.x);
    };
    params.Elevation = getPixel(ServiceSampleType::Elevation);
    params.Slope = getPixel(ServiceSampleType::Slope);
    params.Aspect = getPixel(ServiceSampleType::Aspect);
    params.CanopyCover = getPixel(ServiceSampleType::CanopyCover);
    params.CanopyHeight = getPixel(ServiceSampleType::CanopyHeight);
    params.CrownRatio = getPixel(ServiceSampleType::CrownRatio);
    params.WindSpeed = getPixel(ServiceSampleType::WindSpeed);
    params.WindDirection = getPixel(ServiceSampleType::WindDirection);
    params.MoistureOneHour = getPixel(ServiceSampleType::MoistureOneHour);
    params.MoistureTenHour = getPixel(ServiceSampleType::MoistureTenHour);
    params.MoistureHundredHour = getPixel(ServiceSampleType::MoistureHundredHour);
    params.MoistureLiveHerbaceous = getPixel(ServiceSampleType::MoistureLiveHerbaceous);
    params.MoistureLiveWoody = getPixel(ServiceSampleType::MoistureLiveWoody);
    params.OutPath = kFireSimulatorPath.string();
    FireSimulatorRun(params);
    results.Load(kFireSimulatorPath, size, resultsTime);
}

static void DrawGeneral()
{
    ImGui::InputDouble("Min Latitude", &state.MinLatLong.x);
    ImGui::InputDouble("Min Longitude", &state.MinLatLong.y);
    ImGui::InputDouble("Max Latitude", &state.MaxLatLong.x);
    ImGui::InputDouble("Max Longitude", &state.MaxLatLong.y);
    ImGui::InputDouble("Resolution (Degrees)", &state.Resolution);
    for (auto& [type, index] : state.ServiceIndices)
    {
        if (ImGui::BeginCombo(ServiceSampleTypeToString(type), state.Services[index]->GetDisplayName()))
        {
            for (int i = 0; i < int(state.Services.size()); i++)
            {
                if ((state.Services[i]->GetSupportedTypes() & type) == ServiceSampleType{})
                {
                    continue;
                }
                std::string selectableLabel = std::format("{}##{}",
                    state.Services[i]->GetDisplayName(),
                    ServiceSampleTypeToString(type));
                if (ImGui::Selectable(selectableLabel.c_str(), index == i))
                {
                    index = i;
                }
            }
            ImGui::EndCombo();
        }
    }
    for (std::unique_ptr<Service>& service : state.Services)
    {
        if (ImGui::TreeNode(service->GetDisplayName()))
        {
            service->RenderImGui();
            ImGui::TreePop();
        }
    }
    const char* coordinators[] = { "Event Driven", "Brute Force" };
    int coordinator = int(state.CoordinatorType);
    if (ImGui::Combo("Coordinator", &coordinator, coordinators, SDL_arraysize(coordinators)))
    {
        state.CoordinatorType = FireSimulatorCoordinatorType(coordinator);
    }
    if (ImGui::Button("Download"))
    {
        Download();
    }
    if (ImGui::Button("Simulate"))
    {
        Download();
        Simulate();
    }
}

static void DrawImage()
{
    if (ImGui::BeginCombo("Sample Type", ServiceSampleTypeToString(state.ImageDebugSampleType)))
    {
        for (const auto& [type, index] : state.ServiceIndices)
        {
            if (ImGui::Selectable(ServiceSampleTypeToString(type), state.ImageDebugSampleType == type))
            {
                state.ImageDebugSampleType = type;
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("Load Results"))
    {
        glm::ivec2 size = state.Services[state.ServiceIndices.at(ServiceSampleType::FuelModel)]->GetSize(ServiceSampleType::FuelModel);
        results.Load(kFireSimulatorPath, size, resultsTime);
    }
    if (results.GetMaxTime() > 0.0f)
    {
        if (ImGui::SliderFloat("Time", &resultsTime, 0.0f, results.GetMaxTime()))
        {
            results.Update(resultsTime);
        }
    }
    std::unique_ptr<Service>& service = state.Services[state.ServiceIndices.at(state.ImageDebugSampleType)];
    ImTextureRef texture = service->GetTextureRef(state.ImageDebugSampleType);
    if (texture.GetTexID() != ImTextureID_Invalid)
    {
        const ImVec2 position = ImGui::GetCursorScreenPos();
        const ImVec2 region = ImGui::GetContentRegionAvail();
        float width = float(texture._TexData->Width);
        float height = float(texture._TexData->Height);
        float scale = std::min(region.x / width, region.y / height);
        ImGui::Image(texture, ImVec2(width * scale, height * scale));
        if (results.GetTexture() && results.GetTexture()->GetTexID() != ImTextureID_Invalid)
        {
            ImVec2 end(position.x + width * scale, position.y + height * scale);
            ImGui::GetWindowDrawList()->AddImage(results.GetTexture()->GetTexRef(), position, end);
        }
        if (ImGui::IsItemHovered())
        {
            ImVec2 mouse = ImGui::GetMousePos();
            int x = (mouse.x - position.x) / scale;
            int y = (mouse.y - position.y) / scale;
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                state.Ignition = {x, y};
            }
            ServicePixel pixel = service->GetPixel(state.ImageDebugSampleType, x, y);
            ServicePixelType pixelType = ServiceSampleTypeToPixelType(state.ImageDebugSampleType);
            if (pixelType == ServicePixelType::U32)
            {
                ImGui::SetTooltip("%u", pixel.U32);
            }
            else if (pixelType == ServicePixelType::F32)
            {
                ImGui::SetTooltip("%.3f", pixel.F32);
            }
            else
            {
                SDL_assert(false);
            }
        }
        if (state.Ignition.x >= 0 && state.Ignition.x < int(width) &&
            state.Ignition.y >= 0 && state.Ignition.y < int(height))
        {
            ImVec2 origin(
                position.x + (state.Ignition.x + 0.5f) * scale,
                position.y + (state.Ignition.y + 0.5f) * scale);
            ImGui::GetWindowDrawList()->AddCircleFilled(origin, 4.0f, IM_COL32(255, 0, 0, 255));
        }
    }
}

static void Tick()
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    if (ImGui::Begin("General"))
    {
        DrawGeneral();
    }
    ImGui::End();
    if (ImGui::Begin("Image"))
    {
        DrawImage();
    }
    ImGui::End();
    if (ImGui::Begin("Console"))
    {
        console.RenderImGui();
    }
    ImGui::End();
    ImGui::Render();
    SDL_SetRenderDrawColorFloat(renderer, 0.1f, 0.1f, 0.1f, 1.0f);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

int main(int argc, char** argv)
{
    if (!Init())
    {
        return 1;
    }
    while (true)
    {
        if (!Poll())
        {
            break;
        }
        Tick();
    }
    Quit();
    return 0;
}
