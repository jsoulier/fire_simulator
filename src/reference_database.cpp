#include <ImGuiDatePicker.hpp>
#include <geo_names_imgui.hpp>
#include <glm/glm.hpp>
#include <imgui.h>

#include <optional>

#include "reference_database.hpp"

ReferenceDatabase::ReferenceDatabase()
    : MinLatLong{45.30, -75.85}
    , MaxLatLong{45.50, -75.55}
{
    // TODO: create offline database similar to GeoNames
    StartDate = Date(2021, 7, 13).ToTm();
    EndDate = Date(2021, 7, 23).ToTm();
    Fires = {
        {"Dixie Fire (2021)", 39.877, -121.381, 0.35, Date(2021, 7, 13), Date(2021, 7, 23)},
        {"Caldor Fire (2021)", 38.610, -120.530, 0.40, Date(2021, 8, 14), Date(2021, 8, 24)},
        {"Bootleg Fire (2021)", 42.650, -121.300, 0.40, Date(2021, 7, 6), Date(2021, 7, 16)},
        {"August Complex (2020)", 39.760, -122.670, 0.60, Date(2020, 8, 17), Date(2020, 8, 27)},
        {"Creek Fire (2020)", 37.200, -119.260, 0.40, Date(2020, 9, 4), Date(2020, 9, 14)},
        {"Cameron Peak Fire (2020)", 40.620, -105.880, 0.40, Date(2020, 8, 13), Date(2020, 8, 23)},
        {"Glass Fire (2020)", 38.560, -122.490, 0.20, Date(2020, 9, 27), Date(2020, 10, 7)},
        {"Camp Fire (2018)", 39.810, -121.437, 0.20, Date(2018, 11, 8), Date(2018, 11, 18)},
        {"Carr Fire (2018)", 40.650, -122.620, 0.30, Date(2018, 7, 23), Date(2018, 8, 2)},
        {"Woolsey Fire (2018)", 34.240, -118.750, 0.25, Date(2018, 11, 8), Date(2018, 11, 18)},
        {"Mendocino Complex (2018)", 39.240, -122.820, 0.50, Date(2018, 7, 27), Date(2018, 8, 6)},
        {"Thomas Fire (2017)", 34.420, -119.200, 0.40, Date(2017, 12, 4), Date(2017, 12, 14)},
        {"Tubbs Fire (2017)", 38.610, -122.620, 0.20, Date(2017, 10, 8), Date(2017, 10, 18)},
        {"Fort McMurray Fire (2016)", 56.730, -111.380, 0.50, Date(2016, 5, 1), Date(2016, 5, 11)},
    };
}

void ReferenceDatabase::Select(const Fire& fire)
{
    MinLatLong = {fire.Latitude - fire.Margin, fire.Longitude - fire.Margin};
    MaxLatLong = {fire.Latitude + fire.Margin, fire.Longitude + fire.Margin};
    StartDate = fire.Start.ToTm();
    EndDate = fire.End.ToTm();
}

void ReferenceDatabase::RenderImGui()
{
    if (std::optional<GeoNames> location = GetImGuiGeoNames())
    {
        const glm::dvec2 extent = (MaxLatLong - MinLatLong) * 0.5;
        const glm::dvec2 center{location->Latitude, location->Longitude};
        MinLatLong = center - extent;
        MaxLatLong = center + extent;
    }
    ImGui::InputDouble("Min Latitude", &MinLatLong.x);
    ImGui::InputDouble("Min Longitude", &MinLatLong.y);
    ImGui::InputDouble("Max Latitude", &MaxLatLong.x);
    ImGui::InputDouble("Max Longitude", &MaxLatLong.y);
    ImGui::DatePicker("Start Date", StartDate);
    ImGui::DatePicker("End Date", EndDate);
    Filter.Draw("##ReferenceDatabaseSearch");
    ImGui::BeginChild("##ReferenceDatabaseFires", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6), true);
    for (const Fire& fire : Fires)
    {
        const char* name = fire.Name.data();
        if (Filter.PassFilter(name, name + fire.Name.size()) && ImGui::Selectable(name))
        {
            Select(fire);
        }
    }
    ImGui::EndChild();
}

const glm::dvec2& ReferenceDatabase::GetMinLatLong() const
{
    return MinLatLong;
}

const glm::dvec2& ReferenceDatabase::GetMaxLatLong() const
{
    return MaxLatLong;
}

Date ReferenceDatabase::GetStartDate() const
{
    return Date(StartDate);
}

Date ReferenceDatabase::GetEndDate() const
{
    return Date(EndDate);
}
