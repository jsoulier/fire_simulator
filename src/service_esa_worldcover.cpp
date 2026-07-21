// https://esa-worldcover.s3.eu-central-1.amazonaws.com/v200/2021/docs/WorldCover_PUM_V2.0.pdf

#include <cmath>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "service.hpp"

static constexpr double kTileDegrees = 3.0;
static constexpr const char* kURL = "https://esa-worldcover.s3.eu-central-1.amazonaws.com/v200/2021/map";

class ServiceESAWorldCover : public Service
{
    SAVEPOINT_POLY(ServiceESAWorldCover)

public:
    const char* GetName() const override
    {
        return "esa_worldcover";
    }

    const char* GetDisplayName() const override
    {
        return "ESA WorldCover";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::FuelModel;
    }

    std::vector<std::string> GetURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, const Date& startDate, const Date& endDate) const override
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
                urls.push_back(std::format("/vsicurl/{}/ESA_WorldCover_10m_2021_v200_{}{:02d}{}{:03d}_Map.tif",
                    kURL,
                    int(lat) >= 0 ? 'N' : 'S',
                    std::abs(int(lat)),
                    int(lon) >= 0 ? 'E' : 'W',
                    std::abs(int(lon))));
            }
        }
        return urls;
    }

    int GetBand(ServiceSampleType type) const override
    {
        return 1;
    }

    void PostProcessStaticData(ServiceSampleType type, std::vector<ServiceSampleTypeValue>& pixels) override
    {
        for (ServiceSampleTypeValue& pixel : pixels)
        {
            switch (pixel.U32)
            {
            case 10: pixel.U32 = kFireFuelModelTU5; break; // tree cover
            case 20: pixel.U32 = kFireFuelModelSH5; break; // shrubland
            case 30: pixel.U32 = kFireFuelModelGR2; break; // grassland
            case 40: pixel.U32 = kFireFuelModelGR1; break; // cropland
            case 50: pixel.U32 = kFireFuelModelNB1; break; // built-up
            case 60: pixel.U32 = kFireFuelModelNB9; break; // bare / sparse vegetation
            case 70: pixel.U32 = kFireFuelModelNB2; break; // snow and ice
            case 80: pixel.U32 = kFireFuelModelNB8; break; // permanent water bodies
            case 90: pixel.U32 = kFireFuelModelGR2; break; // herbaceous wetland
            case 95: pixel.U32 = kFireFuelModelTU1; break; // mangroves
            case 100: pixel.U32 = kFireFuelModelGR1; break; // moss and lichen
            default:  pixel.U32 = kFireFuelModelNB9; break; // unknown / no data
            }
        }
    }
};

std::unique_ptr<Service> ServiceCreateESAWorldCover()
{
    return std::make_unique<ServiceESAWorldCover>();
}
