#pragma once
// enums used because integers will get confusing 


enum class AirCraftType{
    commercial,
    cargo,
    emergency
};

enum class Direction{
    north,  // international arrivals
    south,  // domestic arrivals
    east,   // international departures
    west    // domestic departures
};

enum class AirCraftState{
    holding,
    approach,
    landing,
    taxi,
    at_gate,
    takeoff_roll,
    climb,
    departure,
};

enum class Runway{
    RWY_A,  // NW - ARRIVALS
    RWY_B,  // EW - DEPARTURES
    RWY_C,  // FLEXIBLE - cargo/emergency/overflow
};

enum class FlightType{
    international_arrival,
    domestic_arrival,
    international_departure,
    domestic_departure
};



