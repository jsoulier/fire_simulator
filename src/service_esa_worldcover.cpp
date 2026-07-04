// https://esa-worldcover.s3.eu-central-1.amazonaws.com/v200/2021/docs/WorldCover_PUM_V2.0.pdf

#include <cmath>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "service.hpp"

static constexpr double kTileDegrees = 3.0;
static constexpr const char* kURL = "https://esa-worldcover.s3.eu-central-1.amazonaws.com/v200/2021/map";

class ServiceEsaWorldCover : public Service
{
public:
    std::string GetName() const override
    {
        return "esa_worldcover";
    }

    std::vector<std::string> GetSourceURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong) const override
    {
        // ESA WorldCover is distributed as 3x3 degree GeoTIFFs
        glm::dvec2 cornerMinLatLong;
        cornerMinLatLong.x = std::floor(minLatLong.x / kTileDegrees) * kTileDegrees;
        cornerMinLatLong.y = std::floor(minLatLong.y / kTileDegrees) * kTileDegrees;
        std::vector<std::string> urls;
        for (double lat = cornerMinLatLong.x; lat < maxLatLong.x; lat += kTileDegrees)
        {
            for (double lon = cornerMinLatLong.y; lon < maxLatLong.y; lon += kTileDegrees)
            {
                int latValue = lat;
                int lonValue = lon;
                urls.push_back(std::format("/vsicurl/{}/ESA_WorldCover_10m_2021_v200_{}{:02d}{}{:03d}_Map.tif",
                    kURL,
                    latValue >= 0 ? 'N' : 'S',
                    std::abs(latValue),
                    lonValue >= 0 ? 'E' : 'W',
                    std::abs(lonValue)));
            }
        }
        return urls;
    }

    void SetSample(const glm::dvec2& latLong, ServiceSample& sample, ServiceSampleType type) override
    {
    }
};

std::unique_ptr<Service> CreateEsaWorldCoverService()
{
    return std::make_unique<ServiceEsaWorldCover>();
}
