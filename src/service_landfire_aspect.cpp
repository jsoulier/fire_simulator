#include <memory>

#include "service_landfire.hpp"

class ServiceLandfireAspect : public ServiceLandfire
{
    SAVEPOINT_POLY(ServiceLandfireAspect)

public:
    const char* GetName() const override
    {
        return "landfire_aspect";
    }

    const char* GetDisplayName() const override
    {
        return "LANDFIRE Aspect (US)";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::Aspect;
    }

    const char* GetServer() const override
    {
        return "Landfire_Topo/LF2020_Asp_CONUS";
    }

    const char* GetInterpolation() const override
    {
        return "RSP_NearestNeighbor";
    }
};

std::unique_ptr<Service> ServiceCreateLandfireAspect()
{
    return std::make_unique<ServiceLandfireAspect>();
}
