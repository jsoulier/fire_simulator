#pragma once

#include <glm/glm.hpp>
#include <imgui.h>
#include <savepoint/savepoint.hpp>

#include <string_view>
#include <ctime>
#include <vector>

#include "date.hpp"

class ReferenceDatabase
{
public:
    ReferenceDatabase();
    void Visit(SavepointVisitor& visitor);
    void RenderImGui();
    const glm::dvec2& GetMinLatLong() const;
    const glm::dvec2& GetMaxLatLong() const;
    Date GetStartDate() const;
    Date GetEndDate() const;

private:
    struct Fire
    {
        std::string_view Name;
        double Latitude;
        double Longitude;
        double Margin;
        Date Start;
        Date End;
    };

    void Select(const Fire& fire);

    glm::dvec2 MinLatLong;
    glm::dvec2 MaxLatLong;
    Date StartDate;
    Date EndDate;
    ImGuiTextFilter Filter;
    std::vector<Fire> Fires;
};
