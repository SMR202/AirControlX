#pragma once
#include <string>
#include "enums.hpp"
#include "Airline.hpp"
#include <cstdlib> 

class Flight{
    static int nextId;
public:
    int id;
    std::string flightNumber;
    Airline* airline;
    Direction direction;
    AirCraftState state;
    AirCraftType type;
    Runway runway;
    double speed;
    int priority;
    bool hasActiveAVN;
    bool isEmergency;
    FlightType flightType;
    time_t scheduleTime;

    Flight(std::string flightNmber, Airline* airline, Direction direction, bool isEmergency = false)
        : flightNumber(flightNumber), airline(airline), direction(direction), hasActiveAVN(hasActiveAVN), isEmergency(isEmergency) {
        id = nextId++;
        type = airline->type;
        priority = calculatePriority();
        
        if (direction == Direction::north || direction == Direction::south) {
            runway = Runway::RWY_A;

            state = AirCraftState::holding;

            speed = 400 + (rand() % 201); // Random speed between 400 and 600
        } 
        else if (direction == Direction::east || direction == Direction::west) {
            runway = Runway::RWY_B;

            state = AirCraftState::at_gate;

            speed = 0;
        } 
        else {
            runway = Runway::RWY_C;
        }
        
        switch (direction) {
            case Direction::north:
                flightType = FlightType::international_arrival;
                break;
            case Direction::south:
                flightType = FlightType::domestic_arrival;
                break;
            case Direction::east:
                flightType = FlightType::international_departure;
                break;
            case Direction::west:
                flightType = FlightType::domestic_departure;
                break;
        }

    }

    int calculatePriority() {
        
        if (isEmergency) {
            return 100;
        }

        switch (type) {
            case AirCraftType::emergency:
                return 80;
            case AirCraftType::cargo:
                return 50;
            case AirCraftType::commercial:
                return 30;
            default:
                return 0;
        }
    }

    bool speedViolation(){
        bool violation = false;

        switch (state) {
            case AirCraftState::holding:
                if (speed > 600) {
                    violation = true;
                }
                break;
            case AirCraftState::approach:
                if (speed < 240 || speed > 290) {
                    violation = true;
                }
                break;
            case AirCraftState::landing:
                if (speed < 30 || speed > 240) {
                    violation = true;
                }
                break;
            case AirCraftState::taxi:
                if (speed > 30) {
                    violation = true;
                }
                break;
            case AirCraftState::at_gate:
                if (speed > 10) {
                    violation = true;
                }
                break;
            case AirCraftState::takeoff_roll:
                if (speed > 290) {
                    violation = true;
                }
                break;
            case AirCraftState::climb:
                if (speed > 463) {
                    violation = true;
                }
                break;
            case AirCraftState::departure:
                if (speed < 800 || speed > 900) {
                    violation = true;
                }
                break;
        }

        return violation;
    }

    void updateState() {

        switch (state) {
            case AirCraftState::holding:
                state = AirCraftState::approach;
                speed = 240 + (std::rand() % 51); // 240-290 km/h
                break;
                
            case AirCraftState::approach:
                state = AirCraftState::landing;
                speed = 240; // Start landing at 240 km/h
                break;
                
            case AirCraftState::landing:
                speed = 30; // Slow down to taxi speed
                state = AirCraftState::taxi;
                break;
                
            case AirCraftState::taxi:
                if (direction == Direction::north || direction == Direction::south) {
                    state = AirCraftState::at_gate;
                    speed = 0;
                } else {
                    state = AirCraftState::takeoff_roll;
                    speed = 0; // Start takeoff from standstill
                }
                break;
                
            case AirCraftState::at_gate:
                if (direction == Direction::east || direction == Direction::west) {
                    state = AirCraftState::taxi;
                    speed = 15 + (std::rand() % 16); // 15-30 km/h
                }
                break;
                
            case AirCraftState::takeoff_roll:
                state = AirCraftState::climb;
                speed = 250 + (std::rand() % 214); // 250-463 km/h
                break;
                
            case AirCraftState::climb:
                state = AirCraftState::departure;
                speed = 800 + (std::rand() % 101); // 800-900 km/h
                break;
                
            case AirCraftState::departure:
                // Flight has departed, nothing to do
                break;
        }
    }

    bool isArrival() {
        return (direction == Direction::north || direction == Direction::south);
    }
    bool isDeparture() {
        return (direction == Direction::east || direction == Direction::west);
    }
    bool isInternational() {
        return (direction == Direction::north || direction == Direction::east);
    }
    bool isDomestic() {
        return (direction == Direction::south || direction == Direction::west);
    }

    std::string getStateString() const {
        switch (state) {
            case AirCraftState::holding:
                return "Holding";
            case AirCraftState::approach:
                return "Approach";
            case AirCraftState::landing:
                return "Landing";
            case AirCraftState::taxi:
                return "Taxi";
            case AirCraftState::at_gate:
                return "At Gate";
            case AirCraftState::takeoff_roll:
                return "Takeoff Roll";
            case AirCraftState::climb:
                return "Climb";
            case AirCraftState::departure:
                return "Departure";
            default:
                return "Unknown State";
        }
    }

    std::string getDirectionString() const {
        switch (direction) {
            case Direction::north:
                return "North";
            case Direction::south:
                return "South";
            case Direction::east:
                return "East";
            case Direction::west:
                return "West";
            default:
                return "Unknown Direction";
        }
    }

    std::string getRunwayString() const {
        switch (runway) {
            case Runway::RWY_A:
                return "RWY-A";
            case Runway::RWY_B:
                return "RWY-B";
            case Runway::RWY_C:
                return "RWY-C";
            default:
                return "Unknown Runway";
        }
    }

    std::string getTypeString() const {
        switch (type) {
            case AirCraftType::commercial:
                return "Commercial";
            case AirCraftType::cargo:
                return "Cargo";
            case AirCraftType::emergency:
                return "Emergency";
            default:
                return "Unknown Type";
        }
    }

    std::string getFlightTypeString() const {
        switch (flightType) {
            case FlightType::international_arrival:
                return "International Arrival";
            case FlightType::domestic_arrival:
                return "Domestic Arrival";
            case FlightType::international_departure:
                return "International Departure";
            case FlightType::domestic_departure:
                return "Domestic Departure";
            default:
                return "Unknown Flight Type";
        }
    }


};

int Flight::nextId = 1;