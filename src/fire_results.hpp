#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

#include <filesystem>
#include <vector>

class FireResults
{
public:
    FireResults();
    void Load(const std::filesystem::path& path, glm::ivec2 size);
    void Update(float time);
    float GetMaxTime() const;
    ImTextureData* GetTexture();

private:
    int Width;
    int Height;
    float MaxTime;
    std::vector<float> Ignitions;
    std::vector<float> Burns;
    ImTextureData* Texture;
};
