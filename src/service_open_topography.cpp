// https://portal.opentopography.org/apidocs/#/Public/getGlobalDem

#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "auth.hpp"
#include "service.hpp"

static constexpr const char* kURL = "https://portal.opentopography.org/API/globaldem";
static constexpr const char* kDEMType = "SRTMGL1";

class ServiceOpenTopography : public Service
{
public:
    const char* GetName() const override
    {
        return "open_topography";
    }

    const char* GetDisplayName() const override
    {
        return "Open Topography";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::Elevation | ServiceSampleType::Slope | ServiceSampleType::Aspect;
    }

    ServiceSampleType GetRequiredSampleTypes(ServiceSampleType types) const override
    {
        return ServiceSampleType::Elevation;
    }

    std::vector<std::string> GetURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong) const override
    {
        const std::string apiKey = AuthGetKey("open_topography.txt");
        if (apiKey.empty())
        {
            return {};
        }
        return
        {
            std::format("/vsicurl_streaming/{}?demtype={}&south={}&north={}&west={}&east={}&outputFormat=GTiff&API_Key={}",
                kURL,
                kDEMType,
                minLatLong.x,
                maxLatLong.x,
                minLatLong.y,
                maxLatLong.y,
                apiKey),
        };
    }

    int GetBand(ServiceSampleType type) const override
    {
        return 1;
    }

    void Derive(ServiceSampleType type, GDALDatasetH lowResolution, const std::string& basePath) override
    {
        if (type == ServiceSampleType::Elevation)
        {
            DEMProcessing(lowResolution, basePath, ServiceSampleType::Slope);
            DEMProcessing(lowResolution, basePath, ServiceSampleType::Aspect);
        }
    }
};

std::unique_ptr<Service> ServiceCreateOpenTopography()
{
    return std::make_unique<ServiceOpenTopography>();
}
