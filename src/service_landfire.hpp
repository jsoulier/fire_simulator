// https://lfps.usgs.gov/arcgis/rest/services

#pragma once

#include <string>
#include <vector>

#include "service.hpp"

class ServiceLandfire : public Service
{
protected:
    std::vector<std::string> GetURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong) const override;
    int GetBand(ServiceSampleType type) const override;
    virtual const char* GetServer() const = 0;
    virtual const char* GetInterpolation() const = 0;
};
