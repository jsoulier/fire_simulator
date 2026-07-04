#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <ankerl/unordered_dense.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <utility>
#include <vector>

#include "service.hpp"

struct State
{
    State()
        : MinLatLong{49.20, -123.20}
        , MaxLatLong{49.35, -123.00}
        , Resolution{0.001f}
    {
        Services.emplace_back(ServiceCreateESAWorldCover());
        ServiceIndices[ServiceSampleType::FuelModel] = 0;
    }

    glm::dvec2 MinLatLong;
    glm::dvec2 MaxLatLong;
    double Resolution;
    std::vector<std::unique_ptr<Service>> Services;
    ankerl::unordered_dense::map<ServiceSampleType, int> ServiceIndices;
};

static SDL_Window* window;
static SDL_Renderer* renderer;
static State state;

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

static void Tick()
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::SeparatorText("GeoTIFFs");
        ImGui::InputDouble("Min Latitude", &state.MinLatLong.x);
        ImGui::InputDouble("Min Longitude", &state.MinLatLong.y);
        ImGui::InputDouble("Max Latitude", &state.MaxLatLong.x);
        ImGui::InputDouble("Max Longitude", &state.MaxLatLong.y);
        ImGui::InputDouble("Resolution (Degrees)", &state.Resolution);
        for (auto& [type, index] : state.ServiceIndices)
        {

        }
        if (ImGui::Button("Download"))
        {

        }
    }

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
