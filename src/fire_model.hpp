#pragma once

#include <behaveRun.h>
#include <cadmium/celldevs/grid/cell.hpp>
#include <cadmium/celldevs/grid/config.hpp>
#include <fuelModels.h>
#include <nlohmann/json.hpp>
#include <species_master_table.h>
#include <ankerl/unordered_dense.h>

#include <format>
#include <limits>
#include <memory>
#include <ostream>
#include <unordered_map>

#include "fire_fuel_model.hpp"

enum class FireCellStatus
{
    Unburnt = 0,
    Burning = 1,
    Burnt = 2,
    Igniting = 3
};

NLOHMANN_JSON_SERIALIZE_ENUM(FireCellStatus, {
    {FireCellStatus::Unburnt, 0},
    {FireCellStatus::Burning, 1},
    {FireCellStatus::Burnt, 2},
    {FireCellStatus::Igniting, 3},
})

struct FireState
{
    FireState();

    FireCellStatus Status;
    float BurnTime;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FireState, Status)

bool operator!=(const FireState& lhs, const FireState& rhs);
std::ostream& operator<<(std::ostream& stream, const FireState& state);

class FireModel : public cadmium::celldevs::GridCell<FireState, double>
{
public:
    FireModel(const cadmium::celldevs::coordinates& id, const std::shared_ptr<const cadmium::celldevs::GridCellConfig<FireState, double>>& config, float resolution);
    [[nodiscard]] bool isComplete() const override;
    [[nodiscard]] FireState localComputation(FireState state, const cadmium::celldevs::Neighborhood<cadmium::celldevs::coordinates, FireState, double>& neighborhood) const override;
    [[nodiscard]] double outputDelay(const FireState& state) const override;
    [[nodiscard]] std::string logState() const override;

private:
    friend std::ostream& operator<<(std::ostream& stream, const FireModel& model);
    float GetDirectionSpreadRate(float bearing) const;
    float GetMaxSpreadRate() const;

    float Resolution;
    FireFuelModelType FuelModel;
    float Slope;
    float Aspect;
    double Longitude;
    double Latitude;
    float Height;
    float WindSpeed;
    float WindDirection;
    float CanopyCover;
    float CanopyHeight;
    float CrownRatio;
    float Moisture1Hour;
    float Moisture10Hour;
    float Moisture100Hour;
    float MoistureLiveHerbaceous;
    float MoistureLiveWoody;
};
