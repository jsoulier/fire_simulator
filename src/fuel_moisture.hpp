#pragma once

#include <span>
#include <vector>

#include "service.hpp"

std::vector<ServiceSampleTypeValue> FuelMoistureCalculateDead(
    std::span<const ServiceSampleTypeValue> temperature,
    std::span<const ServiceSampleTypeValue> humidity,
    std::span<const ServiceSampleTypeValue> precipitation,
    double timeLag);
std::vector<ServiceSampleTypeValue> FuelMoistureCalculateLive(
    std::span<const ServiceSampleTypeValue> temperature,
    std::span<const ServiceSampleTypeValue> humidity,
    std::span<const ServiceSampleTypeValue> precipitation,
    double latitude,
    ServiceSampleType type);
