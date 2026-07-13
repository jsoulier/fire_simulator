#include <memory>
#include <vector>

#include "service_landfire.hpp"

class ServiceLandfireCanopyCover : public ServiceLandfire
{
public:
    const char* GetName() const override
    {
        return "landfire_canopy_cover";
    }

    const char* GetDisplayName() const override
    {
        return "LANDFIRE Canopy Cover (US)";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::CanopyCover;
    }

    const char* GetServer() const override
    {
        return "Landfire_LF2024/LF2024_CC_CONUS";
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
                pixel.F32 *= 0.01f;
            }
        }
    }
};

std::unique_ptr<Service> ServiceCreateLandfireCanopyCover()
{
    return std::make_unique<ServiceLandfireCanopyCover>();
}
