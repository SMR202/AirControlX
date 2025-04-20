#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <pthread.h>
#include <time.h>
#include <iomanip>
#include <unistd.h>
#include <map>
#include <cstdlib>
#include <ctime>

// Define aircraft types
enum class AircraftType {
    COMMERCIAL,
    CARGO,
    EMERGENCY
};

// Define flight direction
enum class Direction {
    NORTH,
    SOUTH,
    EAST,
    WEST
};

// Define aircraft state
enum class AircraftState {
    HOLDING,
    APPROACH,
    LANDING,
    TAXI,
    AT_GATE,
    TAKEOFF_ROLL,
    CLIMB,
    DEPARTURE
};

// Define runway
enum class Runway {
    RWY_A,  // North-South alignment (arrivals)
    RWY_B,  // East-West alignment (departures)
    RWY_C   // Flexible for cargo/emergency/overflow
};

// Define airline
struct Airline {
    std::string name;
    AircraftType type;
    int totalAircrafts;
    int flightsInOperation;
};

// Define flight
class Flight {
private:
    static int nextId;

public:
    int id;
    std::string flightNumber;
    Airline* airline;
    AircraftType type;
    Direction direction;
    AircraftState state;
    double speed;
    bool isEmergency;
    bool hasActiveAVN;
    Runway assignedRunway;
    time_t scheduleTime;
    int priority; // Higher number means higher priority

    Flight(std::string flightNum, Airline* air, AircraftType t, Direction dir, bool emergency = false) {
        id = nextId++;
        flightNumber = flightNum;
        airline = air;
        type = t;
        direction = dir;
        isEmergency = emergency;
        hasActiveAVN = false;
        priority = calculatePriority();
        
        // Initialize state and speed based on direction (arrival or departure)
        if (dir == Direction::NORTH || dir == Direction::SOUTH) {
            state = AircraftState::HOLDING;
            // Set initial speed for holding pattern
            speed = 400 + (rand() % 201); // 400-600 km/h
        } else {
            state = AircraftState::AT_GATE;
            speed = 0; // At gate, not moving
        }
    }

    int calculatePriority() {
        int basePriority = 0;
        
        // Emergency flights get highest priority
        if (isEmergency)
            return 100;
            
        // Type-based priority
        switch (type) {
            case AircraftType::EMERGENCY:
                basePriority = 80;
                break;
            case AircraftType::CARGO:
                basePriority = 50;
                break;
            case AircraftType::COMMERCIAL:
                basePriority = 30;
                break;
        }
        
        return basePriority;
    }

    bool checkSpeedViolation() {
        bool violation = false;
        
        switch (state) {
            case AircraftState::HOLDING:
                violation = (speed > 600);
                break;
            case AircraftState::APPROACH:
                violation = (speed < 240 || speed > 290);
                break;
            case AircraftState::LANDING:
                // For landing, we need to check if speed is appropriate for the landing phase
                // This is simplified for now
                violation = (speed > 240 || speed < 30);
                break;
            case AircraftState::TAXI:
                violation = (speed > 30);
                break;
            case AircraftState::AT_GATE:
                violation = (speed > 5);
                break;
            case AircraftState::TAKEOFF_ROLL:
                violation = (speed > 290);
                break;
            case AircraftState::CLIMB:
                violation = (speed > 463);
                break;
            case AircraftState::DEPARTURE:
                violation = (speed < 800 || speed > 900);
                break;
        }
        
        return violation;
    }

    void updateState() {
        // Simulate state transitions
        switch (state) {
            case AircraftState::HOLDING:
                state = AircraftState::APPROACH;
                speed = 240 + (rand() % 51); // 240-290 km/h
                break;
                
            case AircraftState::APPROACH:
                state = AircraftState::LANDING;
                speed = 240; // Start landing at 240 km/h
                break;
                
            case AircraftState::LANDING:
                speed = 30; // Slow down to taxi speed
                state = AircraftState::TAXI;
                break;
                
            case AircraftState::TAXI:
                if (direction == Direction::NORTH || direction == Direction::SOUTH) {
                    state = AircraftState::AT_GATE;
                    speed = 0;
                } else {
                    state = AircraftState::TAKEOFF_ROLL;
                    speed = 0; // Start takeoff from standstill
                }
                break;
                
            case AircraftState::AT_GATE:
                if (direction == Direction::EAST || direction == Direction::WEST) {
                    state = AircraftState::TAXI;
                    speed = 15 + (rand() % 16); // 15-30 km/h
                }
                break;
                
            case AircraftState::TAKEOFF_ROLL:
                state = AircraftState::CLIMB;
                speed = 250 + (rand() % 214); // 250-463 km/h
                break;
                
            case AircraftState::CLIMB:
                state = AircraftState::DEPARTURE;
                speed = 800 + (rand() % 101); // 800-900 km/h
                break;
                
            case AircraftState::DEPARTURE:
                // Flight has departed, nothing to do
                break;
        }
    }
    
    std::string getStateString() const {
        switch (state) {
            case AircraftState::HOLDING: return "Holding";
            case AircraftState::APPROACH: return "Approach";
            case AircraftState::LANDING: return "Landing";
            case AircraftState::TAXI: return "Taxi";
            case AircraftState::AT_GATE: return "At Gate";
            case AircraftState::TAKEOFF_ROLL: return "Takeoff Roll";
            case AircraftState::CLIMB: return "Climb";
            case AircraftState::DEPARTURE: return "Departure";
            default: return "Unknown";
        }
    }
    
    std::string getDirectionString() const {
        switch (direction) {
            case Direction::NORTH: return "North";
            case Direction::SOUTH: return "South";
            case Direction::EAST: return "East";
            case Direction::WEST: return "West";
            default: return "Unknown";
        }
    }
    
    std::string getRunwayString() const {
        switch (assignedRunway) {
            case Runway::RWY_A: return "RWY-A";
            case Runway::RWY_B: return "RWY-B";
            case Runway::RWY_C: return "RWY-C";
            default: return "None";
        }
    }
    
    std::string getTypeString() const {
        switch (type) {
            case AircraftType::COMMERCIAL: return "Commercial";
            case AircraftType::CARGO: return "Cargo";
            case AircraftType::EMERGENCY: return "Emergency";
            default: return "Unknown";
        }
    }
    
    bool isArrival() const {
        return direction == Direction::NORTH || direction == Direction::SOUTH;
    }
    
    bool isDeparture() const {
        return direction == Direction::EAST || direction == Direction::WEST;
    }
};

int Flight::nextId = 1;

// AVN (Airspace Violation Notice)
struct AVN {
    int id;
    Flight* flight;
    double recordedSpeed;
    double allowedSpeed;
    time_t issueTime;
    bool isPaid;
    
    AVN(Flight* f, double recorded, double allowed) {
        static int nextId = 1;
        id = nextId++;
        flight = f;
        recordedSpeed = recorded;
        allowedSpeed = allowed;
        issueTime = time(nullptr);
        isPaid = false;
    }
};

// ATC (Air Traffic Control) System
class ATCSystem {
private:
    std::vector<Airline> airlines;
    std::vector<Flight*> activeFlights;
    std::vector<AVN> issuedAVNs;
    
    pthread_mutex_t flightMutex;
    pthread_mutex_t runwayMutex;
    pthread_mutex_t avnMutex;
    
    bool runwayStatus[3]; // false means runway is available
    time_t simulationStartTime;
    bool simulationRunning;
    
    // Helper methods
    bool isRunwayAvailable(Runway runway) {
        return !runwayStatus[static_cast<int>(runway)];
    }
    
    void occupyRunway(Runway runway) {
        runwayStatus[static_cast<int>(runway)] = true;
    }
    
    void releaseRunway(Runway runway) {
        runwayStatus[static_cast<int>(runway)] = false;
    }
    
    Runway assignRunway(Flight* flight) {
        pthread_mutex_lock(&runwayMutex);
        
        // For cargo flights, always try to use RWY-C first
        if (flight->type == AircraftType::CARGO) {
            if (isRunwayAvailable(Runway::RWY_C)) {
                occupyRunway(Runway::RWY_C);
                pthread_mutex_unlock(&runwayMutex);
                return Runway::RWY_C;
            }
        }
        
        // For emergency flights, choose first available runway
        if (flight->isEmergency) {
            if (isRunwayAvailable(Runway::RWY_A)) {
                occupyRunway(Runway::RWY_A);
                pthread_mutex_unlock(&runwayMutex);
                return Runway::RWY_A;
            }
            if (isRunwayAvailable(Runway::RWY_B)) {
                occupyRunway(Runway::RWY_B);
                pthread_mutex_unlock(&runwayMutex);
                return Runway::RWY_B;
            }
            if (isRunwayAvailable(Runway::RWY_C)) {
                occupyRunway(Runway::RWY_C);
                pthread_mutex_unlock(&runwayMutex);
                return Runway::RWY_C;
            }
        }
        
        // Normal allocation based on direction
        if (flight->isArrival()) {
            if (isRunwayAvailable(Runway::RWY_A)) {
                occupyRunway(Runway::RWY_A);
                pthread_mutex_unlock(&runwayMutex);
                return Runway::RWY_A;
            }
        } else if (flight->isDeparture()) {
            if (isRunwayAvailable(Runway::RWY_B)) {
                occupyRunway(Runway::RWY_B);
                pthread_mutex_unlock(&runwayMutex);
                return Runway::RWY_B;
            }
        }
        
        // Use RWY-C as overflow if available
        if (isRunwayAvailable(Runway::RWY_C)) {
            occupyRunway(Runway::RWY_C);
            pthread_mutex_unlock(&runwayMutex);
            return Runway::RWY_C;
        }
        
        pthread_mutex_unlock(&runwayMutex);
        
        // If no runway is available, return the default runway based on direction
        // (the flight will be queued until runway becomes available)
        if (flight->isArrival()) {
            return Runway::RWY_A;
        } else {
            return Runway::RWY_B;
        }
    }
    
    void generateEmergency() {
        int emergencyChance = rand() % 100 + 1;
        
        // Check probability based on direction
        for (auto flight : activeFlights) {
            bool makeEmergency = false;
            
            switch (flight->direction) {
                case Direction::NORTH:
                    if (emergencyChance <= 10) makeEmergency = true; // 10% chance
                    break;
                case Direction::SOUTH:
                    if (emergencyChance <= 5) makeEmergency = true;  // 5% chance
                    break;
                case Direction::EAST:
                    if (emergencyChance <= 15) makeEmergency = true; // 15% chance
                    break;
                case Direction::WEST:
                    if (emergencyChance <= 20) makeEmergency = true; // 20% chance
                    break;
            }
            
            if (makeEmergency && !flight->isEmergency) {
                pthread_mutex_lock(&flightMutex);
                flight->isEmergency = true;
                flight->priority = flight->calculatePriority();
                std::cout << "EMERGENCY DECLARED: Flight " << flight->flightNumber << " (" << flight->airline->name << ")\n";
                pthread_mutex_unlock(&flightMutex);
            }
        }
    }
    
    void checkSpeedViolations() {
        pthread_mutex_lock(&flightMutex);
        
        for (auto flight : activeFlights) {
            if (flight->checkSpeedViolation() && !flight->hasActiveAVN) {
                double allowedSpeed = 0;
                
                // Determine allowed speed range based on state
                switch (flight->state) {
                    case AircraftState::HOLDING:
                        allowedSpeed = 600;
                        break;
                    case AircraftState::APPROACH:
                        allowedSpeed = 290;
                        break;
                    case AircraftState::LANDING:
                        allowedSpeed = 240;
                        break;
                    case AircraftState::TAXI:
                        allowedSpeed = 30;
                        break;
                    case AircraftState::AT_GATE:
                        allowedSpeed = 5;
                        break;
                    case AircraftState::TAKEOFF_ROLL:
                        allowedSpeed = 290;
                        break;
                    case AircraftState::CLIMB:
                        allowedSpeed = 463;
                        break;
                    case AircraftState::DEPARTURE:
                        allowedSpeed = 900; // Using upper bound
                        break;
                }
                
                pthread_mutex_lock(&avnMutex);
                AVN avn(flight, flight->speed, allowedSpeed);
                issuedAVNs.push_back(avn);
                flight->hasActiveAVN = true;
                
                std::cout << "AVN ISSUED: Flight " << flight->flightNumber 
                          << " (" << flight->airline->name << ") - Speed: " 
                          << flight->speed << " km/h, Allowed: " << allowedSpeed 
                          << " km/h, State: " << flight->getStateString() << "\n";
                pthread_mutex_unlock(&avnMutex);
            }
        }
        
        pthread_mutex_unlock(&flightMutex);
    }

    // Thread functions - these need to be static for pthread compatibility
    static void* flightGeneratorThreadFunc(void* arg) {
        ATCSystem* atc = static_cast<ATCSystem*>(arg);
        atc->flightGeneratorLoop();
        return nullptr;
    }
    
    static void* flightProcessorThreadFunc(void* arg) {
        ATCSystem* atc = static_cast<ATCSystem*>(arg);
        atc->flightProcessorLoop();
        return nullptr;
    }
    
    static void* displayThreadFunc(void* arg) {
        ATCSystem* atc = static_cast<ATCSystem*>(arg);
        atc->displayLoop();
        return nullptr;
    }
    
    static void* radarThreadFunc(void* arg) {
        ATCSystem* atc = static_cast<ATCSystem*>(arg);
        atc->radarLoop();
        return nullptr;
    }

public:
    ATCSystem() {
        // Initialize mutexes
        pthread_mutex_init(&flightMutex, nullptr);
        pthread_mutex_init(&runwayMutex, nullptr);
        pthread_mutex_init(&avnMutex, nullptr);
        
        // Initialize runways
        runwayStatus[0] = false;
        runwayStatus[1] = false;
        runwayStatus[2] = false;
        
        // Initialize airlines
        airlines = {
            {"PIA", AircraftType::COMMERCIAL, 6, 4},
            {"AirBlue", AircraftType::COMMERCIAL, 4, 4},
            {"FedEx", AircraftType::CARGO, 3, 2},
            {"Pakistan Airforce", AircraftType::EMERGENCY, 2, 1},
            {"Blue Dart", AircraftType::CARGO, 2, 2},
            {"AghaKhan Air Ambulance", AircraftType::EMERGENCY, 2, 1}
        };
        
        simulationRunning = false;
    }
    
    ~ATCSystem() {
        // Clean up any active flights
        for (auto flight : activeFlights) {
            delete flight;
        }
        
        // Destroy mutexes
        pthread_mutex_destroy(&flightMutex);
        pthread_mutex_destroy(&runwayMutex);
        pthread_mutex_destroy(&avnMutex);
    }
    
    void startSimulation() {
        simulationStartTime = time(nullptr);
        simulationRunning = true;
        
        // Create initial flights
        createInitialFlights();
        
        // Start simulation threads
        pthread_t flightGeneratorThread, flightProcessorThread, displayThread, radarThread;
        pthread_create(&flightGeneratorThread, nullptr, flightGeneratorThreadFunc, this);
        pthread_create(&flightProcessorThread, nullptr, flightProcessorThreadFunc, this);
        pthread_create(&displayThread, nullptr, displayThreadFunc, this);
        pthread_create(&radarThread, nullptr, radarThreadFunc, this);
        
        // Run simulation for 5 minutes (300 seconds)
        sleep(300);
        
        // Stop simulation
        simulationRunning = false;
        
        // Join threads
        pthread_join(flightGeneratorThread, nullptr);
        pthread_join(flightProcessorThread, nullptr);
        pthread_join(displayThread, nullptr);
        pthread_join(radarThread, nullptr);
        
        // Display final statistics
        displayFinalStats();
    }
    
    void createInitialFlights() {
        pthread_mutex_lock(&flightMutex);
        
        // Create initial flights for each airline
        for (auto& airline : airlines) {
            for (int i = 0; i < airline.flightsInOperation; i++) {
                // Create flight number
                std::string flightNum = airline.name + "-" + std::to_string(100 + i);
                
                // Determine direction randomly (N/S for arrivals, E/W for departures)
                Direction dir;
                
                if (airline.type == AircraftType::CARGO) {
                    // Cargo flights can be arrivals or departures
                    dir = static_cast<Direction>((rand() % 2) * 2 + (rand() % 2));
                } else if (airline.type == AircraftType::EMERGENCY) {
                    // Emergency flights are more likely to be arrivals
                    dir = static_cast<Direction>(rand() % 2);
                } else {
                    // Commercial flights can be arrivals or departures
                    dir = static_cast<Direction>((rand() % 2) * 2 + (rand() % 2));
                }
                
                // Create flight
                Flight* flight = new Flight(flightNum, &airline, airline.type, dir);
                
                // For emergency type airlines, set emergency flag
                if (airline.type == AircraftType::EMERGENCY) {
                    flight->isEmergency = true;
                    flight->priority = flight->calculatePriority();
                }
                
                // Add to active flights
                activeFlights.push_back(flight);
            }
        }
        
        pthread_mutex_unlock(&flightMutex);
    }
    
    void flightGeneratorLoop() {
        // Generate flights according to specified intervals
        time_t northTime = time(nullptr) + 180;
        time_t southTime = time(nullptr) + 120;
        time_t eastTime = time(nullptr) + 150;
        time_t westTime = time(nullptr) + 240;
        
        while (simulationRunning) {
            time_t now = time(nullptr);
            
            // Generate North arrivals (every 3 minutes)
            if (now >= northTime) {
                generateFlight(Direction::NORTH);
                northTime = now + 180;
            }
            
            // Generate South arrivals (every 2 minutes)
            if (now >= southTime) {
                generateFlight(Direction::SOUTH);
                southTime = now + 120;
            }
            
            // Generate East departures (every 2.5 minutes)
            if (now >= eastTime) {
                generateFlight(Direction::EAST);
                eastTime = now + 150;
            }
            
            // Generate West departures (every 4 minutes)
            if (now >= westTime) {
                generateFlight(Direction::WEST);
                westTime = now + 240;
            }
            
            // Generate random emergencies
            generateEmergency();
            
            // Sleep for a bit to prevent CPU overuse
            usleep(500000); // 500ms in microseconds
        }
    }
    
    void generateFlight(Direction dir) {
        pthread_mutex_lock(&flightMutex);
        
        // Select a random airline
        Airline& airline = airlines[rand() % airlines.size()];
        
        // If cargo flight, ensure it uses RWY-C
        if (airline.type == AircraftType::CARGO) {
            pthread_mutex_lock(&runwayMutex);
            bool runwayCAvailable = isRunwayAvailable(Runway::RWY_C);
            pthread_mutex_unlock(&runwayMutex);
            
            if (!runwayCAvailable) {
                // Skip generating this flight if RWY-C is not available
                pthread_mutex_unlock(&flightMutex);
                return;
            }
        }
        
        // Create flight number
        std::string flightNum = airline.name + "-" + std::to_string(200 + activeFlights.size());
        
        // Determine if this is an emergency flight
        bool isEmergency = false;
        int emergencyChance = rand() % 100 + 1;
        
        switch (dir) {
            case Direction::NORTH:
                isEmergency = (emergencyChance <= 10); // 10% chance
                break;
            case Direction::SOUTH:
                isEmergency = (emergencyChance <= 5);  // 5% chance
                break;
            case Direction::EAST:
                isEmergency = (emergencyChance <= 15); // 15% chance
                break;
            case Direction::WEST:
                isEmergency = (emergencyChance <= 20); // 20% chance
                break;
        }
        
        // Create flight
        Flight* flight = new Flight(flightNum, &airline, airline.type, dir, isEmergency);
        
        // Add to active flights
        activeFlights.push_back(flight);
        
        std::cout << "NEW FLIGHT: " << flightNum << " (" << airline.name << ") - " 
                  << flight->getTypeString() << " - Direction: " << flight->getDirectionString();
        
        if (isEmergency) {
            std::cout << " - EMERGENCY";
        }
        
        std::cout << "\n";
        
        pthread_mutex_unlock(&flightMutex);
    }
    
    void flightProcessorLoop() {
        while (simulationRunning) {
            pthread_mutex_lock(&flightMutex);
            
            // Process each active flight
            for (auto it = activeFlights.begin(); it != activeFlights.end();) {
                Flight* flight = *it;
                
                // Check if flight needs a runway
                if ((flight->state == AircraftState::APPROACH && flight->isArrival()) || 
                    (flight->state == AircraftState::TAXI && flight->isDeparture())) {
                    
                    // Assign runway if needed
                    Runway runway = assignRunway(flight);
                    flight->assignedRunway = runway;
                    
                    // Check if runway is available 
                    pthread_mutex_lock(&runwayMutex);
                    bool runwayAvailable = isRunwayAvailable(runway);
                    pthread_mutex_unlock(&runwayMutex);
                    
                    // If runway is available, proceed with state update
                    if (runwayAvailable) {
                        flight->updateState();
                        
                        // Release runway after use
                        if ((flight->state == AircraftState::TAXI && flight->isArrival()) || 
                            (flight->state == AircraftState::CLIMB && flight->isDeparture())) {
                            pthread_mutex_lock(&runwayMutex);
                            releaseRunway(flight->assignedRunway);
                            pthread_mutex_unlock(&runwayMutex);
                        }
                    }
                } else {
                    // Update state for flights not needing a runway
                    flight->updateState();
                }
                
                // Remove flights that have completed their journey
                if ((flight->state == AircraftState::AT_GATE && flight->isArrival()) || 
                    (flight->state == AircraftState::DEPARTURE && flight->isDeparture())) {
                    
                    // Allow for some time at gate before removing
                    static std::map<Flight*, time_t> completionTimes;
                    
                    if (completionTimes.find(flight) == completionTimes.end()) {
                        completionTimes[flight] = time(nullptr) + 10;
                    } else if (time(nullptr) >= completionTimes[flight]) {
                        delete flight;
                        it = activeFlights.erase(it);
                        continue;
                    }

                }
                
                ++it;
            }
            
            pthread_mutex_unlock(&flightMutex);
            
            // Sleep to prevent CPU overuse
            usleep(1000000); // 1 second in microseconds
        }
    }
    
    void displayLoop() {
        while (simulationRunning) {
            // Clear screen for better display (platform dependent)
            #ifdef _WIN32
            system("cls");
            #else
            system("clear");
            #endif
            
            // Calculate elapsed time
            time_t now = time(nullptr);
            int elapsed = static_cast<int>(difftime(now, simulationStartTime));
            
            // Display simulation time
            std::cout << "==== AirControlX Simulation - Time: " 
                      << elapsed / 60 << ":" << std::setw(2) << std::setfill('0') << elapsed % 60 
                      << " / 5:00 ====\n\n";
            
            // Display active flights
            pthread_mutex_lock(&flightMutex);
            std::cout << "ACTIVE FLIGHTS: " << activeFlights.size() << "\n";
            std::cout << "----------------------------------------------------------------------\n";
            std::cout << std::left << std::setw(15) << "Flight" 
                      << std::setw(15) << "Airline" 
                      << std::setw(12) << "Type" 
                      << std::setw(10) << "Direction" 
                      << std::setw(12) << "State" 
                      << std::setw(8) << "Speed" 
                      << std::setw(8) << "Runway" 
                      << "Emergency\n";
            std::cout << "----------------------------------------------------------------------\n";
                      
            for (auto flight : activeFlights) {
                std::cout << std::left << std::setw(15) << flight->flightNumber 
                          << std::setw(15) << flight->airline->name 
                          << std::setw(12) << flight->getTypeString()
                          << std::setw(10) << flight->getDirectionString() 
                          << std::setw(12) << flight->getStateString() 
                          << std::setw(8) << flight->speed
                          << std::setw(8) << flight->getRunwayString()
                          << (flight->isEmergency ? "YES" : "NO") 
                          << (flight->hasActiveAVN ? " (AVN)" : "") << "\n";
            }
            pthread_mutex_unlock(&flightMutex);
            
            // Display runway status
            pthread_mutex_lock(&runwayMutex);
            std::cout << "\nRUNWAY STATUS:\n";
            std::cout << "RWY-A (North-South): " << (runwayStatus[0] ? "OCCUPIED" : "AVAILABLE") << "\n";
            std::cout << "RWY-B (East-West): " << (runwayStatus[1] ? "OCCUPIED" : "AVAILABLE") << "\n";
            std::cout << "RWY-C (Cargo/Emergency): " << (runwayStatus[2] ? "OCCUPIED" : "AVAILABLE") << "\n";
            pthread_mutex_unlock(&runwayMutex);
            
            // Display AVN information
            pthread_mutex_lock(&avnMutex);
            std::cout << "\nISSUED AVNs: " << issuedAVNs.size() << "\n";
            if (!issuedAVNs.empty()) {
                std::cout << "----------------------------------------------------------------------\n";
                std::cout << std::left << std::setw(10) << "AVN ID" 
                          << std::setw(15) << "Flight" 
                          << std::setw(15) << "Airline" 
                          << std::setw(15) << "Recorded Speed" 
                          << "Allowed Speed\n";
                std::cout << "----------------------------------------------------------------------\n";
                
                for (const auto& avn : issuedAVNs) {
                    std::cout << std::left << std::setw(10) << "AVN-" + std::to_string(avn.id)
                              << std::setw(15) << avn.flight->flightNumber
                              << std::setw(15) << avn.flight->airline->name
                              << std::setw(15) << avn.recordedSpeed
                              << avn.allowedSpeed << "\n";
                }
            }
            pthread_mutex_unlock(&avnMutex);
            
            // Wait for 2 seconds before refreshing
            sleep(2);
        }
    }
    
    void radarLoop() {
        // Radar updates 5 times per second
        const int radarInterval = 200000; // 200ms in microseconds
        
        while (simulationRunning) {
            pthread_mutex_lock(&flightMutex);
            
            for (auto flight : activeFlights) {
                // Check for speed violations
                if (flight->checkSpeedViolation() && !flight->hasActiveAVN) {
                    issueSpeedViolationAVN(flight);
                }
                
                // Check for airspace boundary violations
                checkAirspaceBoundaryViolation(flight);
                
                // Check for phase-specific restrictions
                checkPhaseViolation(flight);
            }
            
            pthread_mutex_unlock(&flightMutex);
            
            // Sleep until next radar sweep (200ms for 5 times per second)
            usleep(radarInterval);
        }
    }
    
    void issueSpeedViolationAVN(Flight* flight) {
        double allowedSpeed = 0;
        
        // Determine allowed speed range based on state
        switch (flight->state) {
            case AircraftState::HOLDING:
                allowedSpeed = 600;
                break;
            case AircraftState::APPROACH:
                allowedSpeed = 290;
                break;
            case AircraftState::LANDING:
                allowedSpeed = 240;
                break;
            case AircraftState::TAXI:
                allowedSpeed = 30;
                break;
            case AircraftState::AT_GATE:
                allowedSpeed = 5;
                break;
            case AircraftState::TAKEOFF_ROLL:
                allowedSpeed = 290;
                break;
            case AircraftState::CLIMB:
                allowedSpeed = 463;
                break;
            case AircraftState::DEPARTURE:
                allowedSpeed = 900; // Using upper bound
                break;
        }
        
        pthread_mutex_lock(&avnMutex);
        AVN avn(flight, flight->speed, allowedSpeed);
        issuedAVNs.push_back(avn);
        flight->hasActiveAVN = true;
        
        std::cout << "AVN ISSUED: Flight " << flight->flightNumber 
                  << " (" << flight->airline->name << ") - Speed Violation: " 
                  << flight->speed << " km/h, Allowed: " << allowedSpeed 
                  << " km/h, State: " << flight->getStateString() << "\n";
        pthread_mutex_unlock(&avnMutex);
    }
    
    void checkAirspaceBoundaryViolation(Flight* flight) {
        // In a real system, this would check geographical coordinates
        // For simulation purposes, we'll use a simplified model
        
        // Check if flight is in departure state but not assigned a departure runway
        if (flight->state == AircraftState::DEPARTURE && 
            (flight->direction == Direction::EAST || flight->direction == Direction::WEST) &&
            flight->assignedRunway != Runway::RWY_B && 
            flight->assignedRunway != Runway::RWY_C &&
            !flight->hasActiveAVN) {
            
            pthread_mutex_lock(&avnMutex);
            AVN avn(flight, 0, 0); // Using 0 for speeds as this is not a speed violation
            issuedAVNs.push_back(avn);
            flight->hasActiveAVN = true;
            
            std::cout << "AVN ISSUED: Flight " << flight->flightNumber 
                      << " (" << flight->airline->name << ") - Airspace Boundary Violation: "
                      << "Incorrect departure path for assigned runway"
                      << ", State: " << flight->getStateString() << "\n";
            pthread_mutex_unlock(&avnMutex);
        }
        
        // Check if flight is in arrival state but not in arrival airspace
        if ((flight->state == AircraftState::HOLDING || flight->state == AircraftState::APPROACH) && 
            !(flight->direction == Direction::NORTH || flight->direction == Direction::SOUTH) &&
            !flight->hasActiveAVN) {
            
            pthread_mutex_lock(&avnMutex);
            AVN avn(flight, 0, 0); // Using 0 for speeds as this is not a speed violation
            issuedAVNs.push_back(avn);
            flight->hasActiveAVN = true;
            
            std::cout << "AVN ISSUED: Flight " << flight->flightNumber 
                      << " (" << flight->airline->name << ") - Airspace Boundary Violation: "
                      << "Aircraft in wrong airspace sector for its phase"
                      << ", State: " << flight->getStateString() << "\n";
            pthread_mutex_unlock(&avnMutex);
        }
    }
    
    void checkPhaseViolation(Flight* flight) {
        // Check if flight is in an incompatible phase for its aircraft type
        // For example, cargo aircraft may have restrictions on certain approaches
        
        if (flight->type == AircraftType::CARGO && 
            flight->state == AircraftState::APPROACH && 
            flight->direction == Direction::NORTH &&
            flight->assignedRunway != Runway::RWY_C &&
            !flight->hasActiveAVN) {
            
            pthread_mutex_lock(&avnMutex);
            AVN avn(flight, 0, 0);
            issuedAVNs.push_back(avn);
            flight->hasActiveAVN = true;
            
            std::cout << "AVN ISSUED: Flight " << flight->flightNumber 
                      << " (" << flight->airline->name << ") - Phase Violation: "
                      << "Cargo aircraft using restricted approach path"
                      << ", State: " << flight->getStateString() << "\n";
            pthread_mutex_unlock(&avnMutex);
        }
        
        // Check for state sequence violations (e.g. skipping mandatory states)
        // This is a simplified example - a real system would have more complex rules
        if (flight->state == AircraftState::LANDING && 
            flight->speed > 200 &&  // Aircraft is still at cruising speed
            !flight->hasActiveAVN) {
            
            pthread_mutex_lock(&avnMutex);
            AVN avn(flight, flight->speed, 200);
            issuedAVNs.push_back(avn);
            flight->hasActiveAVN = true;
            
            std::cout << "AVN ISSUED: Flight " << flight->flightNumber 
                      << " (" << flight->airline->name << ") - Phase Transition Violation: "
                      << "Attempted landing at unsafe speed: " << flight->speed << " km/h"
                      << ", State: " << flight->getStateString() << "\n";
            pthread_mutex_unlock(&avnMutex);
        }
    }
    
    void displayFinalStats() {
        std::cout << "\n==== AirControlX Simulation Final Statistics ====\n";
        
        pthread_mutex_lock(&flightMutex);
        pthread_mutex_lock(&avnMutex);
        
        std::cout << "Total Flights Processed: " << activeFlights.size() + issuedAVNs.size() << "\n";
        std::cout << "Total AVNs Issued: " << issuedAVNs.size() << "\n";
        
        // Count violations by airline
        std::map<std::string, int> violationsByAirline;
        for (const auto& avn : issuedAVNs) {
            violationsByAirline[avn.flight->airline->name]++;
        }
        
        std::cout << "\nAVNs by Airline:\n";
        for (const auto& pair : violationsByAirline) {
            std::cout << pair.first << ": " << pair.second << "\n";
        }
        
        pthread_mutex_unlock(&avnMutex);
        pthread_mutex_unlock(&flightMutex);
    }
};

int main() {
    // Initialize random number generator
    srand(time(nullptr));
    
    std::cout << "Starting AirControlX Simulation...\n";
    std::cout << "Press Enter to begin the simulation (will run for 5 minutes).\n";
    std::cin.get();
    
    ATCSystem atc;
    atc.startSimulation();
    
    std::cout << "Simulation completed.\n";
    return 0;
}