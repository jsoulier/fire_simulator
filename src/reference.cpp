#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "fire_model.hpp"
#include "http.hpp"
#include "reference.hpp"
#include "timer.hpp"

Future<FireResults> Reference::Fetch(
    Worker& worker,
    const glm::dvec2& minLatLong,
    const glm::dvec2& maxLatLong,
    double resolution,
    const Date& startDate,
    const Date& endDate)
{
    return worker.Submit([this, minLatLong, maxLatLong, resolution, startDate, endDate]()
    {
        TimerBlock(std::format("{} fetch", GetName()));
        FireResults results;
        double latRange = maxLatLong.x - minLatLong.x;
        double longRange = maxLatLong.y - minLatLong.y;
        if (latRange <= 0.0 || longRange <= 0.0)
        {
            return results;
        }
        int width = std::max(int(longRange / resolution), 1);
        int height = std::max(int(latRange / resolution), 1);
        std::filesystem::path basePath = SDL_GetBasePath();
        std::filesystem::path path = basePath / std::format("{}_{}.{}_{}.{}_{}_{}_{}.csv",
            GetName(),
            minLatLong.x,
            minLatLong.y,
            maxLatLong.x,
            maxLatLong.y,
            resolution,
            startDate.ToString(),
            endDate.ToString());
        if (!std::filesystem::exists(path))
        {
            std::vector<ReferencePoint> points;
            for (const std::string& url : GetURLs(minLatLong, maxLatLong, startDate, endDate))
            {
                std::optional<std::string> data = HttpGet(url);
                if (!data)
                {
                    continue;
                }
                std::vector<ReferencePoint> parsed = GetPoints(*data);
                points.insert(points.end(), parsed.begin(), parsed.end());
            }
            if (points.empty())
            {
                spdlog::warn("No points found for {}", GetName());
                return results;
            }
            double t1 = std::ranges::min(points, {}, &ReferencePoint::Time).Time;
            std::ofstream file(path);
            file << "time,x,y,longitude,latitude,elevation,size,status\n";
            for (const ReferencePoint& point : points)
            {
                int x = (point.LatLong.y - minLatLong.y) / longRange * width;
                int y = (maxLatLong.x - point.LatLong.x) / latRange * height;
                if (x < 0 || x >= width || y < 0 || y >= height)
                {
                    continue;
                }
                file << std::format("{},{},{},{},{},0,0,{}\n",
                    (point.Time - t1) / 3600.0,
                    x, y,
                    point.LatLong.y, point.LatLong.x,
                    int(FireCellStatus::Igniting));
            }
        }
        results.Load(path, {width, height});
        return results;
    });
}
