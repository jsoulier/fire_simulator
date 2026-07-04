#include <memory>

#include "service.hpp"

int main(int argc, char** argv)
{
    std::unique_ptr<Service> service = ServiceCreateESAWorldCover();
    const glm::dvec2 minLatLong = { 49.20, -123.20 };
    const glm::dvec2 maxLatLong = { 49.35, -123.00 };
    const double resolution = 0.001;
    service->Download(ServiceSampleType::FuelModel, minLatLong, maxLatLong, resolution);
    return 0;
}
