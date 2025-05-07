#!/bin/bash

echo "Compiling AirControlX application with integrated ATCSystem..."

# Compile the SFML menu with integrated ATCSystem using pthreads
g++ -o sfml_menu sfml_menu.cpp -lsfml-graphics -lsfml-window -lsfml-system -pthread -Wall

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo "To run the application, execute: ./sfml_menu"
else
    echo "Compilation failed. Please check for errors."
fi