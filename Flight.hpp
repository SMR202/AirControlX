#pragma once
#include <string>
#include "enums.hpp"
#include "Airline.hpp"
#include <cstdlib> 
#include <SFML/Graphics.hpp>

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
    int altitude; // New altitude property in feet
    sf::Sprite planeSprite;

    Flight(std::string flightNumber, Airline* airline, Direction direction, bool isEmergency = false)
        : flightNumber(flightNumber), airline(airline), direction(direction), isEmergency(isEmergency) {
        id = nextId++;
        type = airline->type;
        priority = calculatePriority();
        hasActiveAVN = false;
        scheduleTime = time(nullptr);
        
        
        if (direction == Direction::north || direction == Direction::south) {

            if (type == AirCraftType::cargo || type == AirCraftType::emergency) {
                runway = Runway::RWY_C;
            } else {
                runway = Runway::RWY_A;
            }

            state = AirCraftState::holding;

            speed = 400 + (rand() % 201); // Random speed between 400 and 600
        } 
        else if (direction == Direction::east || direction == Direction::west) {
            if (type == AirCraftType::cargo || type == AirCraftType::emergency) {
                runway = Runway::RWY_C;
            } else {
                runway = Runway::RWY_B;
            }

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
        
        // Initialize altitude based on current state
        altitude = getAltitude();

        planeSprite.setTexture(airline->planeTexture);
        planeSprite.setOrigin(airline->planeTexture.getSize().x / 2, airline->planeTexture.getSize().y / 2);
        planeSprite.setScale(0.4f, 0.4f);
        if (runway == Runway::RWY_C){
            planeSprite.setPosition(WindowX * 0.9f, WindowY * 0.9f);
        } 
        else if (runway == Runway::RWY_A){
            if (direction == Direction::north){
                planeSprite.setPosition(WindowX * 0.1f, WindowY * 0.9f);
                planeSprite.setRotation(0); // Rotate for north direction
            } else {
                planeSprite.setPosition(WindowX * 0.1f, WindowY * 0.1f);
                planeSprite.setRotation(180); // Rotate for south direction
            }
        } 
        else if (runway == Runway::RWY_B){
            if (direction == Direction::east){
                planeSprite.setPosition(WindowX * 0.2f, WindowY * 0.85f);
                planeSprite.setRotation(90); // Rotate for east direction
            } else {
                planeSprite.setPosition(WindowX * 0.8f, WindowY * 0.85f);
                planeSprite.setRotation(270); // Rotate for west direction
            }
        }
    }

    Flight(const Flight& other) {
        id = other.id;
        flightNumber = other.flightNumber;
        airline = other.airline;
        direction = other.direction;
        state = other.state;
        type = other.type;
        runway = other.runway;
        speed = other.speed;
        priority = other.priority;
        hasActiveAVN = other.hasActiveAVN;
        isEmergency = other.isEmergency;
        flightType = other.flightType;
        scheduleTime = other.scheduleTime;
        altitude = other.altitude;
    }
    Flight& operator=(const Flight& other) {
        if (this != &other) {
            id = other.id;
            flightNumber = other.flightNumber;
            airline = other.airline;
            direction = other.direction;
            state = other.state;
            type = other.type;
            runway = other.runway;
            speed = other.speed;
            priority = other.priority;
            hasActiveAVN = other.hasActiveAVN;
            isEmergency = other.isEmergency;
            flightType = other.flightType;
            scheduleTime = other.scheduleTime;
            altitude = other.altitude;
        }
        return *this;
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
        // Add a small chance (5%) of speed violation for demonstration purposes
        bool createViolation = (rand() % 100 < 5);

        switch (state) {
            case AirCraftState::holding:
                state = AirCraftState::approach;
                if (createViolation) {
                    // Deliberately create a speed violation by exceeding 290 km/h for approach
                    speed = 300 + (std::rand() % 50); // 300-350 km/h (over limit)
                } else {
                    speed = 240 + (std::rand() % 51); // 240-290 km/h (normal)
                }
                break;
                
            case AirCraftState::approach:
                state = AirCraftState::landing;
                if (createViolation) {
                    // Create a landing speed violation (over 240 km/h)
                    speed = 250 + (std::rand() % 30); // 250-280 km/h (over limit)
                } else {
                    speed = 240; // Normal landing speed
                }
                break;
                
            case AirCraftState::landing:
                speed = createViolation ? 35 : 30; // Possibly violate taxi speed limit
                state = AirCraftState::taxi;
                break;
                
            case AirCraftState::taxi:
                if (direction == Direction::north || direction == Direction::south) {
                    state = AirCraftState::at_gate;
                    speed = createViolation ? 12 : 0; // Possibly violate gate speed limit
                } else {
                    state = AirCraftState::takeoff_roll;
                    speed = 0; // Start takeoff from standstill
                }
                break;
                
            case AirCraftState::at_gate:
                if (direction == Direction::east || direction == Direction::west) {
                    state = AirCraftState::taxi;
                    speed = createViolation ? 35 : (15 + (std::rand() % 16)); // Possibly violate taxi speed
                }
                break;
                
            case AirCraftState::takeoff_roll:
                state = AirCraftState::climb;
                if (createViolation) {
                    // Create a climb speed violation (over 463 km/h)
                    speed = 470 + (std::rand() % 50); // 470-520 km/h (over limit)
                } else {
                    speed = 250 + (std::rand() % 214); // 250-463 km/h (normal)
                }
                break;
                
            case AirCraftState::climb:
                state = AirCraftState::departure;
                if (createViolation) {
                    // Create a departure speed violation (outside 800-900 km/h)
                    if (rand() % 2 == 0) {
                        speed = 750 + (std::rand() % 50); // 750-799 km/h (under limit)
                    } else {
                        speed = 901 + (std::rand() % 50); // 901-950 km/h (over limit)
                    }
                } else {
                    speed = 800 + (std::rand() % 101); // 800-900 km/h (normal)
                }
                break;
                
            case AirCraftState::departure:
                // Flight has departed, nothing to do
                break;
        }
        
        // Update altitude based on new state
        altitude = getAltitude();
        
        // Log state changes (without emojis)
        std::cout << "State change for " << flightNumber << ": " << getStateString() 
                  << ", Speed: " << speed << ", Altitude: " << altitude << " ft\n";
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

    int getAltitude() const {
        // Return altitude based on aircraft state
        switch (state) {
            case AirCraftState::holding:
                return 15000; // Holding pattern altitude
            case AirCraftState::approach:
                return 5000;  // Initial approach
            case AirCraftState::landing:
                return 1000;  // Final approach/landing
            case AirCraftState::taxi:
                return 0;     // On ground
            case AirCraftState::at_gate:
                return 0;     // At gate
            case AirCraftState::takeoff_roll:
                return 0;     // Taking off (still on runway)
            case AirCraftState::climb:
                return 8000;  // Climbing after takeoff
            case AirCraftState::departure:
                return 30000; // Cruising altitude
            default:
                return 0;
        }
    }

};

int Flight::nextId = 1;