#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

#include "date.hpp"
#include "fire_results.hpp"
#include "future.hpp"
#include "worker.hpp"

struct ReferencePoint
{
    glm::dvec2 LatLong;
    double Time;
};

class Reference
{
public:
    virtual ~Reference() = default;
    virtual const char* GetName() const = 0;
    virtual const char* GetDisplayName() const = 0;
    Future<FireResults> Fetch(
        Worker& worker,
        const glm::dvec2& minLatLong,
        const glm::dvec2& maxLatLong,
        double resolution,
        const Date& startDate,
        const Date& endDate);

protected:
    virtual std::vector<std::string> GetURLs(
        const glm::dvec2& minLatLong,
        const glm::dvec2& maxLatLong,
        const Date& startDate,
        const Date& endDate) const = 0;
    virtual std::vector<ReferencePoint> GetPoints(const std::string& data) const = 0;
};

std::unique_ptr<Reference> ReferenceCreateFIRMS();
