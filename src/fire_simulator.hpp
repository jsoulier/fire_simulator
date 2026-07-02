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
    int FireX;                                                 // fire origin
    int FireY;                                                 // fire origin
    std::function<FireFuelModelType(int x, int y)> FuelModel;
    std::function<double(int x, int y)> Longitude;
    std::function<double(int x, int y)> Latitude;
    std::function<float(int x, int y)> Elevation;              // metres
    std::function<float(int x, int y)> Slope;                  // degrees
    std::function<float(int x, int y)> Aspect;                 // degrees
    std::function<float(int x, int y)> CanopyCover;            // 0 to 1 fraction
    std::function<float(int x, int y)> CanopyHeight;           // metres
    std::function<float(int x, int y)> CrownRatio;             // 0 to 1 fraction
    std::function<float(int x, int y)> WindSpeed;              // mph
    std::function<float(int x, int y)> WindDirection;          // relative to north degrees
    std::function<float(int x, int y)> MoistureOneHour;        // percent
    std::function<float(int x, int y)> MoistureTenHour;        // percent
    std::function<float(int x, int y)> MoistureHundredHour;    // percent
    std::function<float(int x, int y)> MoistureLiveHerbaceous; // percent
    std::function<float(int x, int y)> MoistureLiveWoody;      // percent
    std::string OutPath;
};

bool FireSimulatorRun(const FireSimulatorParams& params);
