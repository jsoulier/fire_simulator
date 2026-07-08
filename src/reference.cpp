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

Future<FireResults> Reference::Fetch(Worker& worker, const ReferenceDatabase& database, double resolution)
{
    return worker.Submit([this, database, resolution]()
    {
        TimerBlock(std::format("{} fetch", GetName()));
        FireResults results;
        glm::dvec2 minLatLong = database.GetMinLatLong();
        glm::dvec2 maxLatLong = database.GetMaxLatLong();
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
            database.GetStartDate().ToString(),
            database.GetEndDate().ToString());
        if (!std::filesystem::exists(path))
        {
            std::vector<ReferencePoint> points;
            for (const std::string& url : GetURLs(database))
            {
                std::optional<std::string> data = HttpGet(url);
                if (!data)
                {
                    continue;
                }
                std::vector<ReferencePoint> newPoints = GetPoints(*data);
                if (newPoints.empty())
                {
                    spdlog::warn("No points parsed from {}: {}", url, data->substr(0, 256));
                }
                points.insert(points.end(), newPoints.begin(), newPoints.end());
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
                std::string row = std::format("{},{},{},{},{},0,0,",
                    (point.Time - t1) / 3600.0,
                    x, y,
                    point.LatLong.y, point.LatLong.x);
                file << row << int(FireCellStatus::Igniting) << '\n';
                file << row << int(FireCellStatus::Burnt) << '\n';
            }
        }
        results.Load(path, {width, height});
        return results;
    });
}
