#include <memory>

#include "service_landfire.hpp"

class ServiceLandfireSlope : public ServiceLandfire
{
    SAVEPOINT_POLY(ServiceLandfireSlope)

public:
    const char* GetName() const override
    {
        return "landfire_slope";
    }

    const char* GetDisplayName() const override
    {
        return "LANDFIRE Slope (US)";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::Slope;
    }

    const char* GetServer() const override
    {
        return "Landfire_Topo/LF2020_SlpD_CONUS";
    }

    const char* GetInterpolation() const override
    {
        return "RSP_BilinearInterpolation";
    }
};

std::unique_ptr<Service> ServiceCreateLandfireSlope()
{
    return std::make_unique<ServiceLandfireSlope>();
}
