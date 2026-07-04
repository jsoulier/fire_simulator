#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <ankerl/unordered_dense.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "service.hpp"

struct State
{
    State()
        : MinLatLong{45.30, -75.85}
        , MaxLatLong{45.50, -75.55}
        , Resolution{0.001f}
        , ImageDebugSampleType{ServiceSampleType::FuelModel}
    {
        Services.emplace_back(ServiceCreateESAWorldCover());
        Services.emplace_back(ServiceCreateOpenTopography());
        ServiceIndices[ServiceSampleType::FuelModel] = 0;
        ServiceIndices[ServiceSampleType::Elevation] = 1;
        ServiceIndices[ServiceSampleType::Slope] = 1;
        ServiceIndices[ServiceSampleType::Aspect] = 1;
    }

    glm::dvec2 MinLatLong;
    glm::dvec2 MaxLatLong;
    double Resolution;
    ServiceSampleType ImageDebugSampleType;
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
    ImGui::SeparatorText("Services");
    ImGui::InputDouble("Min Latitude", &state.MinLatLong.x);
    ImGui::InputDouble("Min Longitude", &state.MinLatLong.y);
    ImGui::InputDouble("Max Latitude", &state.MaxLatLong.x);
    ImGui::InputDouble("Max Longitude", &state.MaxLatLong.y);
    ImGui::InputDouble("Resolution (Degrees)", &state.Resolution);
    for (auto& [type, index] : state.ServiceIndices)
    {
        if (ImGui::BeginCombo(ServiceSampleTypeToString(type), state.Services[index]->GetName().c_str()))
        {
            for (int i = 0; i < int(state.Services.size()); i++)
            {
                if ((state.Services[i]->GetSupportedTypes() & type) == ServiceSampleType{})
                {
                    continue;
                }
                const bool selected = index == i;
                if (ImGui::Selectable(state.Services[i]->GetName().c_str(), selected))
                {
                    index = i;
                }
            }
            ImGui::EndCombo();
        }
    }
    if (ImGui::Button("Download"))
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
    if (ImGui::Begin("Image Debug"))
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
        if (auto it = state.ServiceIndices.find(state.ImageDebugSampleType); it != state.ServiceIndices.end())
        {
            Service* service = state.Services[it->second].get();
            ImTextureRef texture = service->GetTextureRef(state.ImageDebugSampleType);
            if (texture.GetTexID() != ImTextureID_Invalid)
            {
                const ImVec2 position = ImGui::GetCursorScreenPos();
                const ImVec2 available = ImGui::GetContentRegionAvail();
                float width = float(texture._TexData->Width);
                float height = float(texture._TexData->Height);
                float scale = std::min(available.x / width, available.y / height);
                ImGui::Image(texture, ImVec2(width * scale, height * scale));
                if (ImGui::IsItemHovered())
                {
                    ImVec2 mouse = ImGui::GetMousePos();
                    int x = (mouse.x - position.x) / scale;
                    int y = (mouse.y - position.y) / scale;
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
            }
        }
        else
        {
            ImGui::TextUnformatted("Unloaded");
        }
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
