#pragma once

#include <iostream>
#include <unistd.h>
#include <SFML/Graphics.hpp>
#include "MsgStructs.hpp"
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <cstring>

// Structure to represent a pay button
struct PayButton {
    int avnId;
    sf::RectangleShape shape;
    sf::Text text;
};

class AirlinePortal {
private:
    int avn_to_airline[2];
    int stripe_to_airline[2];
    std::vector<AVNNotice> avnNotices;
    
    // Map to store notices by airline
    std::map<std::string, std::vector<AVNNotice>> noticesByAirline;
    
    // SFML window
    sf::RenderWindow window;
    sf::Font font;
    
    // Current state
    enum State { LOGIN, DASHBOARD };
    State currentState;
    std::string currentAirline;
    
    // Text input for login
    sf::Text loginPrompt;
    sf::Text inputText;
    sf::RectangleShape inputBox;
    std::string inputString;
    
    // Dashboard elements
    sf::Text dashboardTitle;
    sf::Text backButton;
    sf::RectangleShape noticeTable;
    std::vector<sf::Text> tableHeaders;
    std::vector<std::vector<sf::Text>> noticeRows;
    std::vector<PayButton> payButtons;
    float scrollOffset;
    int maxVisibleRows;

    // Payment processing
    bool paymentProcessed;
    int lastProcessedAvnId;
    int paymentMessageTimer;

public:
    AirlinePortal(int* avn_to_airline, int* stripe_to_airline) {
        // Initialize pipe file descriptors
        this->avn_to_airline[0] = avn_to_airline[0];
        this->avn_to_airline[1] = avn_to_airline[1];
        this->stripe_to_airline[0] = stripe_to_airline[0];
        this->stripe_to_airline[1] = stripe_to_airline[1];
        
        // Initialize SFML elements with a wider window
        window.create(sf::VideoMode(1800, 900), "Airline Portal");
        window.setFramerateLimit(60);
        
        // Load font
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf")) {
            if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
                std::cerr << "Failed to load font!" << std::endl;
            }
        }
        
        // Initialize login screen elements - center in the wider window
        currentState = LOGIN;
        loginPrompt.setFont(font);
        loginPrompt.setCharacterSize(36);
        loginPrompt.setFillColor(sf::Color::White);
        loginPrompt.setPosition(740, 300);
        loginPrompt.setString("Enter Airline Name:");
        
        inputBox.setSize(sf::Vector2f(500, 50));
        inputBox.setPosition(690, 400);
        inputBox.setFillColor(sf::Color(50, 50, 50));
        inputBox.setOutlineColor(sf::Color::White);
        inputBox.setOutlineThickness(2);
        
        inputText.setFont(font);
        inputText.setCharacterSize(24);
        inputText.setFillColor(sf::Color::White);
        inputText.setPosition(700, 410);
        
        // Initialize dashboard elements
        dashboardTitle.setFont(font);
        dashboardTitle.setCharacterSize(36);
        dashboardTitle.setFillColor(sf::Color::White);
        dashboardTitle.setPosition(50, 30);
        
        backButton.setFont(font);
        backButton.setCharacterSize(24);
        backButton.setFillColor(sf::Color::White);
        backButton.setString("< Back to Login");
        backButton.setPosition(50, 850);
        
        // Make notice table wider
        noticeTable.setSize(sf::Vector2f(1700, 750));
        noticeTable.setPosition(50, 80);
        noticeTable.setFillColor(sf::Color(30, 30, 50, 200));
        noticeTable.setOutlineColor(sf::Color(100, 100, 150));
        noticeTable.setOutlineThickness(2);
        
        // Set up table headers with wider columns
        std::vector<std::string> headers = {"AVN ID", "Aircraft ID", "Flight", "Recorded Speed", "Allowed Speed", "Aircraft Type", "Fine Amount", "Timestamp", "Actions"};
        float headerX = 60;
        // Increase column width for better readability
        float columnWidth = 180;
        
        for (const auto& header : headers) {
            sf::Text headerText;
            headerText.setFont(font);
            headerText.setCharacterSize(18);
            headerText.setFillColor(sf::Color(200, 200, 255));
            headerText.setPosition(headerX, 90);
            headerText.setString(header);
            tableHeaders.push_back(headerText);
            headerX += columnWidth;
        }
        
        // Initialize scroll values
        scrollOffset = 0;
        maxVisibleRows = 25;

        // Initialize payment processing variables
        paymentProcessed = false;
        lastProcessedAvnId = -1;
        paymentMessageTimer = 0;
    }

    void run() {
        // Start a background thread to read from pipes
        pthread_t readThread;
        pthread_create(&readThread, NULL, &AirlinePortal::readThreadFunc, this);
        
        // Add some mock data for testing if no notices exist yet
        if (avnNotices.empty()) {
            //addMockDataForTesting();
        }
        
        // Main window loop
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
                
                // Handle text input in login state
                if (currentState == LOGIN) {
                    if (event.type == sf::Event::TextEntered) {
                        if (event.text.unicode == 8) { // Backspace
                            if (!inputString.empty())
                                inputString.pop_back();
                        }
                        else if (event.text.unicode == 13) { // Enter
                            if (!inputString.empty()) {
                                currentAirline = inputString;
                                currentState = DASHBOARD;
                                dashboardTitle.setString(currentAirline + " Aviation - AVN Notices");
                                inputString.clear();
                                updateNoticeTable();
                                std::cout << "Switched to dashboard view for airline: " << currentAirline << std::endl;
                            }
                        }
                        else if (event.text.unicode < 128) {
                            inputString += static_cast<char>(event.text.unicode);
                        }
                        inputText.setString(inputString);
                    }
                }
                
                // Handle mouse clicks
                if (event.type == sf::Event::MouseButtonPressed) {
                    if (currentState == DASHBOARD) {
                        // Check if back button was clicked
                        if (backButton.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y)) {
                            currentState = LOGIN;
                            inputString.clear();
                            inputText.setString(inputString);
                            std::cout << "Returned to login screen" << std::endl;
                        }
                        
                        // Check if any Pay button was clicked
                        for (auto& payButton : payButtons) {
                            if (payButton.shape.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y)) {
                                std::cout << "Pay button clicked for AVN ID: " << payButton.avnId << std::endl;
                                processPayment(payButton.avnId);
                            }
                        }
                    }
                }
                
                // Handle scrolling in dashboard
                if (event.type == sf::Event::MouseWheelScrolled && currentState == DASHBOARD) {
                    scrollOffset += event.mouseWheelScroll.delta * -20;
                    if (scrollOffset < 0) scrollOffset = 0;
                    
                    // Calculate max scroll based on number of notices
                    float maxScroll = std::max(0.0f, (float)((noticeRows.size() - maxVisibleRows) * 30));
                    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
                    
                    updateNoticeTable();
                }
            }
            
            // Clear the window
            window.clear(sf::Color(20, 20, 50));
            
            // Draw the appropriate view
            if (currentState == LOGIN) {
                window.draw(loginPrompt);
                window.draw(inputBox);
                window.draw(inputText);
            }
            else if (currentState == DASHBOARD) {
                window.draw(dashboardTitle);
                window.draw(noticeTable);
                
                // Draw headers
                for (auto& header : tableHeaders) {
                    window.draw(header);
                }
                
                // Draw notice rows
                for (auto& row : noticeRows) {
                    for (auto& cell : row) {
                        window.draw(cell);
                    }
                }
                
                // Draw Pay buttons
                for (auto& payButton : payButtons) {
                    window.draw(payButton.shape);
                    window.draw(payButton.text);
                }
                
                window.draw(backButton);
                
                // If no notices, show a message
                if (noticeRows.empty()) {
                    sf::Text noNoticesText;
                    noNoticesText.setFont(font);
                    noNoticesText.setCharacterSize(24);
                    noNoticesText.setFillColor(sf::Color::White);
                    noNoticesText.setPosition(500, 300);
                    noNoticesText.setString("No AVN notices for " + currentAirline);
                    window.draw(noNoticesText);
                }

                // Display payment confirmation message
                if (paymentProcessed && paymentMessageTimer > 0) {
                    sf::Text paymentMessage;
                    paymentMessage.setFont(font);
                    paymentMessage.setCharacterSize(24);
                    paymentMessage.setFillColor(sf::Color::Green);
                    paymentMessage.setPosition(500, 800);
                    paymentMessage.setString("Payment processed for AVN ID: " + std::to_string(lastProcessedAvnId));
                    window.draw(paymentMessage);
                    paymentMessageTimer--;
                }
            }
            
            // Display the window
            window.display();
        }
        
        // Cancel the background thread
        pthread_cancel(readThread);
    }

    // Static method to create and run the portal as a child process
    static void launchPortal(int* avn_to_airline, int* stripe_to_airline) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process
            // Close pipes we don't need in the child process
            close(avn_to_airline[1]); // We only read from this pipe
            close(stripe_to_airline[1]); // We only read from this pipe
            
            // Create and run the portal
            AirlinePortal portal(avn_to_airline, stripe_to_airline);
            portal.run();
            exit(0);  // Exit after window is closed
        }
        else if (pid < 0) {
            std::cerr << "Failed to fork Airline Portal process" << std::endl;
        }
        // Parent process continues
    }

private:
    // Thread function to read from pipes
    static void* readThreadFunc(void* arg) {
        AirlinePortal* portal = static_cast<AirlinePortal*>(arg);
        portal->readLoop();
        return nullptr;
    }
    
    void readLoop() {
        AVNNotice notice;
        while(1) {
            // Read from AVN to Airline pipe
            close(avn_to_airline[1]); // Close the write end in this process
            int bytesread = read(avn_to_airline[0], &notice, sizeof(notice));
            
            if (bytesread > 0) {
                // Store the notice
                avnNotices.push_back(notice);
                std::string airlineName(notice.AirlineName);
                noticesByAirline[airlineName].push_back(notice);
                
                // Update the notice table if we're viewing this airline
                // Use string comparison instead of direct comparison to avoid C-string comparison issues
                if (currentState == DASHBOARD && currentAirline == std::string(notice.AirlineName)) {
                    updateNoticeTable();
                }
            }
            
            // // Also check for payment confirmations from Stripe
            // PaymentConfirmation confirmation;
            // bytesread = read(stripe_to_airline[0], &confirmation, sizeof(confirmation));
            
            // if (bytesread > 0) {
            //     // Update the status for this AVN ID
            //     // This would update the display to show the fine has been paid
            //     std::cout << "Payment confirmation received for AVN ID: " << confirmation.avnId << std::endl;
            // }
            
            // usleep(100000); // Sleep to avoid busy waiting
        }
    }
    
    double calculateFine(double actualSpeed, double allowedSpeed) {
        // Simple fine calculation: $10 for each km/h over the limit
        return (actualSpeed - allowedSpeed) * 10.0;
    }
    
    void updateNoticeTable() {
        try {
            noticeRows.clear();
            payButtons.clear();
            
            // Filter notices for the current airline
            if (noticesByAirline.find(currentAirline) != noticesByAirline.end()) {
                std::vector<AVNNotice>& airlineNotices = noticesByAirline[currentAirline];
                
                // Create table rows
                for (size_t i = 0; i < airlineNotices.size(); i++) {
                    try {
                        const AVNNotice& notice = airlineNotices[i];
                        std::vector<sf::Text> row;
                        
                        // Calculate row position with scrolling
                        float rowY = 120 + i * 30 - scrollOffset;
                        
                        // Skip rows outside visible area
                        if (rowY < 100 || rowY > 700) {
                            continue;
                        }
                        
                        float cellX = 60;
                        float columnWidth = 180;
                        
                        // AVN ID
                        sf::Text cell;
                        cell.setFont(font);
                        cell.setCharacterSize(16);
                        cell.setFillColor(sf::Color::White);
                        cell.setPosition(cellX, rowY);
                        cell.setString("AVN-" + std::to_string(notice.avnId));
                        row.push_back(cell);
                        cellX += columnWidth;
                        
                        // Aircraft ID
                        cell.setPosition(cellX, rowY);
                        cell.setString(notice.aircraftId[0] != '\0' ? notice.aircraftId : "Unknown");
                        row.push_back(cell);
                        cellX += columnWidth;
                        
                        // Flight Number
                        cell.setPosition(cellX, rowY);
                        cell.setString(notice.flightNumber[0] != '\0' ? notice.flightNumber : "Unknown");
                        row.push_back(cell);
                        cellX += columnWidth;
                        
                        // Recorded Speed
                        cell.setPosition(cellX, rowY);
                        cell.setString(std::to_string(static_cast<int>(notice.recordedSpeed)) + " km/h");
                        row.push_back(cell);
                        cellX += columnWidth;
                        
                        // Allowed Speed
                        cell.setPosition(cellX, rowY);
                        cell.setString(std::to_string(static_cast<int>(notice.allowedSpeed)) + " km/h");
                        row.push_back(cell);
                        cellX += columnWidth;
                        
                        // Aircraft Type
                        cell.setPosition(cellX, rowY);
                        cell.setString(notice.aircraftType[0] != '\0' ? notice.aircraftType : "Unknown");
                        row.push_back(cell);
                        cellX += columnWidth;
                        
                        // Fine Amount
                        cell.setPosition(cellX, rowY);
                        cell.setString("$" + std::to_string(static_cast<int>(notice.totalFine)));
                        row.push_back(cell);
                        cellX += columnWidth;
                        
                        // Timestamp with day and date
                        cell.setPosition(cellX, rowY);
                        char timeStr[50];
                        time_t safeTime = notice.timestamp > 0 ? notice.timestamp : time(0);
                        strftime(timeStr, sizeof(timeStr), "%a, %b %d, %Y %H:%M:%S", localtime(&safeTime));
                        cell.setString(timeStr);
                        row.push_back(cell);
                        cellX += columnWidth;
                        
                        // Create a Pay button for this row
                        PayButton payButton;
                        payButton.avnId = notice.avnId;
                        payButton.shape.setSize(sf::Vector2f(80, 24));
                        payButton.shape.setPosition(cellX, rowY - 2);
                        payButton.shape.setFillColor(sf::Color(50, 150, 50));
                        payButton.shape.setOutlineColor(sf::Color::White);
                        payButton.shape.setOutlineThickness(1);
                        
                        payButton.text.setFont(font);
                        payButton.text.setCharacterSize(16);
                        payButton.text.setFillColor(sf::Color::White);
                        payButton.text.setPosition(cellX + 25, rowY);
                        payButton.text.setString("Pay");
                        
                        payButtons.push_back(payButton);
                        
                        noticeRows.push_back(row);
                    }
                    catch (std::exception& e) {
                        std::cerr << "Error processing notice " << i << ": " << e.what() << std::endl;
                        continue; // Skip this notice and continue with the next
                    }
                }
            }
        }
        catch (std::exception& e) {
            std::cerr << "Error updating notice table: " << e.what() << std::endl;
        }
    }

    void addMockDataForTesting() {
        // Create sample AVN notices for each airline
        std::vector<std::string> airlines = {"PIA", "AirBlue", "FedEx", "PAF", "BDart", "AK Amb"};
        
        for (int i = 0; i < airlines.size(); i++) {
            for (int j = 1; j <= 3; j++) {
                AVNNotice notice;
                notice.avnId = i * 10 + j;
                snprintf(notice.aircraftId, sizeof(notice.aircraftId), "%s-%d", airlines[i].c_str(), 100 + j);
                strncpy(notice.AirlineName, airlines[i].c_str(), sizeof(notice.AirlineName));
                strncpy(notice.flightNumber, notice.aircraftId, sizeof(notice.flightNumber));
                notice.recordedSpeed = 300 + (i * 50) + (j * 20);
                notice.allowedSpeed = 250 + (i * 30);
                
                if (i < 2) {
                    strcpy(notice.aircraftType, "Commercial");
                } else if (i < 4) {
                    strcpy(notice.aircraftType, "Cargo");
                } else {
                    strcpy(notice.aircraftType, "Emergency");
                }
                
                notice.totalFine = calculateFine(notice.recordedSpeed, notice.allowedSpeed);
                notice.timestamp = time(0) - (j * 3600); // Spread out the timestamps
                
                // Store in both collections
                avnNotices.push_back(notice);
                noticesByAirline[airlines[i]].push_back(notice);
            }
        }
        
        std::cout << "Added mock data for testing: " << avnNotices.size() << " notices" << std::endl;
    }

    // Handle payment for an AVN notice
    void processPayment(int avnId) {
        // Create payment request to send to Stripe
        PaymentRequest paymentRequest;
        
        // Find the AVN notice with this ID
        AVNNotice* targetNotice = nullptr;
        for (auto& notices : noticesByAirline) {
            for (auto& notice : notices.second) {
                if (notice.avnId == avnId) {
                    targetNotice = &notice;
                    break;
                }
            }
            if (targetNotice) break;
        }
        
        if (!targetNotice) {
            std::cerr << "Error: Could not find AVN notice with ID " << avnId << std::endl;
            return;
        }
        
        // Prepare payment request
        paymentRequest.avnId = avnId;
        strncpy(paymentRequest.aircraftType, targetNotice->aircraftType, sizeof(paymentRequest.aircraftType) - 1);
        paymentRequest.aircraftType[sizeof(paymentRequest.aircraftType) - 1] = '\0';
        paymentRequest.totalFine = targetNotice->totalFine;
        
        // Extract aircraft ID as an integer for the payment system
        int aircraftIdNum = 0;
        try {
            // Try to extract a number from the aircraft ID string
            std::string idStr = targetNotice->aircraftId;
            size_t dashPos = idStr.find('-');
            if (dashPos != std::string::npos && dashPos + 1 < idStr.length()) {
                aircraftIdNum = std::stoi(idStr.substr(dashPos + 1));
            } else {
                aircraftIdNum = std::stoi(idStr);
            }
        } catch (std::exception& e) {
            aircraftIdNum = avnId * 100; // Fallback if parsing fails
        }
        paymentRequest.aircraftId = aircraftIdNum;
        
        // Send the payment request to the Stripe system
        write(stripe_to_airline[1], &paymentRequest, sizeof(paymentRequest));
        
        std::cout << "Payment request sent for AVN ID: " << avnId << 
                     " Aircraft: " << targetNotice->aircraftId << 
                     " Amount: $" << targetNotice->totalFine << std::endl;
        
        // Display a confirmation message to the user
        paymentProcessed = true;
        lastProcessedAvnId = avnId;
        paymentMessageTimer = 180; // Display for 3 seconds (assuming 60 FPS)
    }
};