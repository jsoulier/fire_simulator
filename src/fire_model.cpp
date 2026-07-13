#include <behaveRun.h>
#include <fuelModels.h>
#include <species_master_table.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>
#include <sstream>
#include <string>
#include <utility>

#include "fire_fuel_model.hpp"
#include "fire_model.hpp"
#include "fire_profile.hpp"

static constexpr float kInfinity = std::numeric_limits<float>::infinity();

FireState::FireState()
    : Status(FireCellStatus::Unburnt)
    , BurnTime(kInfinity)
{
}

bool operator!=(const FireState& lhs, const FireState& rhs)
{
    return lhs.Status != rhs.Status || lhs.BurnTime != rhs.BurnTime;
}

std::ostream& operator<<(std::ostream& stream, const FireState& state)
{
    return stream << int(state.Status);
}

FireModel::FireModel(
    const cadmium::celldevs::coordinates& id,
    const std::shared_ptr<const cadmium::celldevs::GridCellConfig<FireState, double>>& config,
    float resolution,
    std::function<float(int, int, float)> windSpeed,
    std::function<float(int, int, float)> windDirection,
    std::function<float(int, int, float)> moistureOneHour,
    std::function<float(int, int, float)> moistureTenHour,
    std::function<float(int, int, float)> moistureHundredHour,
    std::function<float(int, int, float)> moistureLiveHerbaceous,
    std::function<float(int, int, float)> moistureLiveWoody)
    : cadmium::celldevs::GridCell<FireState, double>(id, config)
    , Resolution{resolution}
    , FuelModel{0}
    , Slope{0.0f}
    , Aspect{0.0f}
    , Longitude{0.0}
    , Latitude{0.0}
    , Height{0.0f}
    , WindSpeed{std::move(windSpeed)}
    , WindDirection{std::move(windDirection)}
    , MoistureOneHour{std::move(moistureOneHour)}
    , MoistureTenHour{std::move(moistureTenHour)}
    , MoistureHundredHour{std::move(moistureHundredHour)}
    , MoistureLiveHerbaceous{std::move(moistureLiveHerbaceous)}
    , MoistureLiveWoody{std::move(moistureLiveWoody)}
    , CanopyCover{0.0f}
    , CanopyHeight{0.0f}
    , CrownRatio{0.0f}
{
    const nlohmann::json& cellConfig = config->rawCellConfig;
    assert(cellConfig.is_object());
    FuelModel = cellConfig.value("FuelModel", FuelModel);
    Slope = cellConfig.value("Slope", Slope);
    Aspect = cellConfig.value("Aspect", Aspect);
    Longitude = cellConfig.value("Longitude", Longitude);
    Latitude = cellConfig.value("Latitude", Latitude);
    Height = cellConfig.value("Height", Height);
    CanopyCover = cellConfig.value("CanopyCover", CanopyCover);
    CanopyHeight = cellConfig.value("CanopyHeight", CanopyHeight);
    CrownRatio = cellConfig.value("CrownRatio", CrownRatio);
}

std::ostream& operator<<(std::ostream& stream, const FireModel& model)
{
    return stream << std::format("{:.8f},{:.8f},{:.2f},{:.2f},", model.Longitude, model.Latitude, model.Height, model.Resolution);
}

bool FireModel::isComplete() const
{
    return state.Status == FireCellStatus::Burnt;
}

float FireModel::GetTime() const
{
    return clock / 60.0;
}

FireState FireModel::localComputation(FireState state, const cadmium::celldevs::Neighborhood<cadmium::celldevs::coordinates, FireState, double>& neighborhood) const
{
    FireProfileTag();
    if (state.Status == FireCellStatus::Burnt)
    {
        return state;
    }
    if (state.Status == FireCellStatus::Burning)
    {
        // if the burning cell's state was broadcasted back to the current cell, it's finished burning
        for (const auto& [neighborId, neighbor] : neighborhood)
        {
            if (neighborId == id)
            {
                if (neighbor.state != nullptr && neighbor.state->Status == FireCellStatus::Burning)
                {
                    state.Status = FireCellStatus::Burnt;
                }
                break;
            }
        }
        return state;
    }
    if (!FireFuelModelTypeIsBurnable(FuelModel))
    {
        return state;
    }
    float burnTime = kInfinity;
    if (state.Status == FireCellStatus::Igniting)
    {
        float headRate = GetMaxSpreadRate();
        if (headRate > 0.0f)
        {
            burnTime = Resolution / headRate;
        }
    }
    else
    {
        FireProfileTagBlock("Neighborhood");
        for (const auto& [neighborId, neighbor] : neighborhood)
        {
            if (neighborId == id || neighbor.state == nullptr)
            {
                continue;
            }
            if (neighbor.state->Status != FireCellStatus::Burning)
            {
                continue;
            }
            int deltaE = id.x - neighborId.x;
            int deltaS = id.y - neighborId.y;
            double bearing = std::atan2(double(deltaE), double(-deltaS)) * 180.0 / std::numbers::pi;
            if (bearing < 0.0)
            {
                bearing += 360.0;
            }
            float spreadRate = GetDirectionalSpreadRate(float(bearing));
            if (spreadRate <= 0.0f)
            {
                continue;
            }
            float distance = Resolution * float(std::hypot(deltaE, deltaS));
            burnTime = std::min(burnTime, distance / spreadRate);
        }
    }
    if (burnTime != kInfinity)
    {
        state.Status = FireCellStatus::Burning;
        state.BurnTime = burnTime;
    }
    return state;
}

double FireModel::outputDelay(const FireState& state) const
{
    if (state.Status == FireCellStatus::Burning)
    {
        return state.BurnTime;
    }
    else
    {
        return kInfinity;
    }
}

std::string FireModel::logState() const
{
    thread_local std::stringstream stream;
    stream.str("");
    stream.clear();
    stream << *this << state;
    return stream.str();
}

BehaveRun& FireModel::GetBehaveRun() const
{
    thread_local FuelModels fuelModels;
    thread_local SpeciesMasterTable speciesTable;
    thread_local BehaveRun behave(fuelModels, speciesTable);
    float time = GetTime();
    float moistureOneHour = MoistureOneHour(id.x, id.y, time);
    float moistureTenHour = MoistureTenHour(id.x, id.y, time);
    float moistureHundredHour = MoistureHundredHour(id.x, id.y, time);
    float moistureLiveHerbaceous = MoistureLiveHerbaceous(id.x, id.y, time);
    float moistureLiveWoody = MoistureLiveWoody(id.x, id.y, time);
    float windSpeed = WindSpeed(id.x, id.y, time);
    float windDirection = WindDirection(id.x, id.y, time);
    behave.surface.updateSurfaceInputs(
        FuelModel,
        moistureOneHour, moistureTenHour, moistureHundredHour,
        moistureLiveHerbaceous, moistureLiveWoody, FractionUnits::Percent,
        windSpeed, SpeedUnits::MilesPerHour, WindHeightInputMode::TenMeter,
        windDirection, WindAndSpreadOrientationMode::RelativeToNorth,
        Slope, SlopeUnits::Degrees, Aspect,
        CanopyCover, FractionUnits::Fraction,
        CanopyHeight, LengthUnits::Meters,
        CrownRatio, FractionUnits::Fraction);
    return behave;
}

float FireModel::GetMaxSpreadRate() const
{
    BehaveRun& behave = GetBehaveRun();
    behave.surface.doSurfaceRunInDirectionOfMaxSpread();
    return behave.surface.getSpreadRate(SpeedUnits::MetersPerMinute);
}

float FireModel::GetDirectionalSpreadRate(float bearing) const
{
    BehaveRun& behave = GetBehaveRun();
    behave.surface.doSurfaceRunInDirectionOfInterest(double(bearing), SurfaceFireSpreadDirectionMode::FromIgnitionPoint);
    return behave.surface.getSpreadRateInDirectionOfInterest(SpeedUnits::MetersPerMinute);
}
