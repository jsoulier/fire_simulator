#include <memory>

#include "service_landfire.hpp"

class ServiceLandfireElevation : public ServiceLandfire
{
    SAVEPOINT_POLY(ServiceLandfireElevation)

public:
    const char* GetName() const override
    {
        return "landfire_elevation";
    }

    const char* GetDisplayName() const override
    {
        return "LANDFIRE Elevation (US)";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::Elevation;
    }

    const char* GetServer() const override
    {
        return "Landfire_Topo/LF2020_Elev_CONUS";
    }

    const char* GetInterpolation() const override
    {
        return "RSP_BilinearInterpolation";
    }
};

std::unique_ptr<Service> ServiceCreateLandfireElevation()
{
    return std::make_unique<ServiceLandfireElevation>();
}
