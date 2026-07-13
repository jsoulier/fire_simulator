#pragma once

#include <ankerl/unordered_dense.h>
#include <glm/glm.hpp>

#include <vector>
#include <memory>

#include "fire_results.hpp"
#include "fire_simulator.hpp"
#include "future.hpp"
#include "reference.hpp"
#include "reference_database.hpp"
#include "service.hpp"
#include "worker.hpp"

class ServiceManager
{
public:
    ServiceManager();
    std::unique_ptr<Service>& GetService(ServiceSampleType type);
    std::unique_ptr<Reference>& GetReference();
    void RenderImGui(Worker& worker);
    void Download(Worker& worker);
    Future<FireResults> Simulate(Worker& worker, FireSimulatorParams& params);
    Future<FireResults> Fetch(Worker& worker);

private:
    std::vector<std::unique_ptr<Service>> Services;
    ankerl::unordered_dense::map<ServiceSampleType, int> ServiceIndices;
    ankerl::unordered_dense::map<int, ServiceSampleType> ServiceIndicesToTypes;
    std::vector<std::unique_ptr<Reference>> References;
    int ReferenceIndex = 0;
    ReferenceDatabase Database;
    float TileResolution;
    float TimeResolution;
};
