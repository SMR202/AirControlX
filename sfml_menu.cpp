#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include "ATCSystem.hpp"
#include "SFML_stuff.hpp"
#include "AVNGenerator.hpp"
#include "AirlinePortal.hpp"
#include "StripePayment.hpp"
#include <sys/types.h>
#include <sys/wait.h>

// Global ATCSystem instance that can be accessed by the SFML interface
ATCSystem* ATCS = nullptr;
pthread_mutex_t atcMutex = PTHREAD_MUTEX_INITIALIZER;
bool simulationRunning = false;
pthread_t simulationThreadId;
AVNGenerator *avnGen = nullptr;
AirlinePortal *airlinePortal = nullptr;
StripePayment *stripePayment = nullptr;

pid_t avnGen_id;
pid_t airlinePortal_id;
pid_t stripePayment_id;

const float speed = 2;


// Pipe file descriptors
int atcs_to_avn[2];      // ATCS Controller to AVN Generator
int avn_to_airline[2];   // AVN Generator to Airline Portal
int avn_to_stripe[2];    // AVN Generator to StripePay Process
int stripe_to_avn[2];    // StripePay Process to AVN Generator
int stripe_to_airline[2]; // StripePay Process to Airline Portal
int avn_to_atcs[2];      // AVN Generator to ATCS Controller


// Thread function to run the simulation
void* runSimulationThread(void* arg) {
    std::cout << "Starting simulation in background thread..." << std::endl;
    
    // Start the simulation - this will block until simulation completes
    ATCS->startSimulation();
    
    // When simulation is done
    pthread_mutex_lock(&atcMutex);
    simulationRunning = false;
    std::cout << "Simulation thread completed" << std::endl;
    pthread_mutex_unlock(&atcMutex);
    
    return nullptr;
}

// Enhanced SimulationView class that shows real ATCSystem data
class EnhancedSimulationView : public SimulationView {
private:
    sf::Font statsFont;
    sf::Text timerText;
    sf::Text flightCountText;
    sf::Text avnCountText;
    sf::RectangleShape flightTableBg;
    sf::RectangleShape avnTableBg;
    
    // Runway visualization
    std::vector<sf::Text> runwayTexts;
    std::vector<sf::RectangleShape> runwayShapes;
    
    // Flight table
    sf::Text flightTableTitle;
    std::vector<sf::Text> flightTableHeaders;
    std::vector<std::vector<sf::Text>> flightTableRows;
    
    // AVN table
    sf::Text avnTableTitle;
    std::vector<sf::Text> avnTableHeaders;
    std::vector<std::vector<sf::Text>> avnTableRows;
    
    bool dataInitialized;
    int maxVisibleFlights;
    int maxVisibleAVNs;
    float scrollOffsetFlights;
    float scrollOffsetAVNs;
    
    time_t lastUpdateTime;

public:
    EnhancedSimulationView(sf::RenderWindow& win) : SimulationView(win), dataInitialized(false), 
        maxVisibleFlights(15), maxVisibleAVNs(5), scrollOffsetFlights(0), scrollOffsetAVNs(0),
        lastUpdateTime(0) {
        
        // Load font
        if (!statsFont.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf")) {
            if (!statsFont.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
                std::cerr << "Failed to load font in EnhancedSimulationView!" << std::endl;
            }
        }
        
        sf::Vector2u windowSize = win.getSize();
        
        // Timer text
        timerText.setFont(statsFont);
        timerText.setCharacterSize(36);
        timerText.setFillColor(sf::Color::White);
        timerText.setPosition(windowSize.x * 0.5f - 100, 20);
        timerText.setString("00:00 / 5:00");
        
        // Flight count text
        flightCountText.setFont(statsFont);
        flightCountText.setCharacterSize(24);
        flightCountText.setFillColor(sf::Color::White);
        flightCountText.setPosition(420, 80);
        flightCountText.setString("Active Flights: 0");
        
        // AVN count text
        avnCountText.setFont(statsFont);
        avnCountText.setCharacterSize(24);
        avnCountText.setFillColor(sf::Color::Yellow);
        avnCountText.setPosition(420, 110);
        avnCountText.setString("Active AVNs: 0");
        
        // Initialize runway visualization
        float runwayWidth = 500.0f;
        float runwayHeight = 50.0f;
        float spacing = 20.0f;
        
        // Create runway shapes
        for (int i = 0; i < 3; i++) {
            sf::RectangleShape runway(sf::Vector2f(runwayWidth, runwayHeight));
            runway.setPosition(windowSize.x/2 - runwayWidth/2, 120.0f + i * (runwayHeight + spacing));
            runway.setFillColor(sf::Color(50, 50, 50));
            runway.setOutlineColor(sf::Color::White);
            runway.setOutlineThickness(2.0f);
            runwayShapes.push_back(runway);
            
            sf::Text runwayText;
            runwayText.setFont(statsFont);
            runwayText.setCharacterSize(18);
            runwayText.setFillColor(sf::Color::White);
            
            switch(i) {
                case 0: runwayText.setString("RWY-A (North-South): AVAILABLE"); break;
                case 1: runwayText.setString("RWY-B (East-West): AVAILABLE"); break;
                case 2: runwayText.setString("RWY-C (Cargo/Emergency): AVAILABLE"); break;
            }
            
            runwayText.setPosition(windowSize.x/2 - runwayWidth/2 + 10, 
                                  120.0f + i * (runwayHeight + spacing) + runwayHeight/2 - 9.0f);
            runwayTexts.push_back(runwayText);
        }
        
        // Flight table background
        flightTableBg.setSize(sf::Vector2f(windowSize.x - 800, maxVisibleFlights * 30 + 60));
        flightTableBg.setPosition(400, 350);
        flightTableBg.setFillColor(sf::Color(30, 30, 50, 200));
        flightTableBg.setOutlineColor(sf::Color(100, 100, 150));
        flightTableBg.setOutlineThickness(2.0f);
        
        // Flight table title
        flightTableTitle.setFont(statsFont);
        flightTableTitle.setCharacterSize(20);
        flightTableTitle.setFillColor(sf::Color::White);
        flightTableTitle.setPosition(410, 360);
        flightTableTitle.setString("ACTIVE FLIGHTS");
        
        // Flight table headers - adding Altitude column
        std::vector<std::string> flightHeaders = {"Flight", "Airline", "Type", "Direction", "State", "Speed", "Altitude", "Runway", "Emergency"};
        float headerX = 410;
        float columnWidth = 110; // Adjusted column width to fit the new column
        
        for (const auto& header : flightHeaders) {
            sf::Text headerText;
            headerText.setFont(statsFont);
            headerText.setCharacterSize(16);
            headerText.setFillColor(sf::Color(200, 200, 255));
            headerText.setPosition(headerX, 390);
            headerText.setString(header);
            flightTableHeaders.push_back(headerText);
            
            headerX += columnWidth;
        }
        
        // AVN table background
        avnTableBg.setSize(sf::Vector2f(windowSize.x - 800, maxVisibleAVNs * 30 + 60));
        avnTableBg.setPosition(400, 350 + flightTableBg.getSize().y + 20);
        avnTableBg.setFillColor(sf::Color(30, 30, 50, 200));
        avnTableBg.setOutlineColor(sf::Color(100, 100, 150));
        avnTableBg.setOutlineThickness(2.0f);
        
        // AVN table title
        avnTableTitle.setFont(statsFont);
        avnTableTitle.setCharacterSize(20);
        avnTableTitle.setFillColor(sf::Color::Yellow);
        avnTableTitle.setPosition(410, 360 + flightTableBg.getSize().y + 20);
        avnTableTitle.setString("ISSUED AVNs");
        
        // AVN table headers
        std::vector<std::string> avnHeaders = {"AVN ID", "Flight", "Airline", "Rec Speed", "Allowed Speed"};
        headerX = 410;
        
        for (const auto& header : avnHeaders) {
            sf::Text headerText;
            headerText.setFont(statsFont);
            headerText.setCharacterSize(16);
            headerText.setFillColor(sf::Color(255, 255, 200));
            headerText.setPosition(headerX, 390 + flightTableBg.getSize().y + 20);
            headerText.setString(header);
            avnTableHeaders.push_back(headerText);
            
            headerX += columnWidth;
        }
    }
    
    // Handle mouse wheel scrolling for the flight and AVN tables
    void handleScroll(float delta, sf::Vector2i mousePos) {
        // Check if mouse is over flight table
        if (flightTableBg.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
            scrollOffsetFlights += delta * 30.0f; // Adjust scroll speed
            if (scrollOffsetFlights < 0) scrollOffsetFlights = 0;
            
            // Limit maximum scroll based on number of flights
            float maxScroll = std::max(0.0f, static_cast<float>((flightTableRows.size() - maxVisibleFlights) * 30));
            if (scrollOffsetFlights > maxScroll) scrollOffsetFlights = maxScroll;
        }
        // Check if mouse is over AVN table
        else if (avnTableBg.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
            scrollOffsetAVNs += delta * 30.0f; // Adjust scroll speed
            if (scrollOffsetAVNs < 0) scrollOffsetAVNs = 0;
            
            // Limit maximum scroll based on number of AVNs
            float maxScroll = std::max(0.0f, static_cast<float>((avnTableRows.size() - maxVisibleAVNs) * 30));
            if (scrollOffsetAVNs > maxScroll) scrollOffsetAVNs = maxScroll;
        }
    }
    
    void update2()  {
        if (!simulationRunning || ATCS == nullptr) {
            return;
        }
        
        // Only update every 200ms to avoid excessive updates
        time_t now = time(nullptr);
        if (now - lastUpdateTime < 0.2 && dataInitialized) {
            return;
        }
        lastUpdateTime = now;
        
        pthread_mutex_lock(&atcMutex);
        
        // Update timer
        time_t elapsedTime = difftime(now, ATCS->getSimulationStartTime());
        int minutes = elapsedTime / 60;
        int seconds = elapsedTime % 60;
        
        char timerBuffer[20];
        sprintf(timerBuffer, "%02d:%02d / 5:00", minutes, seconds);
        timerText.setString(timerBuffer);
        
        // Update runway status visualization
        for (int i = 0; i < 3 && i < runwayShapes.size(); i++) {
            bool isOccupied = ATCS->getRunwayStatus(i);
            std::string runwayName;
            
            switch(i) {
                case 0: runwayName = "RWY-A (North-South): "; break;
                case 1: runwayName = "RWY-B (East-West): "; break;
                case 2: runwayName = "RWY-C (Cargo/Emergency): "; break;
            }
            
            if (isOccupied) {
                runwayShapes[i].setFillColor(sf::Color(180, 0, 0, 200));
                runwayTexts[i].setString(runwayName + "OCCUPIED");
            } else {
                runwayShapes[i].setFillColor(sf::Color(0, 180, 0, 200));
                runwayTexts[i].setString(runwayName + "AVAILABLE");
            }
        }
        
        // Update flight count
        size_t flightCount = ATCS->getFlightCount();
        flightCountText.setString("Active Flights: " + std::to_string(flightCount));
        
        // Update AVN count
        size_t avnCount = ATCS->getAVNCount();
        avnCountText.setString("Active AVNs: " + std::to_string(avnCount));
        
        // Get flight data and update flight table using the safe accessor method
        flightTableRows.clear();
        
        // Use the thread-safe accessor method to get flight data
        std::vector<Flight*> currentFlights = ATCS->getFlightsCopy();
        
        for (auto flight : currentFlights) {
            std::vector<sf::Text> row;
            
            // Flight number
            sf::Text cellText;
            cellText.setFont(statsFont);
            cellText.setCharacterSize(14);
            cellText.setFillColor(sf::Color::White);
            cellText.setString(flight->flightNumber);
            row.push_back(cellText);
            
            // Airline
            cellText.setString(flight->airline->name);
            row.push_back(cellText);
            
            // Type
            cellText.setString(flight->getTypeString());
            row.push_back(cellText);
            
            // Direction
            cellText.setString(flight->getDirectionString());
            row.push_back(cellText);
            
            // State
            cellText.setString(flight->getStateString());
            row.push_back(cellText);
            
            // Speed
            cellText.setString(std::to_string(static_cast<int>(flight->speed)) + " km/h");
            row.push_back(cellText);
            
            // Altitude
            std::string altitudeStr = (flight->altitude == 0) ? 
                "Ground" : std::to_string(flight->altitude) + " ft";
            cellText.setString(altitudeStr);
            row.push_back(cellText);
            
            // Runway
            cellText.setString(flight->getRunwayString());
            row.push_back(cellText);
            
            // Emergency + AVN status
            std::string emergencyStatus = flight->isEmergency ? "YES" : "NO";
            if (flight->hasActiveAVN) {
                emergencyStatus += " (AVN)";
                cellText.setFillColor(sf::Color::Yellow);
            } else if (flight->isEmergency) {
                cellText.setFillColor(sf::Color::Red);
            } else {
                cellText.setFillColor(sf::Color::White);
            }
            cellText.setString(emergencyStatus);
            row.push_back(cellText);
            
            flightTableRows.push_back(row);
        }
        
        // Get AVN data and update AVN table using the safe accessor method
        avnTableRows.clear();
        
        // Use the thread-safe accessor method to get AVN data
        std::vector<AVN> currentAVNs = ATCS->getAVNsCopy();
        
        for (const auto& avn : currentAVNs) {
            // Skip invalid AVNs
            bool flightValid = false;
            for (auto flight : currentFlights) {
                if (avn.flight == flight) {
                    flightValid = true;
                    break;
                }
            }
            
            if (!flightValid) continue;
            
            std::vector<sf::Text> row;
            
            // AVN ID
            sf::Text cellText;
            cellText.setFont(statsFont);
            cellText.setCharacterSize(14);
            cellText.setFillColor(sf::Color::Yellow);
            cellText.setString("AVN-" + std::to_string(avn.id));
            row.push_back(cellText);
            
            // Flight number
            cellText.setString(avn.flight->flightNumber);
            row.push_back(cellText);
            
            // Airline
            cellText.setString(avn.flight->airline->name);
            row.push_back(cellText);
            
            // Recorded speed
            cellText.setString(std::to_string(static_cast<int>(avn.recordedSpeed)) + " km/h");
            row.push_back(cellText);
            
            // Allowed speed
            cellText.setString(std::to_string(static_cast<int>(avn.allowedSpeed)) + " km/h");
            row.push_back(cellText);
            
            avnTableRows.push_back(row);
        }
        
        pthread_mutex_unlock(&atcMutex);
        
        dataInitialized = true;
    }
    
    void draw2()  {
        // Call the parent class's draw method first
        SimulationView::draw();
        
        if (!active() || !dataInitialized) {
            return;
        }
        
        // Draw timer
        SimulationView::window.draw(timerText);
        
        // Draw flight count
        SimulationView::window.draw(flightCountText);
        
        // Draw AVN count
        SimulationView::window.draw(avnCountText);
        
        // Draw runway status
        for (int i = 0; i < runwayShapes.size(); i++) {
            SimulationView::window.draw(runwayShapes[i]);
            SimulationView::window.draw(runwayTexts[i]);
        }
        
        // Draw flight table
        SimulationView::window.draw(flightTableBg);
        SimulationView::window.draw(flightTableTitle);
        
        for (auto& header : flightTableHeaders) {
            SimulationView::window.draw(header);
        }
        
        // Draw visible flight rows (with scrolling)
        for (size_t i = 0; i < flightTableRows.size(); i++) {
            float rowY = 420 + i * 30 - scrollOffsetFlights;
            
            // Skip rows outside the visible area
            if (rowY < 370 || rowY > 370 + maxVisibleFlights * 30) {
                continue;
            }
            
            float cellX = 410;
            for (auto& cell : flightTableRows[i]) {
                cell.setPosition(cellX, rowY);
                SimulationView::window.draw(cell);
                cellX += 110; // Adjusted column width
            }
        }
        
        // Draw AVN table
        SimulationView::window.draw(avnTableBg);
        SimulationView::window.draw(avnTableTitle);
        
        for (auto& header : avnTableHeaders) {
            SimulationView::window.draw(header);
        }
        
        // Draw visible AVN rows (with scrolling)
        for (size_t i = 0; i < avnTableRows.size(); i++) {
            float rowY = 420 + flightTableBg.getSize().y + 20 + i * 30 - scrollOffsetAVNs;
            
            // Skip rows outside the visible area
            if (rowY < 370 + flightTableBg.getSize().y + 20 || 
                rowY > 370 + flightTableBg.getSize().y + 20 + maxVisibleAVNs * 30) {
                continue;
            }
            
            float cellX = 410;
            for (auto& cell : avnTableRows[i]) {
                cell.setPosition(cellX, rowY);
                SimulationView::window.draw(cell);
                cellX += 110; // Adjusted column width
            }
        }
    }
};

int main() {
    srand(time(0)); // Seed the random number generator

    ///////////////////

    if (pipe(atcs_to_avn) < 0 || pipe(avn_to_airline) < 0 ||
        pipe(avn_to_stripe) < 0 || pipe(stripe_to_avn) < 0 ||
        pipe(stripe_to_airline) < 0 || pipe(avn_to_atcs) < 0){

        std::cerr << "Pipe creation failed\n";
        return 1;
    }
    if (avnGen == nullptr) {
        avnGen_id = fork();
        if (avnGen_id == 0) {
            avnGen = new AVNGenerator(atcs_to_avn, avn_to_atcs, avn_to_airline, avn_to_stripe, stripe_to_avn);
            avnGen->run();
            exit(0);
        }
    }
    if (airlinePortal == nullptr) {
        airlinePortal_id = fork();
        if (airlinePortal_id == 0) {
            airlinePortal = new AirlinePortal(avn_to_airline, stripe_to_airline);
            airlinePortal->run();
            exit(0);
        }
    }
    if (stripePayment == nullptr) {
        stripePayment_id = fork();
        if (stripePayment_id == 0) {
            stripePayment = new StripePayment(avn_to_stripe, stripe_to_avn, stripe_to_airline);
            stripePayment->run();
            exit(0);
        }
    }

    // Create the main window 
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "AirControlX SFML Menu", sf::Style::Fullscreen);
    window.setFramerateLimit(60);
    
    // Create the menu
    Menu menu(window.getSize());


    WindowX = window.getSize().x;
    WindowY = window.getSize().y;
    
    // Create the enhanced simulation view
    EnhancedSimulationView simulationView(window);
    
    // Background
    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("/home/sameer/Desktop/desk/AirControlX/Media/MenuBackground3.png")) {
        std::cout << "Failed to load background image, using default gradient background." << std::endl;
    }
    
    sf::Sprite backgroundSprite;
    // Scale background to fit window
    if (backgroundTexture.getSize().x > 0) {
        backgroundSprite.setTexture(backgroundTexture);
        float scaleX = (float)window.getSize().x / backgroundTexture.getSize().x;
        float scaleY = (float)window.getSize().y / backgroundTexture.getSize().y;
        float scale = std::max(scaleX, scaleY);
        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(
            (window.getSize().x - backgroundTexture.getSize().x * scale) / 2.0f,
            (window.getSize().y - backgroundTexture.getSize().y * scale) / 2.0f
        );
    }
    
    // Main loop
    while (window.isOpen()) {
        // Handle events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                // Clean up before closing
                if (simulationRunning) {
                    // Wait for simulation thread to finish
                    pthread_join(simulationThreadId, NULL);
                    simulationRunning = false;
                }
                
                // Delete the global ATC system if it exists
                if (ATCS != nullptr) {
                    delete ATCS;
                    ATCS = nullptr;
                }
                if (avnGen != nullptr) {
                    kill(avnGen_id, SIGTERM);
                    avnGen = nullptr;
                }
                if (airlinePortal != nullptr) {
                    kill(airlinePortal_id, SIGTERM);
                    airlinePortal = nullptr;
                }
                if (stripePayment != nullptr) {
                    kill(stripePayment_id, SIGTERM);
                    stripePayment = nullptr;
                }
                
                window.close();
            }
            
            // Handle escape key to exit
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                // Clean up before closing
                if (simulationRunning) {
                    // Wait for simulation thread to finish
                    pthread_join(simulationThreadId, NULL);
                    simulationRunning = false;
                }
                
                // Delete the global ATC system if it exists
                if (ATCS != nullptr) {
                    delete ATCS;
                    ATCS = nullptr;
                }
                if (avnGen != nullptr) {
                    kill(avnGen_id, SIGTERM);
                    avnGen = nullptr;
                }
                if (airlinePortal != nullptr) {
                    kill(airlinePortal_id, SIGTERM);
                    airlinePortal = nullptr;
                }
                if (stripePayment != nullptr) {
                    kill(stripePayment_id, SIGTERM);
                    stripePayment = nullptr;
                }

                
                window.close();
            }

            // Get mouse position for hover effects
            if (event.type == sf::Event::MouseMoved) {
                if (simulationView.active()) {
                    if (simulationView.isBackButtonClicked(event.mouseMove.x, event.mouseMove.y)) {
                        // Highlight back button
                    }
                } else {
                    menu.update(event.mouseMove.x, event.mouseMove.y);
                }
            }
            
            // Handle menu item clicks
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                if (simulationView.active()) {
                    if (simulationView.isBackButtonClicked(event.mouseButton.x, event.mouseButton.y)) {
                        simulationView.deactivate();
                    }
                } else {
                    int selectedIndex = menu.getSelectedIndex();
                    if (selectedIndex != -1) {
                        switch (selectedIndex) {
                            case 0: // Start Simulation
                                std::cout << "Starting simulation..." << std::endl;
                                
                                // Only start if not already running
                                if (!simulationRunning) {
                                    // Create the ATC system if it doesn't exist

                                    if (ATCS == nullptr) {

                                        ATCS = new ATCSystem(atcs_to_avn, avn_to_atcs);
                                    }
                                    
                                    // Set the simulation running flag
                                    simulationRunning = true;
                                    
                                    // Start the simulation in a separate thread using pthread
                                    if (pthread_create(&simulationThreadId, NULL, runSimulationThread, NULL) != 0) {
                                        std::cerr << "Error creating simulation thread." << std::endl;
                                        simulationRunning = false;
                                    } else {
                                        // Detach the thread so it can run independently
                                        pthread_detach(simulationThreadId);
                                    }
                                }
                                
                                simulationView.activate();
                                break;
                            case 1: // Airline Portal
                                std::cout << "Opening Airline Portal..." << std::endl;
                                // Call the static method to launch the airline portal in a child process
                                //AirlinePortal::launchPortal(avn_to_airline, stripe_to_airline);
                                break;
                            case 2: // Settings
                                std::cout << "Opening settings..." << std::endl;
                                break;
                            case 3: // View Statistics
                                std::cout << "Viewing statistics..." << std::endl;
                                break;
                            case 4: // Credits
                                std::cout << "Showing credits..." << std::endl;
                                break;
                            case 5: // Exit
                                std::cout << "Exiting..." << std::endl;
                                
                                // Clean up before closing
                                if (simulationRunning) {
                                    // Wait for simulation thread to finish
                                    pthread_join(simulationThreadId, NULL);
                                    simulationRunning = false;
                                }
                                
                                // Delete the global ATC system if it exists
                                if (ATCS != nullptr) {
                                    delete ATCS;
                                    ATCS = nullptr;
                                }
                                
                                // Close all the pipes
                                close(atcs_to_avn[0]);
                                close(atcs_to_avn[1]);
                                close(avn_to_atcs[0]);
                                close(avn_to_atcs[1]);
                                close(avn_to_airline[0]);
                                close(avn_to_airline[1]);
                                close(avn_to_stripe[0]);
                                close(avn_to_stripe[1]);
                                close(stripe_to_avn[0]);
                                close(stripe_to_avn[1]);
                                close(stripe_to_airline[0]);
                                close(stripe_to_airline[1]);
                                
                                // Terminate child processes
                                kill(avnGen_id, SIGTERM);
                                kill(airlinePortal_id, SIGTERM);
                                kill(stripePayment_id, SIGTERM);
                                
                                window.close();
                                break;
                        }
                        menu.resetSelection();
                    }
                }
            }
            
            // Handle mouse wheel scrolling
            if (event.type == sf::Event::MouseWheelScrolled) {
                simulationView.handleScroll(event.mouseWheelScroll.delta, sf::Vector2i(event.mouseWheelScroll.x, event.mouseWheelScroll.y));
            }
        }
        
        // Clear the screen
        window.clear(sf::Color(0, 0, 50));
        
        if (simulationView.active()) {
            simulationView.update2(); // Update simulation view with current data
            simulationView.draw2();
            
            // Safely draw plane sprites with proper mutex locking
            if (ATCS != nullptr) {
                pthread_mutex_lock(&atcMutex);
                for(auto& plane : ATCS->flights){
                    if (plane != nullptr && plane->airline != nullptr && (plane->state == AirCraftState::landing || plane->state == AirCraftState::takeoff_roll)) {
                        // Draw the sprite
                        if (plane->runway == Runway::RWY_C){
                            plane->planeSprite.setPosition(plane->planeSprite.getPosition().x, plane->planeSprite.getPosition().y-speed);
                        }
                        else if (plane->runway == Runway::RWY_A && plane->direction == Direction::north){
                            plane->planeSprite.setPosition(plane->planeSprite.getPosition().x, plane->planeSprite.getPosition().y-speed);
                        }
                        else if (plane->runway == Runway::RWY_B && plane->direction == Direction::east){
                            plane->planeSprite.setPosition(plane->planeSprite.getPosition().x+speed, plane->planeSprite.getPosition().y);
                        }
                        else if (plane->runway == Runway::RWY_A && plane->direction == Direction::south){
                            plane->planeSprite.setPosition(plane->planeSprite.getPosition().x, plane->planeSprite.getPosition().y+speed);
                        }
                        else if (plane->runway == Runway::RWY_B && plane->direction == Direction::west){
                            plane->planeSprite.setPosition(plane->planeSprite.getPosition().x-speed, plane->planeSprite.getPosition().y);
                        }
                        window.draw(plane->planeSprite);
                    }
                }
                pthread_mutex_unlock(&atcMutex);
            }
        } else {
            // Draw background if texture loaded successfully
            if (backgroundTexture.getSize().x > 0) {
                window.draw(backgroundSprite);
            }
            
            // Draw the menu
            menu.draw(window);
        }
        
        // Display the window
        window.display();
    }
    
    // Clean up mutex
    pthread_mutex_destroy(&atcMutex);
    waitpid(avnGen_id, NULL, 0);
    waitpid(airlinePortal_id, NULL, 0);
    waitpid(stripePayment_id, NULL, 0);
    
    return 0;
}