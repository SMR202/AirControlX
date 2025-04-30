#include <iostream>
#include <cstdlib>
#include <ctime>
#include "ATCSystem.hpp"

int main() {
    // Initialize random number generator
    srand(time(0));
    
    std::cout << "Starting AirControlX Simulation...\n";
    //std::cout << "Press Enter to begin the simulation (will run for 5 minutes).\n";
    //std::cin.get();
    
    ATCSystem atc;
    atc.startSimulation();
    
    std::cout << "Simulation completed.\n";
    return 0;
}