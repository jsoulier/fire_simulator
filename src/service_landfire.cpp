#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <vector>

#include "service_landfire.hpp"

static constexpr double kMetresPerDegree = 111320.0;
static constexpr double kDegreesToRadians = 0.017453292519943295;
static constexpr double kLandfireMetres = 30.0;

std::vector<std::string> ServiceLandfire::GetURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, const Date& startDate, const Date& endDate) const
{
    double center = (minLatLong.x + maxLatLong.x) * 0.5;
    double widthMetres = (maxLatLong.y - minLatLong.y) * kMetresPerDegree * std::cos(center * kDegreesToRadians);
    double heightMetres = (maxLatLong.x - minLatLong.x) * kMetresPerDegree;
    int width = std::max(int(widthMetres / kLandfireMetres), 1);
    int height = std::max(int(heightMetres / kLandfireMetres), 1);
    return
    {
        std::format(
            "/vsicurl_streaming/https://lfps.usgs.gov/arcgis/rest/services/{}/ImageServer/exportImage"
            "?bbox={},{},{},{}&bboxSR=4326&imageSR=4326&size={},{}"
            "&format=tiff&pixelType=S16&interpolation={}&f=image",
            GetServer(),
            minLatLong.y, minLatLong.x, maxLatLong.y, maxLatLong.x,
            width, height,
            GetInterpolation()),
    };
}

int ServiceLandfire::GetBand(ServiceSampleType type) const
{
    return 1;
}
