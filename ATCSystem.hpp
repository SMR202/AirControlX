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


class ATCSystem{
private:
    std::vector<Airline> airlines;
    std::vector<Flight*> flights;
    std::vector<AVN> avns;

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
        int emergencyChance = rand() % 100 + 1;

        // Check probability based on direction
        for (auto flight : flights){
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
                flight->isEmergency = true;
                flight->priority = flight->calculatePriority();
                std::cout << "❌ EMERGENCY DECLARED: Flight " << flight->flightNumber << " (" << flight->airline->name << ")\n";
                pthread_mutex_unlock(&flightMutex);
            }
        }
    }

    void checkSpeedViolations() {
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
    ATCSystem(){

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
            {"Pakistan Airforce", AirCraftType::emergency, 2, 1},
            {"Blue Dart", AirCraftType::cargo, 2, 2},
            {"AghaKhan Air Ambulance", AirCraftType::emergency, 2, 1}
        };

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
            
            generateEmergency();
            
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

    void flightProcessorLoop() {
        while (simulationRunning) {
            pthread_mutex_lock(&flightMutex);
            
            for (auto it = flights.begin(); it != flights.end();) {
                Flight* flight = *it;
                
                // Check if flight needs a runway
                if ((flight->state == AirCraftState::approach && flight->isArrival()) || 
                    (flight->state == AirCraftState::taxi && flight->isDeparture())) {
                    
                    Runway runway = assignRunway(flight);
                    flight->runway = runway;
                    
                    pthread_mutex_lock(&runwayMutex);
                    bool runwayAvailable = isRunwayAvailable(runway);
                    pthread_mutex_unlock(&runwayMutex);
                    
                    if (runwayAvailable) {
                        flight->updateState();

                        std::cout<<"🤑🤑🤑🤑FLIGHT " << flight->flightNumber << " (" << flight->airline->name << ") - "
                                 << flight->getStateString() << " - Runway: " << flight->getRunwayString() 
                                 << "\n";
                        
                        // Release runway after use
                        if ((flight->state == AirCraftState::taxi && flight->isArrival()) || 
                            (flight->state == AirCraftState::climb && flight->isDeparture())) {
                            pthread_mutex_lock(&runwayMutex);
                            releaseRunway(flight->runway);
                            pthread_mutex_unlock(&runwayMutex);
                        }
                    }
                } 
                else {
                    // Update state for flights not needing a runway
                    flight->updateState();
                }
                
                if ((flight->state == AirCraftState::at_gate && flight->isArrival()) || 
                    (flight->state == AirCraftState::departure && flight->isDeparture())) {
                    
                    // Allow for some time at gate before removing
                    static std::map<Flight*, time_t> completionTimes;
                    
                    if (completionTimes.find(flight) == completionTimes.end()) {
                        completionTimes[flight] = time(nullptr) + 10;
                    } 
                    else if (time(0) >= completionTimes[flight]) {
                        delete flight;
                        it = flights.erase(it);
                        continue;
                    }

                }
                
                ++it;
            }
            
            pthread_mutex_unlock(&flightMutex);
            
            usleep(1000000); //1
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
            
            std::cout << "==== AirControlX Simulation - Time: " 
                      << elapsed / 60 << ":" << std::setw(2) << std::setfill('0') << elapsed % 60 
                      << " / 5:00 ====\n\n";
    
            pthread_mutex_lock(&flightMutex);
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
            
            // Display AVN information
            pthread_mutex_lock(&avnMutex);
            std::cout << "\nISSUED AVNs: " << avns.size() << "\n";
            if (!avns.empty()) {
                std::cout << "----------------------------------------------------------------------\n";
                std::cout << std::left << std::setw(10) << "AVN ID" 
                          << std::setw(15) << "Flight" 
                          << std::setw(15) << "Airline" 
                          << std::setw(15) << "Recorded Speed" 
                          << "Allowed Speed\n";
                std::cout << "----------------------------------------------------------------------\n";
                
                for (const auto& avn : avns) {
                    std::cout << std::left << std::setw(10) << "AVN-" + std::to_string(avn.id)
                              << std::setw(15) << avn.flight->flightNumber
                              << std::setw(15) << avn.flight->airline->name
                              << std::setw(15) << avn.recordedSpeed
                              << avn.allowedSpeed << "\n";
                }
            }
            pthread_mutex_unlock(&avnMutex);
            
            sleep(2);
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
        flight->hasActiveAVN = true;
        
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
        
        std::cout << "Total Flights Processed: " << flights.size() + avns.size() << "\n";
        std::cout << "Total AVNs Issued: " << avns.size() << "\n";
        
        // Count violations by airline
        std::map<std::string, int> violationsByAirline;
        for (const auto& avn : avns) {
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