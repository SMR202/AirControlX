#pragma once
#include <string>
#include "enums.hpp"
#include <SFML/Graphics.hpp>

struct Airline{
    std::string name;
    AirCraftType type;
    int totalAircrafts;
    int flightsInOperation;
    sf::Sprite planeSprite;
    sf::Texture planeTexture;
};