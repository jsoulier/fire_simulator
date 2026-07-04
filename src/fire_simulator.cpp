#include <cadmium/core/simulation/brute_force_root_coordinator.hpp>
#include <cadmium/core/simulation/event_driven_root_coordinator.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <format>
#include <limits>
#include <memory>
#include <print>
#include <string>
#include <utility>

#include "fire_fuel_model.hpp"
#include "fire_grid_coupled.hpp"
#include "fire_model.hpp"
#include "fire_model_logger.hpp"
#include "fire_profile.hpp"
#include "fire_simulator.hpp"

FireSimulatorParams::FireSimulatorParams()
    : Width{0}
    , Height{0}
    , Resolution{30.0}
    , CoordinatorType{FireSimulatorCoordinatorType::EventDriven}
    , FireX{0}
    , FireY{0}
    , FuelModel{[](int, int) { return kFireFuelModelFM4; }}
    , Longitude{[](int, int) { return 0.0f; }}
    , Latitude{[](int, int) { return 0.0f; }}
    , Elevation{[](int, int) { return 0.0f; }}
    , Slope{[](int, int) { return 0.0f; }}
    , Aspect{[](int, int) { return 0.0f; }}
    , CanopyCover{[](int, int) { return 0.0f; }}
    , CanopyHeight{[](int, int) { return 0.0f; }}
    , CrownRatio{[](int, int) { return 0.0f; }}
    , WindSpeed{[](int, int) { return 5.0f; }}
    , WindDirection{[](int, int) { return 0.0f; }}
    , MoistureOneHour{[](int, int) { return 8.0f; }}
    , MoistureTenHour{[](int, int) { return 9.0f; }}
    , MoistureHundredHour{[](int, int) { return 10.0f; }}
    , MoistureLiveHerbaceous{[](int, int) { return 60.0f; }}
    , MoistureLiveWoody{[](int, int) { return 90.0f; }}
{
}

bool FireSimulatorRun(const FireSimulatorParams& params)
{
    spdlog::info("Creating fire simulation parameters");
    int columns = params.Width;
    int rows = params.Height;
    try
    {
        nlohmann::json scenario;
        scenario["scenario"] = {
            {"shape", {columns, rows}},
            {"origin", {0, 0}},
            {"wrapped", false},
            {"neighborhood", {
                {"type", "moore"},
                {"range", 1},
                {"vicinity", 1}
            }}
        };
        std::shared_ptr<cadmium::celldevs::GridCellDEVSCoupled<FireState, double>> model = std::make_shared<FireGridCoupled>(
            "fire",
            [resolution = params.Resolution]
            (const auto& id, const auto& config)
            {
                return std::make_shared<FireModel>(id, config, resolution);
            },
            std::move(scenario));
        {
            FireProfileTagBlock("Providers");
            nlohmann::json neighborhood = nlohmann::json::array({{
                {"type", "moore"},
                {"range", 1},
                {"vicinity", 1}
            }});
            for (int y = 0; y < rows; y++)
            {
                for (int x = 0; x < columns; x++)
                {
                    FireFuelModelType fuelModel = params.FuelModel(x, y);
                    bool igniting = x == params.FireX && y == params.FireY;
                    if (!FireFuelModelTypeIsBurnable(fuelModel) && !igniting)
                    {
                        continue;
                    }
                    nlohmann::json config = {
                        {"cell_map", nlohmann::json::array({nlohmann::json::array({x, y})})},
                        {"neighborhood", neighborhood},
                        {"state", {
                            {"Status", igniting ? int(FireCellStatus::Igniting) : 0}
                        }},
                        {"config", {
                            {"FuelModel", igniting ? kFireFuelModelFM4 : fuelModel},
                            {"Slope", params.Slope(x, y)},
                            {"Aspect", params.Aspect(x, y)},
                            {"Longitude", params.Longitude(x, y)},
                            {"Latitude", params.Latitude(x, y)},
                            {"Height", params.Elevation(x, y)},
                            {"WindSpeed", params.WindSpeed(x, y)},
                            {"WindDirection", params.WindDirection(x, y)},
                            {"CanopyCover", params.CanopyCover(x, y)},
                            {"CanopyHeight", params.CanopyHeight(x, y)},
                            {"CrownRatio", params.CrownRatio(x, y)},
                            {"Moisture1Hour", params.MoistureOneHour(x, y)},
                            {"Moisture10Hour", params.MoistureTenHour(x, y)},
                            {"Moisture100Hour", params.MoistureHundredHour(x, y)},
                            {"MoistureLiveHerbaceous", params.MoistureLiveHerbaceous(x, y)},
                            {"MoistureLiveWoody", params.MoistureLiveWoody(x, y)}
                        }}
                    };
                    model->addCells(model->loadCellConfig(std::format("{},{}", x, y), config));
                }
            }
        }
        {
            FireProfileTagBlock("Models");
            spdlog::info("Building fire simulation couplings");
            model->addCouplings();
        }
        auto simulate = [&params](auto coordinator)
        {
            coordinator.setLogger(std::make_shared<FireModelLogger>(params.OutPath));
            coordinator.start();
            spdlog::info("Started fire simulation");
            std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
            coordinator.simulate();
            std::chrono::duration<double> elapsedTime = std::chrono::steady_clock::now() - startTime;
            spdlog::info("Completed fire simulation in {:.3f}s", elapsedTime.count());
            coordinator.stop();
        };
        switch (params.CoordinatorType)
        {
        case FireSimulatorCoordinatorType::BruteForce:
            simulate(cadmium::BruteForceRootCoordinator(model));
            break;
        case FireSimulatorCoordinatorType::EventDriven:
            simulate(cadmium::EventDrivenRootCoordinator(model));
            break;
        }
    }
    catch (const std::exception& e)
    {
        spdlog::info("fire simulation failed: {}", e.what());
        return false;
    }
    return true;
}
