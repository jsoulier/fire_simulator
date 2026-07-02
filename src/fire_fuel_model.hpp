// sourced from behave/src/behave/fuelModels.cpp

#pragma once

#include <cstdint>

using FireFuelModelType = uint8_t;

bool FireFuelModelTypeIsBurnable(FireFuelModelType fuelModel);

// original
inline constexpr FireFuelModelType kFireFuelModelFM1 = 1;
inline constexpr FireFuelModelType kFireFuelModelFM2 = 2;
inline constexpr FireFuelModelType kFireFuelModelFM3 = 3;
inline constexpr FireFuelModelType kFireFuelModelFM4 = 4;
inline constexpr FireFuelModelType kFireFuelModelFM5 = 5;
inline constexpr FireFuelModelType kFireFuelModelFM6 = 6;
inline constexpr FireFuelModelType kFireFuelModelFM7 = 7;
inline constexpr FireFuelModelType kFireFuelModelFM8 = 8;
inline constexpr FireFuelModelType kFireFuelModelFM9 = 9;
inline constexpr FireFuelModelType kFireFuelModelFM10 = 10;
inline constexpr FireFuelModelType kFireFuelModelFM11 = 11;
inline constexpr FireFuelModelType kFireFuelModelFM12 = 12;
inline constexpr FireFuelModelType kFireFuelModelFM13 = 13;

// non-burnable
inline constexpr FireFuelModelType kFireFuelModelNB1 = 91;
inline constexpr FireFuelModelType kFireFuelModelNB2 = 92;
inline constexpr FireFuelModelType kFireFuelModelNB3 = 93;
inline constexpr FireFuelModelType kFireFuelModelNB4 = 94;
inline constexpr FireFuelModelType kFireFuelModelNB5 = 95;
inline constexpr FireFuelModelType kFireFuelModelNB8 = 98;
inline constexpr FireFuelModelType kFireFuelModelNB9 = 99;

// grass
inline constexpr FireFuelModelType kFireFuelModelGR1 = 101;
inline constexpr FireFuelModelType kFireFuelModelGR2 = 102;
inline constexpr FireFuelModelType kFireFuelModelGR3 = 103;
inline constexpr FireFuelModelType kFireFuelModelGR4 = 104;
inline constexpr FireFuelModelType kFireFuelModelGR5 = 105;
inline constexpr FireFuelModelType kFireFuelModelGR6 = 106;
inline constexpr FireFuelModelType kFireFuelModelGR7 = 107;
inline constexpr FireFuelModelType kFireFuelModelGR8 = 108;
inline constexpr FireFuelModelType kFireFuelModelGR9 = 109;
inline constexpr FireFuelModelType kFireFuelModelVHb = 110;
inline constexpr FireFuelModelType kFireFuelModelVHa = 111;

// grass and shrub
inline constexpr FireFuelModelType kFireFuelModelGS1 = 121;
inline constexpr FireFuelModelType kFireFuelModelGS2 = 122;
inline constexpr FireFuelModelType kFireFuelModelGS3 = 123;
inline constexpr FireFuelModelType kFireFuelModelGS4 = 124;

// shrub
inline constexpr FireFuelModelType kFireFuelModelSH1 = 141;
inline constexpr FireFuelModelType kFireFuelModelSH2 = 142;
inline constexpr FireFuelModelType kFireFuelModelSH3 = 143;
inline constexpr FireFuelModelType kFireFuelModelSH4 = 144;
inline constexpr FireFuelModelType kFireFuelModelSH5 = 145;
inline constexpr FireFuelModelType kFireFuelModelSH6 = 146;
inline constexpr FireFuelModelType kFireFuelModelSH7 = 147;
inline constexpr FireFuelModelType kFireFuelModelSH8 = 148;
inline constexpr FireFuelModelType kFireFuelModelSH9 = 149;
inline constexpr FireFuelModelType kFireFuelModelSCAL17 = 150;
inline constexpr FireFuelModelType kFireFuelModelSCAL15 = 151;
inline constexpr FireFuelModelType kFireFuelModelSCAL16 = 152;
inline constexpr FireFuelModelType kFireFuelModelSCAL14 = 153;
inline constexpr FireFuelModelType kFireFuelModelSCAL18 = 154;
inline constexpr FireFuelModelType kFireFuelModelVMH = 155;
inline constexpr FireFuelModelType kFireFuelModelVMMb = 156;
inline constexpr FireFuelModelType kFireFuelModelVMAb = 157;
inline constexpr FireFuelModelType kFireFuelModelVMMa = 158;
inline constexpr FireFuelModelType kFireFuelModelVMaa = 159;

// timber and understory
inline constexpr FireFuelModelType kFireFuelModelTU1 = 161;
inline constexpr FireFuelModelType kFireFuelModelTU2 = 162;
inline constexpr FireFuelModelType kFireFuelModelTU3 = 163;
inline constexpr FireFuelModelType kFireFuelModelTU4 = 164;
inline constexpr FireFuelModelType kFireFuelModelTU5 = 165;
inline constexpr FireFuelModelType kFireFuelModelMEUCd = 166;
inline constexpr FireFuelModelType kFireFuelModelMH = 167;
inline constexpr FireFuelModelType kFireFuelModelMF = 168;
inline constexpr FireFuelModelType kFireFuelModelMCAD = 169;
inline constexpr FireFuelModelType kFireFuelModelMESC = 170;
inline constexpr FireFuelModelType kFireFuelModelMPIN = 171;
inline constexpr FireFuelModelType kFireFuelModelMEUC = 172;

// timber and litter
inline constexpr FireFuelModelType kFireFuelModelTL1 = 181;
inline constexpr FireFuelModelType kFireFuelModelTL2 = 182;
inline constexpr FireFuelModelType kFireFuelModelTL3 = 183;
inline constexpr FireFuelModelType kFireFuelModelTL4 = 184;
inline constexpr FireFuelModelType kFireFuelModelTL5 = 185;
inline constexpr FireFuelModelType kFireFuelModelTL6 = 186;
inline constexpr FireFuelModelType kFireFuelModelTL7 = 187;
inline constexpr FireFuelModelType kFireFuelModelTL8 = 188;
inline constexpr FireFuelModelType kFireFuelModelTL9 = 189;
inline constexpr FireFuelModelType kFireFuelModelFRAC = 190;
inline constexpr FireFuelModelType kFireFuelModelFFOL = 191;
inline constexpr FireFuelModelType kFireFuelModelFPIN = 192;
inline constexpr FireFuelModelType kFireFuelModelFEUC = 193;

// slash and blowdown
inline constexpr FireFuelModelType kFireFuelModelSB1 = 201;
inline constexpr FireFuelModelType kFireFuelModelSB2 = 202;
inline constexpr FireFuelModelType kFireFuelModelSB3 = 203;
inline constexpr FireFuelModelType kFireFuelModelSB4 = 204;
