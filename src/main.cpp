#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <ankerl/unordered_dense.h>
#include <geo_names_imgui.hpp>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_internal.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "console.hpp"
#include "fire_model.hpp"
#include "fire_results.hpp"
#include "fire_simulator.hpp"
#include "future.hpp"
#include "image_viewer.hpp"
#include "service_manager.hpp"
#include "worker.hpp"

static SDL_Window* window;
static SDL_Renderer* renderer;
static Console console;
static ServiceManager serviceManager;
static Worker worker;
static std::vector<FireResults> results;
static Future<FireResults> pendingResults;
static int resultsIndex = -1;
static float resultsTimestamp;
static ServiceSampleType imageDebugSampleType = ServiceSampleType::FuelModel;
static glm::ivec2 ignition;
static FireSimulatorCoordinatorType coordinator = FireSimulatorCoordinatorType::EventDriven;
static ImageViewer imageViewer;

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
    serviceManager = {};
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
    serviceManager.Download(worker);
}

static void Simulate()
{
    ankerl::unordered_dense::set<glm::ivec2> igniting = imageViewer.GetSelected();
    FireSimulatorParams params;
    params.CoordinatorType = coordinator;
    params.Igniting = [&igniting](int x, int y)
    {
        return igniting.contains(glm::ivec2{x, y});
    };
    pendingResults = serviceManager.Simulate(worker, params);
}

static void DrawGeneral()
{
    static constexpr int kMaxCoordinators = SDL_arraysize(kFireSimulatorCoordinatorTypeStrings);
    int coordinatorIndex = int(coordinator);
    serviceManager.RenderImGui();
    if (ImGui::Combo("Coordinator", &coordinatorIndex, kFireSimulatorCoordinatorTypeStrings, kMaxCoordinators))
    {
        coordinator = FireSimulatorCoordinatorType(coordinatorIndex);
    }
    ImGui::BeginDisabled(worker.IsRunning());
    if (ImGui::Button("Download"))
    {
        Download();
    }
    if (ImGui::Button("Simulate"))
    {
        Download();
        Simulate();
    }
    ImGui::EndDisabled();
}

static void DrawImage()
{
    static constexpr int kMaxImageTypes = SDL_arraysize(kServiceSampleTypeStrings);
    int debugSampleTypeIndex = ServiceSampleTypeToIndex(imageDebugSampleType);
    if (ImGui::Combo("Image Type", &debugSampleTypeIndex, kServiceSampleTypeStrings, kMaxImageTypes))
    {
        imageDebugSampleType = ServiceSampleTypeFromIndex(debugSampleTypeIndex);
    }
    ImGui::BeginChild("Results", ImVec2(140.0f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
    for (int i = 0; i < int(results.size()); i++)
    {
        std::string label = std::format("Result {}", i);
        if (ImGui::Selectable(label.c_str(), resultsIndex == i))
        {
            resultsIndex = i;
            resultsTimestamp = results[i].GetMaxTime();
            results[i].Update(resultsTimestamp);
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginGroup();
    FireResults* current = nullptr;
    if (resultsIndex >= 0 && resultsIndex < int(results.size()))
    {
        current = &results[resultsIndex];
    }
    if (current)
    {
        if (ImGui::SliderFloat("Time", &resultsTimestamp, 0.0f, current->GetMaxTime()))
        {
            current->Update(resultsTimestamp);
        }
    }
    std::unique_ptr<Service>& service = serviceManager.GetService(imageDebugSampleType);
    ImTextureRef overlay;
    if (current && current->GetTexture() && current->GetTexture()->GetTexID() != ImTextureID_Invalid)
    {
        overlay = current->GetTexture()->GetTexRef();
    }
    imageViewer.Draw(service, imageDebugSampleType, overlay);
    ImGui::EndGroup();
}

static void Tick()
{
    if (pendingResults.IsReady())
    {
        results.push_back(pendingResults.Get());
        resultsIndex = int(results.size()) - 1;
        resultsTimestamp = results[resultsIndex].GetMaxTime();
        results[resultsIndex].Update(resultsTimestamp);
    }
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
