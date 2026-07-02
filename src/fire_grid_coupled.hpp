#pragma once

#include <cadmium/celldevs/grid/coupled.hpp>

#include <memory>

#include "fire_model.hpp"

// prevent default cells from being added since we're intentionally omitting non-burnable cells
class FireGridCoupled : public cadmium::celldevs::GridCellDEVSCoupled<FireState, double>
{
public:
    using GridCellDEVSCoupled::GridCellDEVSCoupled;
    void addDefaultCells(const std::shared_ptr<cadmium::celldevs::CellConfig<cadmium::celldevs::coordinates, FireState, double>>&) override {}
};
