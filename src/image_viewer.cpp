#include <glm/glm.hpp>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <memory>

#include "image_viewer.hpp"

ImageViewer::ImageViewer()
    : Type{ServiceSampleType::FuelModel}
    , Size{0, 0}
    , Selected{0, 0}
    , Zoom{1.0f}
    , Pan{0.0f, 0.0f}
{
}

void ImageViewer::Draw(std::unique_ptr<Service>& service, ServiceSampleType type, ImTextureRef overlay)
{
    ImTextureRef texture = service->GetTextureRef(type);
    glm::ivec2 size = service->GetSize(type);
    if (texture.GetTexID() == ImTextureID_Invalid || size.x <= 0 || size.y <= 0)
    {
        return;
    }
    float width = size.x;
    float height = size.y;
    if (type != Type || size != Size)
    {
        Type = type;
        Size = size;
        Zoom = 1.0f;
        Pan = ImVec2(0.0f, 0.0f);
    }
    ImVec2 canvasMin = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x = std::max(canvasSize.x, 1.0f);
    canvasSize.y = std::max(canvasSize.y, 1.0f);
    ImVec2 canvasMax(canvasMin.x + canvasSize.x, canvasMin.y + canvasSize.y);
    ImGui::InvisibleButton("canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    bool isHovered = ImGui::IsItemHovered();
    const ImGuiIO& io = ImGui::GetIO();
    if (isHovered && io.MouseWheel != 0.0f)
    {
        float speed = std::pow(1.1f, io.MouseWheel);
        ImVec2 topLeft(canvasMin.x + Pan.x, canvasMin.y + Pan.y);
        Pan.x = io.MousePos.x - canvasMin.x - (io.MousePos.x - topLeft.x) * speed;
        Pan.y = io.MousePos.y - canvasMin.y - (io.MousePos.y - topLeft.y) * speed;
        Zoom *= speed;
    }
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        Pan.x += io.MouseDelta.x;
        Pan.y += io.MouseDelta.y;
    }
    float scale = std::min(canvasSize.x / width, canvasSize.y / height) * Zoom;
    ImVec2 topLeft(canvasMin.x + Pan.x, canvasMin.y + Pan.y);
    ImVec2 bottomRight(topLeft.x + width * scale, topLeft.y + height * scale);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(canvasMin, canvasMax, true);
    drawList->AddImage(texture, topLeft, bottomRight);
    if (overlay.GetTexID() != ImTextureID_Invalid)
    {
        drawList->AddImage(overlay, topLeft, bottomRight);
    }
    if (isHovered)
    {
        int x = (io.MousePos.x - topLeft.x) / scale;
        int y = (io.MousePos.y - topLeft.y) / scale;
        if (x >= 0 && x < size.x && y >= 0 && y < size.y)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                Selected = {x, y};
            }
            ServicePixel pixel = service->GetPixel(type, x, y);
            if (ServiceSampleTypeToPixelType(type) == ServicePixelType::U32)
            {
                ImGui::SetTooltip("%u", pixel.U32);
            }
            else
            {
                ImGui::SetTooltip("%.3f", pixel.F32);
            }
        }
    }
    if (Selected.x >= 0 && Selected.x < size.x && Selected.y >= 0 && Selected.y < size.y)
    {
        ImVec2 selected(topLeft.x + (Selected.x + 0.5f) * scale, topLeft.y + (Selected.y + 0.5f) * scale);
        drawList->AddCircleFilled(selected, 4.0f, IM_COL32(255, 0, 0, 255));
    }
    drawList->PopClipRect();
}

ankerl::unordered_dense::set<glm::ivec2> ImageViewer::GetSelected() const
{
    return {Selected};
}
