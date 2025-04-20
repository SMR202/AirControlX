#pragma once
#include <string>
#include "enums.hpp"

struct Airline{
    std::string name;
    AirCraftType type;
    int totalAircrafts;
    int flightsInOperation;
};