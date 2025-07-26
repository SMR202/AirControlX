# AirControlX

Automated Air Traffic Control System

## Overview
AirControlX is a simulation-based project for managing air traffic control operations, designed for an Operating Systems final project. It models real-world ATC scenarios, including flight arrivals, departures, runway management, and emergency handling. The project features both a terminal-based simulation and a graphical user interface (GUI) built with SFML, providing a visual and interactive experience for users.

## Key Features
- **Flight Management:** Tracks active flights, including airline, type (commercial/cargo), direction, state (holding, approach, landing, taxi, at gate, departure), speed, assigned runway, and emergency status.
- **Runway Status:** Manages three runways:
  - RWY-A (North-South): Commercial/General
  - RWY-B (East-West): Commercial/General
  - RWY-C (Cargo/Emergency): Dedicated for cargo and emergencies
- **State Transitions:** Flights progress through states such as holding, approach, landing, taxi, and at gate. Runway occupancy and release are tracked in real time.
- **AVN Issuance:** Tracks the number of Air Vehicle Notices (AVNs) issued during simulation.
- **Emergency Handling:** Dedicated runway and state tracking for emergency scenarios.
- **Graphical Interface:** Includes a menu and simulation UI built with SFML, featuring custom graphics and sounds for an immersive ATC experience.

## Example Simulation Output
```
==== AirControlX Simulation - Time: 3:01 / 5:00 ====

ACTIVE FLIGHTS: 2
----------------------------------------------------------------------
Flight         Airline        Type        Direction State       Speed   Runway  Emergency
----------------------------------------------------------------------
Blue Dart-201  Blue Dart      Cargo       East      Departure   819     RWY-C   NO
PIA-201        PIA            Commercial  North     Holding     497     RWY-A   NO

RUNWAY STATUS:
RWY-A (North-South): AVAILABLE
RWY-B (East-West): AVAILABLE
RWY-C (Cargo/Emergency): AVAILABLE

ISSUED AVNs: 0
```

*Note: The project supports both terminal and graphical simulation modes. The graphical mode displays real-time flight and runway status with visual elements and sound effects.*

## How It Works
- Flights are created and managed by the ATC system, with each flight assigned a state and runway.
- Runway status is updated as flights land, take off, or taxi.
- The simulation logs all state changes, arrivals, departures, and emergencies.

## Project Structure
- `source.cpp` and headers: Main simulation logic and ATC system implementation
- `Media/`: Contains images and sounds for the SFML-based menu and simulation UI
- `simulation_output.txt`: Example output log from a simulation run
- `sfml_menu/` and `SFML_stuff.hpp`: Source files for the graphical interface and menu system

## Getting Started
1. Ensure you have a C++ compiler and SFML installed.
2. Run `./compile.sh` to build the project.
3. Execute the simulation binary to start the ATC simulation.
4. For the graphical mode, ensure SFML dependencies are available and launch the SFML-based menu to interact with the simulation visually.

## Authors
Developed by SMR202 and team for the Operating Systems final project.
