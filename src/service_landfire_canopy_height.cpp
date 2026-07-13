#include <memory>
#include <vector>

#include "service_landfire.hpp"

class ServiceLandfireCanopyHeight : public ServiceLandfire
{
public:
    const char* GetName() const override
    {
        return "landfire_canopy_height";
    }

    const char* GetDisplayName() const override
    {
        return "LANDFIRE Canopy Height (US)";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::CanopyHeight;
    }

    const char* GetServer() const override
    {
        return "Landfire_LF2024/LF2024_CH_CONUS";
    }

    const char* GetInterpolation() const override
    {
        return "RSP_BilinearInterpolation";
    }

    void PostProcess(ServiceSampleType type, std::vector<ServiceSampleTypeValue>& pixels) override
    {
        for (ServiceSampleTypeValue& pixel : pixels)
        {
            if (pixel.F32 >= 0.0f)
            {
                pixel.F32 *= 0.1f;
            }
        }
    }
};

std::unique_ptr<Service> ServiceCreateLandfireCanopyHeight()
{
    return std::make_unique<ServiceLandfireCanopyHeight>();
}
