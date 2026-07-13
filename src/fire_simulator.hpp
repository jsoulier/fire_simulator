#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <string>

#include "fire_fuel_model.hpp"

enum class FireSimulatorCoordinatorType
{
    EventDriven, // cells are only computed if they have incoming messages 
    BruteForce,  // cells are computed each frame
};

static constexpr const char* kFireSimulatorCoordinatorTypeStrings[] =
{
    "Event Driven",
    "Brute Force",
};

NLOHMANN_JSON_SERIALIZE_ENUM(FireSimulatorCoordinatorType, {
    {FireSimulatorCoordinatorType::EventDriven, "EventDriven"},
    {FireSimulatorCoordinatorType::BruteForce, "BruteForce"},
})

struct FireSimulatorParams
{
    FireSimulatorParams();

    int Width;                                                 // number of cells
    int Height;                                                // number of cells
    float Resolution;                                          // size of each cell
    FireSimulatorCoordinatorType CoordinatorType;
    std::function<bool(int x, int y)> Igniting;
    std::function<FireFuelModelType(int x, int y)> FuelModel;
    std::function<double(int x, int y)> Longitude;
    std::function<double(int x, int y)> Latitude;
    std::function<float(int x, int y)> Elevation;              // metres
    std::function<float(int x, int y)> Slope;                  // degrees
    std::function<float(int x, int y)> Aspect;                 // degrees
    std::function<float(int x, int y)> CanopyCover;            // 0 to 1 fraction
    std::function<float(int x, int y)> CanopyHeight;           // metres
    std::function<float(int x, int y)> CrownRatio;             // 0 to 1 fraction
    std::function<float(int x, int y, float time)> WindSpeed;  // mph; time is simulation hours
    std::function<float(int x, int y, float time)> WindDirection; // relative to north degrees; time is simulation hours
    std::function<float(int x, int y, float time)> MoistureOneHour;        // percent; time is simulation hours
    std::function<float(int x, int y, float time)> MoistureTenHour;        // percent; time is simulation hours
    std::function<float(int x, int y, float time)> MoistureHundredHour;    // percent; time is simulation hours
    std::function<float(int x, int y, float time)> MoistureLiveHerbaceous; // percent; time is simulation hours
    std::function<float(int x, int y, float time)> MoistureLiveWoody;      // percent; time is simulation hours
    std::string OutPath;
};

bool FireSimulatorRun(const FireSimulatorParams& params);
