#pragma once

#include <ankerl/unordered_dense.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <imgui.h>

#include <memory>

#include "service.hpp"

class ImageViewer
{
public:
    ImageViewer();
    void Draw(std::unique_ptr<Service>& service, ServiceSampleType type, ImTextureRef overlay);
    ankerl::unordered_dense::set<glm::ivec2> GetSelected() const;

private:
    ServiceSampleType Type;
    glm::ivec2 Size;
    glm::ivec2 Selected;
    float Zoom;
    ImVec2 Pan;
};
