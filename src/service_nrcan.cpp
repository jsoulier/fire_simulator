// https://cwfis.cfs.nrcan.gc.ca/downloads/fuels/development/Canadian_Forest_FBP_Fuel_Types/FBPfueltypes_sample_colour_classification.xlsx

#include <memory>
#include <string>
#include <vector>

#include "service.hpp"

static constexpr const char* kSource =
    "/vsicurl_streaming/https://cwfis.cfs.nrcan.gc.ca/downloads/fuels/current/"
    "FBP_fueltypes_Canada_100m_EPSG3978_20240527.tif";

class ServiceNRCan : public Service
{
public:
    const char* GetName() const override
    {
        return "nrcan";
    }

    const char* GetDisplayName() const override
    {
        return "NRCan FBP Fuel Types (Canada)";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::FuelModel;
    }

    std::vector<std::string> GetURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, const Date& startDate, const Date& endDate) const override
    {
        return {kSource};
    }

    int GetBand(ServiceSampleType type) const override
    {
        return 1;
    }

    void PostProcess(ServiceSampleType type, std::vector<ServiceSampleTypeValue>& pixels) override
    {
        for (ServiceSampleTypeValue& pixel : pixels)
        {
            switch (pixel.U32)
            {
            case 1: pixel.U32 = kFireFuelModelTU1; break; // C-1 open spruce-lichen woodland
            case 2: pixel.U32 = kFireFuelModelTU5; break; // C-2 boreal spruce, intense crown fire
            case 3: pixel.U32 = kFireFuelModelTU5; break; // C-3 mature jack/lodgepole pine
            case 4: pixel.U32 = kFireFuelModelTU5; break; // C-4 immature dense pine, heavy ladder
            case 5: pixel.U32 = kFireFuelModelTU1; break; // C-5 red and white pine
            case 6: pixel.U32 = kFireFuelModelTU5; break; // C-6 conifer plantation
            case 7: pixel.U32 = kFireFuelModelTU1; break; // C-7 open ponderosa / Douglas-fir
            case 11: pixel.U32 = kFireFuelModelTL2; break; // D-1 leafless aspen (broadleaf litter)
            case 12: pixel.U32 = kFireFuelModelTL2; break; // D-2 green aspen (broadleaf litter)
            case 13: pixel.U32 = kFireFuelModelTL2; break; // D-1/D-2 aspen (broadleaf litter)
            case 21: pixel.U32 = kFireFuelModelSB1; break; // S-1 jack/lodgepole slash
            case 22: pixel.U32 = kFireFuelModelSB2; break; // S-2 white spruce/balsam slash
            case 23: pixel.U32 = kFireFuelModelSB3; break; // S-3 coastal cedar/hemlock/DF slash
            case 31: pixel.U32 = kFireFuelModelGR1; break; // O-1a matted grass
            case 32: pixel.U32 = kFireFuelModelGR2; break; // O-1b standing grass
            case 40: pixel.U32 = kFireFuelModelTU5; break; // M-1 boreal mixedwood leafless
            case 50: pixel.U32 = kFireFuelModelTU1; break; // M-2 boreal mixedwood green
            case 60: pixel.U32 = kFireFuelModelTU5; break; // M-1/M-2
            case 70: pixel.U32 = kFireFuelModelTU5; break; // M-3 dead balsam fir leafless
            case 80: pixel.U32 = kFireFuelModelTU5; break; // M-4 dead balsam fir green
            case 90: pixel.U32 = kFireFuelModelTU5; break; // M-3/M-4
            case 102: pixel.U32 = kFireFuelModelNB8; break; // water
            case 106: pixel.U32 = kFireFuelModelNB1; break; // urban / built-up
            default:
            {
                if (pixel.U32 >= 400 && pixel.U32 < 1000)
                {
                    // mixedwood percentages in increments of 5%
                    if (pixel.U32 / 100 == 5)
                    {
                        pixel.U32 = kFireFuelModelTU1;
                    }
                    else
                    {
                        pixel.U32 = kFireFuelModelTU5;
                    }
                }
                else
                {
                    // unsupported
                    pixel.U32 = kFireFuelModelNB9;
                }
                break;
            }
            }

        }
    }
};

std::unique_ptr<Service> ServiceCreateNRCan()
{
    return std::make_unique<ServiceNRCan>();
}
