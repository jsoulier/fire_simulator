#pragma once

#include <ankerl/unordered_dense.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <savepoint/savepoint.hpp>

#include <array>
#include <iterator>
#include <map>
#include <memory>
#include <vector>

#include "fire_results.hpp"
#include "fire_simulator.hpp"
#include "future.hpp"
#include "reference.hpp"
#include "reference_database.hpp"
#include "service.hpp"
#include "service_context.hpp"
#include "worker.hpp"

class ServiceManager
{
public:
    ServiceManager();
    void Visit(SavepointVisitor& visitor);
    std::unique_ptr<Reference>& GetReference();
    ServiceSampleTypeValue GetValue(ServiceSampleType type, const glm::dvec2& latLong, float time) const;
    ServiceSampleTypeValue GetValue(ServiceSampleType type, int x, int y, float time) const;
    glm::ivec2 GetSize(ServiceSampleType type) const;
    ImTextureRef GetTextureRef(ServiceSampleType type) const;
    void RenderImGui(Worker& worker);
    void Download(Worker& worker);
    Future<FireResults> Simulate(Worker& worker, ankerl::unordered_dense::set<glm::ivec2> selected);
    Future<FireResults> Fetch(Worker& worker);

private:
    ServiceContext Context;
    FireSimulator Simulator;
    std::vector<std::unique_ptr<Service>> Services;
    std::array<int, kServiceSampleTypeMax> ServiceIndices;
    std::map<int, ServiceSampleType> ServiceIndicesToTypes;
    std::vector<std::unique_ptr<Reference>> References;
    int ReferenceIndex;
    ReferenceDatabase Database;
    float TileResolution;
    float TimeResolution;
};
