#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <ankerl/unordered_dense.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <savepoint/savepoint.hpp>
#include <spdlog/spdlog.h>

#include <memory>
#include <optional>

#include "console.hpp"
#include "fire_model.hpp"
#include "fire_results.hpp"
#include "fire_simulator.hpp"
#include "future.hpp"
#include "image_viewer.hpp"
#include "service_manager.hpp"
#include "worker.hpp"
#include "version.hpp"

static const std::filesystem::path kBasePath = SDL_GetBasePath();
static const std::filesystem::path kSavepointPath = kBasePath / "fire_simulator.sqlite3";

static SDL_Window* window;
static SDL_Renderer* renderer;
static Savepoint savepoint;
static Console console;
static Worker worker;
static ServiceManager serviceManager;
static ImageViewer imageViewer;
static std::optional<FireResults> results;
static std::optional<FireResults> reference;
static Future<FireResults> pendingResults;
static Future<FireResults> pendingReference;

static bool Init()
{
    SDL_SetAppMetadata("Fire Simulator", nullptr, nullptr);
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
        return false;
    }
    if (!SDL_CreateWindowAndRenderer("Fire Simulator", 960, 720, SDL_WINDOW_RESIZABLE, &window, &renderer))
    {
        spdlog::error("Failed to create window and renderer: {}", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(renderer, 1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    if (savepoint.Open(SavepointDriver::SQLite3, kSavepointPath.string(), kVersion) == SavepointStatus::Existing)
    {
        spdlog::info("Loading save");
        savepoint.Read(serviceManager);
    }
    return true;
}

static void Quit()
{
    serviceManager = {};
    savepoint.Close();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void Tick()
{
    if (pendingResults.IsReady())
    {
        results = pendingResults.Get();
    }
    if (pendingReference.IsReady())
    {
        reference = pendingReference.Get();
    }
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    if (ImGui::Begin("General"))
    {
        serviceManager.RenderImGui(worker);
        ImGui::BeginDisabled(worker.IsRunning());
        if (ImGui::Button("Download Maps"))
        {
            serviceManager.Download(worker);
        }
        if (ImGui::Button("Download References"))
        {
            pendingReference = serviceManager.Fetch(worker);
        }
        if (ImGui::Button("Simulate"))
        {
            serviceManager.Download(worker);
            pendingResults = serviceManager.Simulate(worker, imageViewer.GetSelected());
        }
        if (ImGui::Button("Save"))
        {
            savepoint.Write(serviceManager);
            savepoint.Save();
        }
        ImGui::EndDisabled();
    }
    ImGui::End();
    if (ImGui::Begin("Image"))
    {
        imageViewer.Draw(serviceManager, results, reference);
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
    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            }
        }
        Tick();
    }
    Quit();
    return 0;
}
