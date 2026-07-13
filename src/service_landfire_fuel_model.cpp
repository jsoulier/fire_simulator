#include <memory>
#include <vector>

#include "service_landfire.hpp"

class ServiceLandfireFuelModel : public ServiceLandfire
{
public:
    const char* GetName() const override
    {
        return "landfire_fuel";
    }

    const char* GetDisplayName() const override
    {
        return "LANDFIRE Fuel Model (US)";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::FuelModel;
    }

    const char* GetServer() const override
    {
        return "Landfire_LF2024/LF2024_FBFM40_CONUS";
    }

    const char* GetInterpolation() const override
    {
        return "RSP_NearestNeighbor";
    }

    void PostProcess(ServiceSampleType type, std::vector<ServiceSampleTypeValue>& pixels) override
    {
        for (ServiceSampleTypeValue& pixel : pixels)
        {
            // no data (-9999) gets converted to 0 so we guard against that here
            if (pixel.U32 < kFireFuelModelNB1 || pixel.U32 > kFireFuelModelSB4)
            {
                pixel.U32 = kFireFuelModelNB9;
            }
        }
    }
};

std::unique_ptr<Service> ServiceCreateLandfireFuelModel()
{
    return std::make_unique<ServiceLandfireFuelModel>();
}
