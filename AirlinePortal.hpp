#pragma once

#include <iostream>
#include <unistd.h>

class AirlinePortal {
private:
    int avn_to_airline[2];
    int stripe_to_airline[2];

public:
    AirlinePortal(int* avn_to_airline, int* stripe_to_airline) {
        // Initialize pipe file descriptors
        this->avn_to_airline[0] = avn_to_airline[0];
        this->avn_to_airline[1] = avn_to_airline[1];
        this->stripe_to_airline[0] = stripe_to_airline[0];
        this->stripe_to_airline[1] = stripe_to_airline[1];
    }

    void processPayment() {
        // Simulate payment processing
        std::cout << "Processing payment..." << std::endl;
        sleep(1); // Simulate time taken to process payment
        std::cout << "Payment processed." << std::endl;
    }
};