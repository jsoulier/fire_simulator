#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

#include "fire_results.hpp"
#include "future.hpp"
#include "reference_database.hpp"
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
    Future<FireResults> Fetch(Worker& worker, const ReferenceDatabase& database, double resolution);

protected:
    virtual std::vector<std::string> GetURLs(const ReferenceDatabase& database) const = 0;
    virtual std::vector<ReferencePoint> GetPoints(const std::string& data) const = 0;
};

std::unique_ptr<Reference> ReferenceCreateFIRMS();
std::unique_ptr<Reference> ReferenceCreateEONET();
