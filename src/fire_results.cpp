#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include "fire_model.hpp"
#include "fire_results.hpp"

FireResults::FireResults()
    : Width{0}
    , Height{0}
    , MaxTime{0.0f}
    , Texture{nullptr}
{
}

void FireResults::Load(const std::filesystem::path& path, glm::ivec2 size)
{
    std::ifstream file(path);
    if (!file)
    {
        spdlog::error("Failed to open results {}", path.string());
        return;
    }
    SDL_assert(size.x > 0 && size.y > 0);
    Width = size.x;
    Height = size.y;
    MaxTime = 0.0f;
    Ignitions.assign(size_t(Width) * Height, std::numeric_limits<float>::infinity());
    Burns.assign(size_t(Width) * Height, std::numeric_limits<float>::infinity());
    std::string line;
    std::getline(file, line); 
    while (std::getline(file, line))
    {
        float time;
        int x;
        int y;
        int status;
        if (std::sscanf(line.c_str(), "%f,%d,%d,%*f,%*f,%*f,%*f,%d", &time, &x, &y, &status) != 4)
        {
            continue;
        }
        if (x < 0 || x >= Width || y < 0 || y >= Height)
        {
            continue;
        }
        size_t index = size_t(y) * Width + x;
        if (status == int(FireCellStatus::Igniting) || status == int(FireCellStatus::Burning))
        {
            Ignitions[index] = std::min(Ignitions[index], time);
        }
        else if (status == int(FireCellStatus::Burnt))
        {
            Burns[index] = std::min(Burns[index], time);
        }
        MaxTime = std::max(MaxTime, time);
    }
    Texture = IM_NEW(ImTextureData)();
    Texture->Create(ImTextureFormat_RGBA32, Width, Height);
    ImGui::RegisterUserTexture(Texture);
}

void FireResults::Update(float time)
{
    if (!Texture)
    {
        return;
    }
    uint32_t* texels = static_cast<uint32_t*>(Texture->GetPixels());
    for (int i = 0; i < Width * Height; i++)
    {
        uint32_t color = 0;
        if (time >= Ignitions[i])
        {
            if (time < Burns[i])
            {
                float range = Burns[i] - Ignitions[i];
                float alpha = 0.0f;
                if (std::isfinite(range) && range > 0.0f)
                {
                    alpha = std::clamp((time - Ignitions[i]) / range, 0.0f, 1.0f);
                }
                uint8_t g = uint8_t(230.0f - 200.0f * alpha);
                uint8_t b = uint8_t(60.0f - 60.0f * alpha);
                color = IM_COL32(255, g, b, 220);
            }
            else
            {
                color = IM_COL32(120, 55, 25, 200);
            }
        }
        texels[i] = color;
    }
    ImTextureDataQueueUpload(Texture, 0, 0, Width, Height);
}

float FireResults::GetMaxTime() const
{
    return MaxTime;
}

ImTextureData* FireResults::GetTexture()
{
    return Texture;
}
