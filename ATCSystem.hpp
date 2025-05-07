#pragma once
#include "Airline.hpp"
#include "Flight.hpp"
#include "AVN.hpp"
#include "enums.hpp"
#include <iostream>
#include <vector>
#include <pthread.h>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>
#include <map>
#include <string>
#include <queue>
#include <algorithm>
#include "MsgStructs.hpp"
#include <cstring>

class ATCSystem{
public:
    int atcs_to_avn[2];
    int avn_to_atcs[2];
    
    std::vector<Airline> airlines;
    std::vector<Flight*> flights; //fcfs and priority based queue
    std::vector<AVN> avns;
    std::vector<AVN> TotalAVNs;
    std::vector<std::pair<Flight,time_t>> TotalFlights;
    std::map<std::string, int> violationsByAirline;
    pthread_mutex_t flightMutex;
    pthread_mutex_t runwayMutex;
    pthread_mutex_t avnMutex;

    bool runwayStatus[3]; // false means runway is available
    time_t simulationStartTime;
    bool simulationRunning;

    bool isRunwayAvailable(Runway runway){
        return !runwayStatus[static_cast<int>(runway)];
    }
    void occupyRunway(Runway runway){
        runwayStatus[static_cast<int>(runway)] = true;
    }
    void releaseRunway(Runway runway){
        runwayStatus[static_cast<int>(runway)] = false;
    }

    Runway assignRunway(Flight* flight){
        pthread_mutex_lock(&runwayMutex);

        if (flight->type == AirCraftType::cargo){
            if(isRunwayAvailable(Runway::RWY_C)){
                occupyRunway(Runway::RWY_C);
                pthread_mutex_unlock(&runwayMutex);
                return Runway::RWY_C;
            }
        }

        if (flight->type == AirCraftType::emergency){
            if (isRunwayAvailable(Runway::RWY_C)){
                occupyRunway(Runway::RWY_C);
                pthread_mutex_unlock(&runwayMutex);
                return Runway::RWY_C;
            }
        }

        if (flight->isArrival()){
            if (isRunwayAvailable(Runway::RWY_A)){
                occupyRunway(Runway::RWY_A);
                pthread_mutex_unlock(&runwayMutex);
                return Runway::RWY_A;
            }
        } 
        else if (flight->isDeparture()){
            if (isRunwayAvailable(Runway::RWY_B)){
                occupyRunway(Runway::RWY_B);
                pthread_mutex_unlock(&runwayMutex);
                return Runway::RWY_B;
            }
        }

        //no runway available?
        pthread_mutex_unlock(&runwayMutex);

        // If no runway is available, return the default runway based on direction
        // dont occupy runway, just return the default
        // (the flight will be queued until runway becomes available)
        if (flight->isArrival()){
            return Runway::RWY_A;
        } else {
            return Runway::RWY_B;
        }

    }

    void generateEmergency(){
        // Check probability based on direction
        for (auto flight : flights){    //(need to fix probabilities)
            int emergencyChance = rand() % 100 + 1; // Use a larger range for more granular control
            bool makeEmergency = false;

            switch (flight->direction){
                case Direction::north:
                    if (emergencyChance <= 10) makeEmergency = true; // 10% chance
                    break;
                case Direction::south:
                    if (emergencyChance <= 5) makeEmergency = true;  // 5% chance
                    break;
                case Direction::east:
                    if (emergencyChance <= 15) makeEmergency = true; // 15% chance
                    break;
                case Direction::west:
                    if (emergencyChance <= 20) makeEmergency = true; // 20% chance
                    break;
            }

            if (makeEmergency && !flight->isEmergency){
                pthread_mutex_lock(&flightMutex);
                // Double-check that flight still exists and isn't already an emergency
                bool flightExists = false;
                for (auto f : flights) {
                    if (f == flight) {
                        flightExists = true;
                        break;
                    }
                }
                
                if (flightExists && !flight->isEmergency) {
                    flight->isEmergency = true;
                    flight->priority = flight->calculatePriority();
                    std::cout << "❌ EMERGENCY DECLARED: Flight " << flight->flightNumber << " (" << flight->airline->name << ")\n";
                }
                pthread_mutex_unlock(&flightMutex);
            }
            sleep(1);
        }
    }

    void checkSpeedViolations() { //not being used
        pthread_mutex_lock(&flightMutex);
        
        for (auto flight : flights) {
            if (flight->speedViolation() && !flight->hasActiveAVN) {
                double allowedSpeed = 0;
                
                switch (flight->state) {
                    case AirCraftState::holding:
                        allowedSpeed = 600;
                        break;
                    case AirCraftState::approach:
                        allowedSpeed = 290;
                        break;
                    case AirCraftState::landing:
                        allowedSpeed = 240;
                        break;
                    case AirCraftState::taxi:
                        allowedSpeed = 30;
                        break;
                    case AirCraftState::at_gate:
                        allowedSpeed = 5;
                        break;
                    case AirCraftState::takeoff_roll:
                        allowedSpeed = 290;
                        break;
                    case AirCraftState::climb:
                        allowedSpeed = 463;
                        break;
                    case AirCraftState::departure:
                        allowedSpeed = 900;
                        break;
                }
                
                pthread_mutex_lock(&avnMutex);
                AVN avn(flight, flight->speed, allowedSpeed);
                avns.push_back(avn);
                violationsByAirline[avn.flight->airline->name]++;
                //TotalAVNs.push_back(avn);
                flight->hasActiveAVN = true;
                
                std::cout << "💸 AVN ISSUED: Flight " << flight->flightNumber << " (" << flight->airline->name << ") - Speed: " << flight->speed << " km/h, Allowed: " << allowedSpeed << " km/h, State: " << flight->getStateString() << "\n";
                pthread_mutex_unlock(&avnMutex);
            }
        }
        
        pthread_mutex_unlock(&flightMutex);
    }

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
    ATCSystem(int* atcs_to_avn, int* avn_to_atcs) {
        // Initialize pipe file descriptors
        this->atcs_to_avn[0] = atcs_to_avn[0];
        this->atcs_to_avn[1] = atcs_to_avn[1];
        this->avn_to_atcs[0] = avn_to_atcs[0];
        this->avn_to_atcs[1] = avn_to_atcs[1];

        pthread_mutex_init(&flightMutex, NULL);
        pthread_mutex_init(&runwayMutex, NULL);
        pthread_mutex_init(&avnMutex, NULL);

        runwayStatus[0] = false; // RWY_A
        runwayStatus[1] = false; // RWY_B
        runwayStatus[2] = false; // RWY_C

        airlines = {
            {"PIA", AirCraftType::commercial, 6, 4},
            {"AirBlue", AirCraftType::commercial, 4, 4},
            {"FedEx", AirCraftType::cargo, 3, 2},
            {"PAF", AirCraftType::emergency, 2, 1},
            {"BDart", AirCraftType::cargo, 2, 2},
            {"AK Amb", AirCraftType::emergency, 2, 1}
        };
        airlines[0].planeTexture.loadFromFile("Media/AirPlanes/PIA.png");
        airlines[1].planeTexture.loadFromFile("Media/AirPlanes/Airblue.png");
        airlines[2].planeTexture.loadFromFile("Media/AirPlanes/FedEx.png");
        airlines[3].planeTexture.loadFromFile("Media/AirPlanes/PAF.png");
        airlines[4].planeTexture.loadFromFile("Media/AirPlanes/BlueDart.png");
        airlines[5].planeTexture.loadFromFile("Media/AirPlanes/AirAmbulance.png");
        airlines[0].planeSprite.setTexture(airlines[0].planeTexture);
        airlines[1].planeSprite.setTexture(airlines[1].planeTexture);
        airlines[2].planeSprite.setTexture(airlines[2].planeTexture);
        airlines[3].planeSprite.setTexture(airlines[3].planeTexture);
        airlines[4].planeSprite.setTexture(airlines[4].planeTexture);
        airlines[5].planeSprite.setTexture(airlines[5].planeTexture);
        airlines[0].planeSprite.setScale(0.4f, 0.4f);
        airlines[1].planeSprite.setScale(0.4f, 0.4f);
        airlines[2].planeSprite.setScale(0.4f, 0.4f);
        airlines[3].planeSprite.setScale(0.4f, 0.4f);
        airlines[4].planeSprite.setScale(0.4f, 0.4f);
        airlines[5].planeSprite.setScale(0.4f, 0.4f);
        airlines[0].planeSprite.setOrigin(airlines[0].planeTexture.getSize().x / 2, airlines[0].planeTexture.getSize().y / 2);
        airlines[1].planeSprite.setOrigin(airlines[1].planeTexture.getSize().x / 2, airlines[1].planeTexture.getSize().y / 2);
        airlines[2].planeSprite.setOrigin(airlines[2].planeTexture.getSize().x / 2, airlines[2].planeTexture.getSize().y / 2);
        airlines[3].planeSprite.setOrigin(airlines[3].planeTexture.getSize().x / 2, airlines[3].planeTexture.getSize().y / 2);
        airlines[4].planeSprite.setOrigin(airlines[4].planeTexture.getSize().x / 2, airlines[4].planeTexture.getSize().y / 2);
        airlines[5].planeSprite.setOrigin(airlines[5].planeTexture.getSize().x / 2, airlines[5].planeTexture.getSize().y / 2);
        // airlines[0].planeSprite.setPosition(100, 100);
        // airlines[1].planeSprite.setPosition(200, 200);
        // airlines[2].planeSprite.setPosition(300, 300);
        // airlines[3].planeSprite.setPosition(400, 400);
        // airlines[4].planeSprite.setPosition(500, 500);
        // airlines[5].planeSprite.setPosition(600, 600);

        simulationRunning = false;
    }

    // Keep the default constructor for backward compatibility
    ATCSystem() {
        pthread_mutex_init(&flightMutex, NULL);
        pthread_mutex_init(&runwayMutex, NULL);
        pthread_mutex_init(&avnMutex, NULL);

        runwayStatus[0] = false; // RWY_A
        runwayStatus[1] = false; // RWY_B
        runwayStatus[2] = false; // RWY_C

        airlines = {
            {"PIA", AirCraftType::commercial, 6, 4},
            {"AirBlue", AirCraftType::commercial, 4, 4},
            {"FedEx", AirCraftType::cargo, 3, 2},
            {"PAF", AirCraftType::emergency, 2, 1},
            {"BDart", AirCraftType::cargo, 2, 2},
            {"AK Amb", AirCraftType::emergency, 2, 1}
        };
        airlines[0].planeTexture.loadFromFile("Media/AirPlanes/PIA.png");
        airlines[1].planeTexture.loadFromFile("Media/AirPlanes/Airblue.png");
        airlines[2].planeTexture.loadFromFile("Media/AirPlanes/FedEx.png");
        airlines[3].planeTexture.loadFromFile("Media/AirPlanes/PAF.png");
        airlines[4].planeTexture.loadFromFile("Media/AirPlanes/BlueDart.png");
        airlines[5].planeTexture.loadFromFile("Media/AirPlanes/AirAmbulance.png");
        airlines[0].planeSprite.setTexture(airlines[0].planeTexture);
        airlines[1].planeSprite.setTexture(airlines[1].planeTexture);
        airlines[2].planeSprite.setTexture(airlines[2].planeTexture);
        airlines[3].planeSprite.setTexture(airlines[3].planeTexture);
        airlines[4].planeSprite.setTexture(airlines[4].planeTexture);
        airlines[5].planeSprite.setTexture(airlines[5].planeTexture);
        airlines[0].planeSprite.setScale(0.4f, 0.4f);
        airlines[1].planeSprite.setScale(0.4f, 0.4f);
        airlines[2].planeSprite.setScale(0.4f, 0.4f);
        airlines[3].planeSprite.setScale(0.4f, 0.4f);
        airlines[4].planeSprite.setScale(0.4f, 0.4f);
        airlines[5].planeSprite.setScale(0.4f, 0.4f);
        airlines[0].planeSprite.setOrigin(airlines[0].planeTexture.getSize().x / 2, airlines[0].planeTexture.getSize().y / 2);
        airlines[1].planeSprite.setOrigin(airlines[1].planeTexture.getSize().x / 2, airlines[1].planeTexture.getSize().y / 2);
        airlines[2].planeSprite.setOrigin(airlines[2].planeTexture.getSize().x / 2, airlines[2].planeTexture.getSize().y / 2);
        airlines[3].planeSprite.setOrigin(airlines[3].planeTexture.getSize().x / 2, airlines[3].planeTexture.getSize().y / 2);
        airlines[4].planeSprite.setOrigin(airlines[4].planeTexture.getSize().x / 2, airlines[4].planeTexture.getSize().y / 2);
        airlines[5].planeSprite.setOrigin(airlines[5].planeTexture.getSize().x / 2, airlines[5].planeTexture.getSize().y / 2);
        // airlines[0].planeSprite.setPosition(100, 100);
        // airlines[1].planeSprite.setPosition(200, 200);
        // airlines[2].planeSprite.setPosition(300, 300);
        // airlines[3].planeSprite.setPosition(400, 400);
        // airlines[4].planeSprite.setPosition(500, 500);
        // airlines[5].planeSprite.setPosition(600, 600);
        simulationRunning = false;
    }

    ~ATCSystem(){

        for (auto flight : flights){
            delete flight;
        }
        
        pthread_mutex_destroy(&flightMutex);
        pthread_mutex_destroy(&runwayMutex);
        pthread_mutex_destroy(&avnMutex);
    }

    void startSimulation(){
        simulationRunning = true;
        simulationStartTime = time(0);

        createInitialFlights();

        // Start simulation threads
        pthread_t flightGeneratorThread, flightProcessorThread, displayThread, radarThread;
        pthread_create(&flightGeneratorThread, NULL, flightGeneratorThreadFunc, this);
        pthread_create(&flightProcessorThread, NULL, flightProcessorThreadFunc, this);
        pthread_create(&displayThread, NULL, displayThreadFunc, this);
        pthread_create(&radarThread, NULL, radarThreadFunc, this);

        // Run simulation for 5 minutes (300 seconds)
        sleep(300);

        // Stop simulation
        simulationRunning = false;

        // Join threads
        pthread_join(flightGeneratorThread, NULL);
        pthread_join(flightProcessorThread, NULL);
        pthread_join(displayThread, NULL);
        pthread_join(radarThread, NULL);

        displayFinalStats();

    }

    void createInitialFlights(){
        pthread_mutex_lock(&flightMutex);

        for (auto& airline : airlines){
            for (int i = 0 ; i < airline.flightsInOperation; i++){

                std::string flightNum = airline.name + "-" + std::to_string(100 + i);

                Direction direction = static_cast<Direction> (rand() % 4);

                Flight* flight = new Flight(flightNum, &airline, direction);
                flight->priority = flight->calculatePriority(); 
                flights.push_back(flight);
                
            }
        }
        pthread_mutex_unlock(&flightMutex);
    }

    void flightGeneratorLoop() {

        time_t northTime = time(0) + 180;
        time_t southTime = time(0) + 120;
        time_t eastTime = time(0) + 150;
        time_t westTime = time(0) + 240;
        
        while (simulationRunning) {
            time_t now = time(0);
            
            // Generate North arrivals (every 3 minutes)
            if (now >= northTime) {
                generateFlight(Direction::north);
                northTime = now + 180;
            }
            
            // Generate South arrivals (every 2 minutes)
            if (now >= southTime) {
                generateFlight(Direction::south);
                southTime = now + 120;
            }
            
            // Generate East departures (every 2.5 minutes)
            if (now >= eastTime) {
                generateFlight(Direction::east);
                eastTime = now + 150;
            }
            
            // Generate West departures (every 4 minutes)
            if (now >= westTime) {
                generateFlight(Direction::west);
                westTime = now + 240;
            }
            
            if (now % 60 == 0) { // Every minute
                generateEmergency();
            }
            
            usleep(500000); 
        }
    }

    void generateFlight(Direction dir) {
        pthread_mutex_lock(&flightMutex);
        
        Airline& airline = airlines[rand() % airlines.size()];
        
        if (airline.type == AirCraftType::cargo) {
            pthread_mutex_lock(&runwayMutex);
            bool runwayCAvailable = isRunwayAvailable(Runway::RWY_C);
            pthread_mutex_unlock(&runwayMutex);
            
            if (!runwayCAvailable) {
                pthread_mutex_unlock(&flightMutex);
                return;
            }
        }
        
        std::string flightNum = airline.name + "-" + std::to_string(200 + flights.size());
        
        bool isEmergency = false;
        int emergencyChance = rand() % 100 + 1;
        
        switch (dir) {
            case Direction::north:
                isEmergency = (emergencyChance <= 10); // 10% chance
                break;
            case Direction::south:
                isEmergency = (emergencyChance <= 5);  // 5% chance
                break;
            case Direction::east:
                isEmergency = (emergencyChance <= 15); // 15% chance
                break;
            case Direction::west:
                isEmergency = (emergencyChance <= 20); // 20% chance
                break;
        }
        
        Flight* flight = new Flight(flightNum, &airline, dir, isEmergency);
        
        flights.push_back(flight);

        
        std::cout << "NEW FLIGHT: " << flightNum << " (" << airline.name << ") - " 
                  << flight->getTypeString() << " - Direction: " << flight->getDirectionString();
        
        if (isEmergency) {
            std::cout << " - EMERGENCY";
        }
        
        std::cout << "\n";
        
        pthread_mutex_unlock(&flightMutex);
    }

    static bool comparisonFunction(Flight* a, Flight* b) {
        if (a->priority == b->priority) {
            return a->scheduleTime < b->scheduleTime; // Earlier scheduled flights first
        }
        return a->priority > b->priority; // Higher priority first
    }

    void flightProcessorLoop() {
        usleep(3000000); // 3 seconds to see initail states 
        while (simulationRunning) {
            pthread_mutex_lock(&flightMutex);

            //priority queue and fcfs
            std::sort(flights.begin(), flights.end(), comparisonFunction);

            // Process each flight
            for (auto it = flights.begin(); it != flights.end();) {
                Flight* flight = *it;
                bool shouldAdvanceIterator = true;
                bool stateChanged = false;
                
                // Process differently based on current state
                switch(flight->state) {
                    case AirCraftState::holding:
                        // Holding -> Approach (no runway needed)
                        flight->updateState();
                        stateChanged = true;
                        break;
                        
                    case AirCraftState::approach:
                        // Approach -> Landing (needs runway)
                        pthread_mutex_lock(&runwayMutex);
                        if (isRunwayAvailable(flight->runway)) {
                            occupyRunway(flight->runway);
                            pthread_mutex_unlock(&runwayMutex);
                            
                            flight->updateState();
                            stateChanged = true;
                            
                            std::cout << "Flight " << flight->flightNumber << " is landing on " 
                                      << flight->getRunwayString() << std::endl;
                        } else {
                            pthread_mutex_unlock(&runwayMutex);
                            // If runway not available, flight remains in approach state
                        }
                        break;
                        
                    case AirCraftState::landing:
                        // Landing -> Taxi (release runway after landing)
                        flight->updateState();
                        stateChanged = true;
                        
                        // Release runway after landing is complete
                        pthread_mutex_lock(&runwayMutex);
                        releaseRunway(flight->runway);
                        pthread_mutex_unlock(&runwayMutex);
                        
                        std::cout << "Flight " << flight->flightNumber 
                                  << " completed landing, runway " 
                                  << flight->getRunwayString() << " released" << std::endl;
                        break;
                        
                    case AirCraftState::taxi:
                        if (flight->isArrival()) {
                            // Taxi -> At Gate for arrivals
                            flight->updateState();
                            stateChanged = true;
                        } else {
                            // For departures, taxi -> takeoff_roll (needs runway)
                            pthread_mutex_lock(&runwayMutex);
                            if (isRunwayAvailable(flight->runway)) {
                                occupyRunway(flight->runway);
                                pthread_mutex_unlock(&runwayMutex);
                                
                                flight->updateState();
                                stateChanged = true;
                                
                                std::cout << "Flight " << flight->flightNumber 
                                          << " is taking off on " << flight->getRunwayString() << std::endl;
                            } else {
                                pthread_mutex_unlock(&runwayMutex);
                                // If runway not available, flight remains in taxi state
                            }
                        }
                        break;
                        
                    case AirCraftState::at_gate:
                        if (flight->isDeparture()) {
                            // At Gate -> Taxi for departures
                            flight->updateState();
                            stateChanged = true;
                        } else {
                            // For arrivals, this is a terminal state. Check if it's time to remove the flight
                            time_t now = time(nullptr);
                            static std::map<Flight*, time_t> arrivalCompletionTimes;
                            
                            if (arrivalCompletionTimes.find(flight) == arrivalCompletionTimes.end()) {
                                arrivalCompletionTimes[flight] = now + 15; // 15 seconds at gate before removal
                            } else if (now >= arrivalCompletionTimes[flight]) {
                                TotalFlights.push_back(std::make_pair(*flight, now));
                                delete flight;
                                it = flights.erase(it);
                                shouldAdvanceIterator = false;
                                
                                std::cout << "Arrival flight completed and removed from system" << std::endl;
                            }
                        }
                        break;
                        
                    case AirCraftState::takeoff_roll:
                        // Takeoff Roll -> Climb
                        flight->updateState();
                        stateChanged = true;
                        break;
                        
                    case AirCraftState::climb:
                        // Climb -> Departure (release runway after climbout)
                        flight->updateState();
                        stateChanged = true;
                        
                        // Release runway after takeoff is complete
                        pthread_mutex_lock(&runwayMutex);
                        releaseRunway(flight->runway);
                        pthread_mutex_unlock(&runwayMutex);
                        
                        std::cout << "Flight " << flight->flightNumber 
                                  << " completed takeoff, runway " 
                                  << flight->getRunwayString() << " released" << std::endl;
                        break;
                        
                    case AirCraftState::departure:
                        // For departures, this is a terminal state. Check if it's time to remove the flight
                        {
                            time_t now = time(nullptr);
                            static std::map<Flight*, time_t> departureCompletionTimes;
                            
                            if (departureCompletionTimes.find(flight) == departureCompletionTimes.end()) {
                                departureCompletionTimes[flight] = now + 10; // 10 seconds before departure removal
                            } else if (now >= departureCompletionTimes[flight]) {
                                TotalFlights.push_back(std::make_pair(*flight, now));
                                delete flight;
                                it = flights.erase(it);
                                shouldAdvanceIterator = false;
                                
                                std::cout << "Departure flight completed and removed from system" << std::endl;
                            }
                        }
                        break;
                }
                
                if (shouldAdvanceIterator) {
                    ++it;
                }
            }
            
            pthread_mutex_unlock(&flightMutex);
            
            // Sleep to avoid excessive CPU usage
            usleep(5000000); // 5 seconds -- Changed to 5 seconds so state is not changed rapidly
        }
    }

    void displayLoop() {
        while (simulationRunning) {
            #ifdef _WIN32
            system("cls");
            #else
            system("clear");
            #endif
            
            // Calculate elapsed time
            time_t now = time(0);
            int elapsed = static_cast<int>(difftime(now, simulationStartTime));
            std::string minutes = std::to_string(elapsed / 60);
            std::string seconds;
            if (elapsed % 60 < 10) {
                seconds = '0' + std::to_string(elapsed % 60);
            }
            else {
                seconds = std::to_string(elapsed % 60);
            }
            
            // Get current day and date
            char dateBuffer[64];
            struct tm* timeinfo = localtime(&now);
            strftime(dateBuffer, sizeof(dateBuffer), "%A, %B %d, %Y", timeinfo);
            
            std::cout << "==== AirControlX Simulation - Time: " 
                      << minutes << ":"
                      << seconds
                      << " / 5:00 ====\n";
            std::cout << "==== " << dateBuffer << " ====\n\n";
    
            // Store current flights for validation
            std::vector<Flight*> currentFlights;
            pthread_mutex_lock(&flightMutex);
            currentFlights = flights; // Make a copy of the pointers
            
            std::cout << "ACTIVE FLIGHTS: " << flights.size() << "\n";
            std::cout << "----------------------------------------------------------------------\n";
            std::cout << std::left << std::setw(15) << std::setfill(' ') << "Flight" 
                      << std::setw(15) << "Airline" 
                      << std::setw(12) << "Type" 
                      << std::setw(10) << "Direction" 
                      << std::setw(12) << "State" 
                      << std::setw(8) << "Speed" 
                      << std::setw(8) << "Runway" 
                      << "Emergency\n";
            std::cout << "----------------------------------------------------------------------\n";
                      
            for (auto flight : flights) {
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
            
            pthread_mutex_lock(&runwayMutex);
            std::cout << "\nRUNWAY STATUS:\n";
            std::cout << "RWY-A (North-South): " << (runwayStatus[0] ? "OCCUPIED" : "AVAILABLE") << "\n";
            std::cout << "RWY-B (East-West): " << (runwayStatus[1] ? "OCCUPIED" : "AVAILABLE") << "\n";
            std::cout << "RWY-C (Cargo/Emergency): " << (runwayStatus[2] ? "OCCUPIED" : "AVAILABLE") << "\n";
            pthread_mutex_unlock(&runwayMutex);
            
            // Display AVN information with safety checks
            pthread_mutex_lock(&avnMutex);
            std::cout << "\nISSUED AVNs: " << avns.size() << "\n";
            
            // Check for and remove dangling AVNs
            for (auto it = avns.begin(); it != avns.end();) {
                bool flightExists = false;
                
                // Check if the flight referenced by this AVN still exists
                for (auto flight : currentFlights) {
                    if (it->flight == flight) {
                        flightExists = true;
                        break;
                    }
                }
                
                if (!flightExists) {
                    // Flight no longer exists, remove this AVN
                    it = avns.erase(it);
                } else {
                    ++it;
                }
            }
            
            // Now display the valid AVNs
            if (!avns.empty()) {
                std::cout << "----------------------------------------------------------------------\n";
                std::cout << std::left << std::setw(10) << "AVN ID" 
                          << std::setw(15) << "Flight" 
                          << std::setw(15) << "Airline" 
                          << std::setw(15) << "Recorded Speed" 
                          << "Allowed Speed\n";
                std::cout << "----------------------------------------------------------------------\n";
                
                for (const auto& avn : avns) {
                    // Additional safety check
                    bool flightValid = false;
                    for (auto flight : currentFlights) {
                        if (avn.flight == flight) {
                            flightValid = true;
                            break;
                        }
                    }
                    
                    if (flightValid) {
                        std::cout << std::left << std::setw(10) << "AVN-" + std::to_string(avn.id)
                                  << std::setw(15) << avn.flight->flightNumber
                                  << std::setw(15) << avn.flight->airline->name
                                  << std::setw(15) << avn.recordedSpeed
                                  << avn.allowedSpeed << "\n";
                    } else {
                        std::cout << std::left << std::setw(10) << "AVN-" + std::to_string(avn.id)
                                  << std::setw(15) << "[deleted]"
                                  << std::setw(15) << "[deleted]"
                                  << std::setw(15) << avn.recordedSpeed
                                  << avn.allowedSpeed << "\n";
                    }
                }
            }
            pthread_mutex_unlock(&avnMutex);
            
            sleep(1);
        }
    }

    void radarLoop() {

        const int radarInterval = 200000; // 200ms
        
        while (simulationRunning) {
            pthread_mutex_lock(&flightMutex);
            
            for (auto flight : flights) {
                if (flight->speedViolation() && !flight->hasActiveAVN) {
                    issueSpeedViolationAVN(flight);
                }
            }
            
            pthread_mutex_unlock(&flightMutex);
            
            usleep(radarInterval);
        }
    }
    
    void issueSpeedViolationAVN(Flight* flight) {
        double allowedSpeed = 0;
        
        switch (flight->state) {
            case AirCraftState::holding:
                allowedSpeed = 600;
                break;
            case AirCraftState::approach:
                allowedSpeed = 290;
                break;
            case AirCraftState::landing:
                allowedSpeed = 240;
                break;
            case AirCraftState::taxi:
                allowedSpeed = 30;
                break;
            case AirCraftState::at_gate:
                allowedSpeed = 5;
                break;
            case AirCraftState::takeoff_roll:
                allowedSpeed = 290;
                break;
            case AirCraftState::climb:
                allowedSpeed = 463;
                break;
            case AirCraftState::departure:
                allowedSpeed = 900;
                break;
        }
        
        pthread_mutex_lock(&avnMutex);
        AVN avn(flight, flight->speed, allowedSpeed);
        avns.push_back(avn);
        violationsByAirline[avn.flight->airline->name]++;
        flight->hasActiveAVN = true;
        
        // Properly copy strings to char arrays
        AVNNotice avnToGenerate;
        avnToGenerate.avnId = avn.id;
        
        strncpy(avnToGenerate.aircraftId, flight->flightNumber.c_str(), sizeof(avnToGenerate.aircraftId) - 1);
        avnToGenerate.aircraftId[sizeof(avnToGenerate.aircraftId) - 1] = '\0'; // Ensure null termination
        
        strncpy(avnToGenerate.AirlineName, flight->airline->name.c_str(), sizeof(avnToGenerate.AirlineName) - 1);
        avnToGenerate.AirlineName[sizeof(avnToGenerate.AirlineName) - 1] = '\0'; // Ensure null termination
        
        strncpy(avnToGenerate.aircraftType, flight->getTypeString().c_str(), sizeof(avnToGenerate.aircraftType) - 1);
        avnToGenerate.aircraftType[sizeof(avnToGenerate.aircraftType) - 1] = '\0'; // Ensure null termination
        
        strncpy(avnToGenerate.flightNumber, flight->flightNumber.c_str(), sizeof(avnToGenerate.flightNumber) - 1);
        avnToGenerate.flightNumber[sizeof(avnToGenerate.flightNumber) - 1] = '\0'; // Ensure null termination
        
        avnToGenerate.recordedSpeed = flight->speed;
        avnToGenerate.allowedSpeed = allowedSpeed;
        if (flight->type == AirCraftType::commercial) {
            avnToGenerate.totalFine = 500000 * 1.15;
        } else if (flight->type == AirCraftType::cargo) {
            avnToGenerate.totalFine = 700000 * 1.15;
        } else if (flight->type == AirCraftType::emergency) {
            avnToGenerate.totalFine = 100000 * 1.15;
        }
        avnToGenerate.timestamp = time(0);
        
        // Don't close the read end, and don't close the write end here
        // Only write to the pipe
        write(atcs_to_avn[1], &avnToGenerate, sizeof(avnToGenerate));
        
        std::cout << "AVN ISSUED: Flight " << flight->flightNumber 
                  << " (" << flight->airline->name << ") - Speed Violation: " 
                  << flight->speed << " km/h, Allowed: " << allowedSpeed 
                  << " km/h, State: " << flight->getStateString() << "\n";
        pthread_mutex_unlock(&avnMutex);
    }

    
    void displayFinalStats() {
        std::cout << "\n==== AirControlX Simulation Final Statistics ====\n";
        
        pthread_mutex_lock(&flightMutex);
        pthread_mutex_lock(&avnMutex);
        
        // Make a copy of current flights for validation
        std::vector<Flight*> currentFlights = flights;
        
        std::cout << "Total Flights Processed: " << TotalFlights.size() << "\n";
        
        // Clean up AVNs with invalid flight pointers
        for (auto it = avns.begin(); it != avns.end();) {
            bool flightExists = false;
            for (auto flight : currentFlights) {
                if (it->flight == flight) {
                    flightExists = true;
                    break;
                }
            }
            
            if (!flightExists) {
                it = avns.erase(it);
            } else {
                ++it;
            }
        }
        
        std::cout << "Total AVNs Issued: " << violationsByAirline.size() << "\n";
        
        // Display table of all processed flights with details
        std::cout << "\n==== FLIGHT HISTORY TABLE ====\n";
        std::cout << "------------------------------------------------------------------------------------------------------------------------------------------------\n";
        std::cout << std::left << std::setw(15) << "Flight" 
                  << std::setw(20) << "Airline" 
                  << std::setw(12) << "Type" 
                  << std::setw(12) << "Direction" 
                  << std::setw(12) << "Final State"
                  << std::setw(15) << "Priority"
                  << std::setw(20) << "Schedule Time"
                  << std::setw(20) << "Completion Time"
                  << std::setw(15) << "Wait Time (s)"
                  << "Emergency\n";
        std::cout << "------------------------------------------------------------------------------------------------------------------------------------------------\n";
        
        for (const auto& flightPair : TotalFlights) {
            const Flight& flight = flightPair.first;
            time_t completionTime = flightPair.second;
            time_t waitTime = completionTime - flight.scheduleTime;
            
            char scheduleTimeBuffer[26];
            char completionTimeBuffer[26];
            
            // Format the time values
            std::strftime(scheduleTimeBuffer, 26, "%H:%M:%S", std::localtime(&flight.scheduleTime));
            std::strftime(completionTimeBuffer, 26, "%H:%M:%S", std::localtime(&completionTime));
            
            std::cout << std::left << std::setw(15) << flight.flightNumber 
                      << std::setw(20) << flight.airline->name 
                      << std::setw(12) << flight.getTypeString()
                      << std::setw(12) << flight.getDirectionString() 
                      << std::setw(12) << flight.getStateString() 
                      << std::setw(15) << flight.priority
                      << std::setw(20) << scheduleTimeBuffer
                      << std::setw(20) << completionTimeBuffer
                      << std::setw(15) << waitTime
                      << (flight.isEmergency ? "YES" : "NO")
                      << (flight.hasActiveAVN ? " (AVN)" : "") << "\n";
        }
        std::cout << "------------------------------------------------------------------------------------------------------------------------------------------------\n";
        
        // Add statistics about average wait times by priority and type
        std::map<int, std::pair<int, int>> waitTimesByPriority; // priority -> (total wait time, count)
        std::map<AirCraftType, std::pair<int, int>> waitTimesByType; // type -> (total wait time, count)
        
        for (const auto& flightPair : TotalFlights) {
            const Flight& flight = flightPair.first;
            time_t completionTime = flightPair.second;
            int waitTime = static_cast<int>(completionTime - flight.scheduleTime);
            
            waitTimesByPriority[flight.priority].first += waitTime;
            waitTimesByPriority[flight.priority].second++;
            
            waitTimesByType[flight.type].first += waitTime;
            waitTimesByType[flight.type].second++;
        }
        
        std::cout << "\n==== AVERAGE WAIT TIMES ====\n";
        std::cout << "\nBy Priority:\n";
        for (const auto& pair : waitTimesByPriority) {
            int avgWaitTime = pair.second.first / (pair.second.second > 0 ? pair.second.second : 1);
            std::cout << "Priority " << pair.first << ": " << avgWaitTime << " seconds\n";
        }
        
        std::cout << "\nBy Aircraft Type:\n";
        for (const auto& pair : waitTimesByType) {
            int avgWaitTime = pair.second.first / (pair.second.second > 0 ? pair.second.second : 1);
            std::string typeStr;
            switch(pair.first) {
                case AirCraftType::commercial: typeStr = "Commercial"; break;
                case AirCraftType::cargo: typeStr = "Cargo"; break;
                case AirCraftType::emergency: typeStr = "Emergency"; break;
                default: typeStr = "Unknown";
            }
            std::cout << typeStr << ": " << avgWaitTime << " seconds\n";
        }
        
        std::cout << "\nAVNs by Airline:\n";
        for (const auto& pair : violationsByAirline) {
            std::cout << pair.first << ": " << pair.second << "\n";
        }
        
        pthread_mutex_unlock(&avnMutex);
        pthread_mutex_unlock(&flightMutex);
        
        std::cout << "\nSimulation completed." << std::endl;
    }

    // New accessor methods for SFML integration
    
    // Get runway status (thread-safe)
    bool getRunwayStatus(int index) {
        pthread_mutex_lock(&runwayMutex);
        bool status = runwayStatus[index];
        pthread_mutex_unlock(&runwayMutex);
        return status;
    }
    
    // Get flight count (thread-safe)
    size_t getFlightCount() {
        pthread_mutex_lock(&flightMutex);
        size_t count = flights.size();
        pthread_mutex_unlock(&flightMutex);
        return count;
    }
    
    // Get AVN count (thread-safe)
    size_t getAVNCount() {
        pthread_mutex_lock(&avnMutex);
        size_t count = avns.size();
        pthread_mutex_unlock(&avnMutex);
        return count;
    }
    
    // Get simulation running status
    bool isSimulationRunning() {
        return simulationRunning;
    }
    
    // Additional accessor methods for SFML UI integration
    
    // Get a thread-safe copy of the flights vector
    std::vector<Flight*> getFlightsCopy() {
        pthread_mutex_lock(&flightMutex);
        std::vector<Flight*> flightsCopy = flights;
        pthread_mutex_unlock(&flightMutex);
        return flightsCopy;
    }
    
    // Get a thread-safe copy of the AVNs vector
    std::vector<AVN> getAVNsCopy() {
        pthread_mutex_lock(&avnMutex);
        std::vector<AVN> avnsCopy = avns;
        pthread_mutex_unlock(&avnMutex);
        return avnsCopy;
    }
    
    // Get simulation start time
    time_t getSimulationStartTime() {
        return simulationStartTime;
    }
};