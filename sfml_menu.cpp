#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>

// Structure to represent a menu button
struct MenuItem {
    sf::Text text;
    sf::ConvexShape background;
    bool isSelected;
    
    MenuItem(const sf::Font& font, const std::string& label, float xPos, float yPos, float width, float height) 
        : isSelected(false) {
        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(30); // Increased text size from 24 to 30
        text.setFillColor(sf::Color::White);
        //text.setStyle(sf::Text::Bold); // Added bold style for better visibility
        
        // Improved text centering with better calculation
        float textWidth = text.getLocalBounds().width;
        float textHeight = text.getLocalBounds().height;
        text.setOrigin(textWidth / 2.0f, textHeight / 2.0f);
        text.setPosition(
            xPos + width / 2.0f,
            yPos + height / 2.5f
        );
        
        background.setPointCount(8); // For rounded rectangle (4 corners, 2 points per corner)
        
        // Set rounded corners for the buttons
        float cornerRadius = 20.0f; // Increased radius for properly rounded corners
        
        // Define the rounded rectangle points
        float left = xPos;
        float top = yPos;
        float right = xPos + width;
        float bottom = yPos + height;
        
        // Top-left corner
        background.setPoint(0, sf::Vector2f(left + cornerRadius, top));
        background.setPoint(1, sf::Vector2f(left, top + cornerRadius));
        
        // Bottom-left corner
        background.setPoint(2, sf::Vector2f(left, bottom - cornerRadius));
        background.setPoint(3, sf::Vector2f(left + cornerRadius, bottom));
        
        // Bottom-right corner
        background.setPoint(4, sf::Vector2f(right - cornerRadius, bottom));
        background.setPoint(5, sf::Vector2f(right, bottom - cornerRadius));
        
        // Top-right corner
        background.setPoint(6, sf::Vector2f(right, top + cornerRadius));
        background.setPoint(7, sf::Vector2f(right - cornerRadius, top));
        
        background.setFillColor(sf::Color(100, 100, 100, 200)); // Default gray color
        background.setOutlineThickness(2.0f);
        background.setOutlineColor(sf::Color(150, 150, 150));
    }
    
    void setSelected(bool selected) {
        isSelected = selected;
        if (isSelected) {
            background.setFillColor(sf::Color(70, 130, 180, 220)); // Highlight color
            background.setOutlineColor(sf::Color(173, 216, 230));
        } else {
            background.setFillColor(sf::Color(100, 100, 100, 200)); // Default color
            background.setOutlineColor(sf::Color(150, 150, 150));
        }
    }
    
    bool contains(float x, float y) const {
        return background.getGlobalBounds().contains(x, y);
    }
    
    void setPosition(float xPos, float yPos) {
        float width = background.getPoint(4).x - background.getPoint(0).x;
        float height = background.getPoint(3).y - background.getPoint(1).y;

        text.setPosition(
            xPos + width / 2.0f,
            yPos + height / 2.5f
        );

        float cornerRadius = 20.0f;

        float left = xPos;
        float top = yPos;
        float right = xPos + width;
        float bottom = yPos + height;

        background.setPoint(0, sf::Vector2f(left + cornerRadius, top));
        background.setPoint(1, sf::Vector2f(left, top + cornerRadius));
        background.setPoint(2, sf::Vector2f(left, bottom - cornerRadius));
        background.setPoint(3, sf::Vector2f(left + cornerRadius, bottom));
        background.setPoint(4, sf::Vector2f(right - cornerRadius, bottom));
        background.setPoint(5, sf::Vector2f(right, bottom - cornerRadius));
        background.setPoint(6, sf::Vector2f(right, top + cornerRadius));
        background.setPoint(7, sf::Vector2f(right - cornerRadius, top));
    }
    
    void draw(sf::RenderWindow& window) const {
        window.draw(background);
        window.draw(text);
    }
};

class Menu {
private:
    sf::Font font;
    std::vector<MenuItem> menuItems;
    int selectedIndex;
    sf::Vector2u windowSize;
    float menuPosX; // X position control variable
    float menuPosY; // Y position control variable
    
public:
    Menu(const sf::Vector2u& winSize) : selectedIndex(-1), windowSize(winSize), 
                                        menuPosX(0.65f), menuPosY(0.3f) {
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            std::cerr << "Failed to load font!" << std::endl;
        }
        
        // Create menu items - position relative to window size
        float menuItemWidth = windowSize.x * 0.3f;
        float menuItemHeight = windowSize.y * 0.08f;
        float startY = windowSize.y * menuPosY; // Use menuPosY to control vertical position
        float spacing = windowSize.y * 0.03f;
        
        // Position buttons using menuPosX to control horizontal position
        float positionX = windowSize.x * menuPosX;
        
        menuItems.push_back(MenuItem(font, "Start Simulation", positionX, startY, menuItemWidth, menuItemHeight));
        menuItems.push_back(MenuItem(font, "Settings", positionX, startY + (menuItemHeight + spacing), menuItemWidth, menuItemHeight));
        menuItems.push_back(MenuItem(font, "View Statistics", positionX, startY + 2 * (menuItemHeight + spacing), menuItemWidth, menuItemHeight));
        menuItems.push_back(MenuItem(font, "Credits", positionX, startY + 3 * (menuItemHeight + spacing), menuItemWidth, menuItemHeight));
        menuItems.push_back(MenuItem(font, "Exit", positionX, startY + 4 * (menuItemHeight + spacing), menuItemWidth, menuItemHeight));
    }
    
    // Add methods to update menu position
    void setPosition(float x, float y) {
        menuPosX = x;
        menuPosY = y;
        updateMenuItemPositions();
    }
    
    void updateMenuItemPositions() {
        float menuItemWidth = windowSize.x * 0.3f;
        float menuItemHeight = windowSize.y * 0.08f;
        float startY = windowSize.y * menuPosY;
        float spacing = windowSize.y * 0.03f;
        float positionX = windowSize.x * menuPosX;
        
        // Update each menu item's position
        for (int i = 0; i < menuItems.size(); i++) {
            menuItems[i].setPosition(positionX, startY + i * (menuItemHeight + spacing));
        }
    }
    
    void update(float mouseX, float mouseY) {
        // Reset all items
        for (int i = 0; i < menuItems.size(); i++) {
            menuItems[i].setSelected(false);
        }
        
        // Check for hovering
        for (int i = 0; i < menuItems.size(); i++) {
            if (menuItems[i].contains(mouseX, mouseY)) {
                menuItems[i].setSelected(true);
                selectedIndex = i;
                break;
            }
        }
    }
    
    int getSelectedIndex() const {
        return selectedIndex;
    }
    
    void resetSelection() {
        selectedIndex = -1;
        for (auto& item : menuItems) {
            item.setSelected(false);
        }
    }
    
    void draw(sf::RenderWindow& window) const {
        // Draw menu items
        for (const auto& item : menuItems) {
            item.draw(window);
        }
    }
};

// SimulationView class to display the simulation
class SimulationView {
private:
    sf::RenderWindow& window;
    sf::Font font;
    sf::RectangleShape backgroundPanel;
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    sf::Text title;
    sf::Text backButton;
    bool isActive;

public:
    SimulationView(sf::RenderWindow& win) : window(win), isActive(false) {
        // Load font
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            std::cerr << "Failed to load font in SimulationView!" << std::endl;
        }
        
        // Get window dimensions for proper scaling
        sf::Vector2u windowSize = window.getSize();
        
        // Background panel
        backgroundPanel.setSize(sf::Vector2f(windowSize.x, windowSize.y));
        backgroundPanel.setFillColor(sf::Color(20, 30, 50, 240));
        
        // Load background texture
        if (!backgroundTexture.loadFromFile("/home/sameer/Desktop/desk/AirControlX/Media/SimBG1.png")) {
            std::cerr << "Failed to load simulation background!" << std::endl;
        } else {
            backgroundSprite.setTexture(backgroundTexture);
            float scaleX = (float)windowSize.x / backgroundTexture.getSize().x;
            float scaleY = (float)windowSize.y / backgroundTexture.getSize().y;
            float scale = std::max(scaleX, scaleY);
            backgroundSprite.setScale(scale, scale);
            backgroundSprite.setPosition(
                (windowSize.x - backgroundTexture.getSize().x * scale) / 2.0f,
                (windowSize.y - backgroundTexture.getSize().y * scale) / 2.0f
            );
        }
        
        // Title
        title.setFont(font);
        title.setString("Air Control Simulation");
        title.setCharacterSize(42);
        title.setFillColor(sf::Color::White);
        title.setStyle(sf::Text::Bold);
        title.setPosition(windowSize.x * 0.5f - title.getLocalBounds().width / 2.0f, windowSize.y * 0.05f);
        
        // Back button
        backButton.setFont(font);
        backButton.setString("< Back to Menu");
        backButton.setCharacterSize(24);
        backButton.setFillColor(sf::Color::White);
        backButton.setPosition(windowSize.x * 0.05f, windowSize.y * 0.05f);
    }
    
    void activate() {
        isActive = true;
    }
    
    void deactivate() {
        isActive = false;
    }
    
    bool isBackButtonClicked(float mouseX, float mouseY) {
        return backButton.getGlobalBounds().contains(mouseX, mouseY);
    }
    
    void update() {
        // For future use: Add simulation elements here
    }
    
    void draw() {
        if (!isActive) return;
        
        window.draw(backgroundSprite);
        //window.draw(backgroundPanel);
        
        // Draw title and back button
        window.draw(title);
        window.draw(backButton);
        
        // Empty simulation area for custom runway display
    }
    
    bool active() const {
        return isActive;
    }
};

int main() {
    // Create the main window in fullscreen mode
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "AirControlX SFML Menu", sf::Style::Fullscreen);
    window.setFramerateLimit(60);
    
    // Create the menu
    Menu menu(window.getSize());
    
    // Create the simulation view
    SimulationView simulationView(window);
    
    // Background
    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("/home/sameer/Desktop/desk/AirControlX/Media/MenuBackground3.png")) {
        // If loading fails, create a gradient background
        std::cout << "Failed to load background image, using default gradient background." << std::endl;
    }
    
    sf::Sprite backgroundSprite;
    // Scale background to fit fullscreen
    if (backgroundTexture.getSize().x > 0) {
        backgroundSprite.setTexture(backgroundTexture);
        // Scale the background to cover the entire screen
        float scaleX = (float)window.getSize().x / backgroundTexture.getSize().x;
        float scaleY = (float)window.getSize().y / backgroundTexture.getSize().y;
        float scale = std::max(scaleX, scaleY);
        backgroundSprite.setScale(scale, scale);
        // Center the background
        backgroundSprite.setPosition(
            (window.getSize().x - backgroundTexture.getSize().x * scale) / 2.0f,
            (window.getSize().y - backgroundTexture.getSize().y * scale) / 2.0f
        );
    }
    
    // Darker overlay to make menu more visible
    sf::RectangleShape darkOverlay;
    darkOverlay.setSize(sf::Vector2f(window.getSize().x, window.getSize().y));
    darkOverlay.setFillColor(sf::Color(0, 0, 20, 200));
    
    // Main loop
    while (window.isOpen()) {
        // Handle events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            // Get mouse position for hover effects
            if (event.type == sf::Event::MouseMoved) {
                if (simulationView.active()) {
                    if (simulationView.isBackButtonClicked(event.mouseMove.x, event.mouseMove.y)) {
                        simulationView.deactivate();
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
                                simulationView.activate();
                                break;
                            case 1: // Settings
                                std::cout << "Opening settings..." << std::endl;
                                break;
                            case 2: // View Statistics
                                std::cout << "Viewing statistics..." << std::endl;
                                break;
                            case 3: // Credits
                                std::cout << "Showing credits..." << std::endl;
                                break;
                            case 4: // Exit
                                std::cout << "Exiting..." << std::endl;
                                window.close();
                                break;
                        }
                        menu.resetSelection();
                    }
                }
            }
        }
        
        // Clear the screen
        window.clear(sf::Color(0, 0, 50));
        
        if (simulationView.active()) {
            simulationView.update();
            simulationView.draw();
        } else {
            // Draw background if texture loaded successfully
            if (backgroundTexture.getSize().x > 0) {
                window.draw(backgroundSprite);
                //window.draw(darkOverlay);
            }
            
            // Draw the menu
            menu.draw(window);
        }
        
        // Display the window
        window.display();
    }
    
    return 0;
}