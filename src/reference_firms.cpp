// https://firms.modaps.eosdis.nasa.gov/api/area

#include <glm/glm.hpp>

#include <algorithm>
#include <cstdio>
#include <format>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "auth.hpp"
#include "date.hpp"
#include "reference.hpp"

static constexpr const char* kURL = "https://firms.modaps.eosdis.nasa.gov/api/area/csv";
static constexpr const char* kSource = "VIIRS_SNPP_SP";
static constexpr int kMaxDaysPerURL = 5;

class ReferenceFIRMS : public Reference
{
public:
    const char* GetName() const override
    {
        return "firms";
    }

    const char* GetDisplayName() const override
    {
        return "NASA FIRMS";
    }

    std::vector<std::string> GetURLs(
        const glm::dvec2& minLatLong,
        const glm::dvec2& maxLatLong,
        const Date& startDate,
        const Date& endDate) const override
    {
        std::string key = AuthGetKey("firms_map_key.txt");
        if (key.empty())
        {
            return {};
        }
        int maxDays = endDate - startDate + 1;
        std::vector<std::string> urls;
        for (int i = 0; i < maxDays; i += kMaxDaysPerURL)
        {
            const int dayRange = std::min(kMaxDaysPerURL, maxDays - i);
            urls.push_back(std::format("{}/{}/{}/{},{},{},{}/{}/{}",
                kURL,
                key,
                kSource,
                minLatLong.y, minLatLong.x, maxLatLong.y, maxLatLong.x,
                dayRange,
                (startDate + i).ToString()));
        }
        return urls;
    }

    std::vector<ReferencePoint> GetPoints(const std::string& data) const override
    {
        std::vector<ReferencePoint> points;
        std::istringstream stream(data);
        std::string line;
        std::getline(stream, line); // header
        while (std::getline(stream, line))
        {
            double lat;
            double lon;
            int year;
            int month;
            int day;
            int hm;
            if (std::sscanf(line.c_str(), "%lf,%lf,%*f,%*f,%*f,%d-%d-%d,%d", &lat, &lon, &year, &month, &day, &hm) == 6)
            {
                points.push_back({glm::dvec2(lat, lon), Date(year, month, day).ToEpoch(hm)});
            }
        }
        return points;
    }
};

std::unique_ptr<Reference> ReferenceCreateFIRMS()
{
    return std::make_unique<ReferenceFIRMS>();
}
