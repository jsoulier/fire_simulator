// https://portal.opentopography.org/apidocs/#/Public/getGlobalDem

#include <spdlog/spdlog.h>

#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "auth.hpp"
#include "math.hpp"
#include "service.hpp"

static constexpr const char* kURL = "https://portal.opentopography.org/API/globaldem";
static constexpr const char* kDEMType = "SRTMGL1";
static constexpr double kMinimumSizeMeters = 250.0; // "Error: Each side of the bounding box must be greater than approximately 250 meters"

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

    std::vector<std::string> GetURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, const Date& startDate, const Date& endDate) const override
    {
        const glm::dvec2 size = MathLatLongToMeters(minLatLong, maxLatLong);
        if (size.x <= kMinimumSizeMeters || size.y <= kMinimumSizeMeters)
        {
            spdlog::error("Bounds must be greater then {} meters: {}", kMinimumSizeMeters, GetName());
            return {};
        }
        const std::string key = AuthGetKey("open_topography.txt");
        if (key.empty())
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
                key),
        };
    }

    int GetBand(ServiceSampleType type) const override
    {
        return 1;
    }

    void DeriveStaticData(ServiceSampleType type, GDALDatasetH lowResolution, const std::string& directory) override
    {
        if (type == ServiceSampleType::Elevation)
        {
            DEMProcessing(lowResolution, directory, ServiceSampleType::Slope);
            DEMProcessing(lowResolution, directory, ServiceSampleType::Aspect);
        }
    }
};

std::unique_ptr<Service> ServiceCreateOpenTopography()
{
    return std::make_unique<ServiceOpenTopography>();
}
