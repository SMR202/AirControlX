#pragma once

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include "MsgStructs.hpp"

class StripePayment {
private:
    int avn_to_stripe[2];
    int stripe_to_avn[2];
    int stripe_to_airline[2];

public:
    StripePayment(int* avn_to_stripe, int* stripe_to_avn, int* stripe_to_airline) {
        // Initialize pipe file descriptors
        this->avn_to_stripe[0] = avn_to_stripe[0];
        this->avn_to_stripe[1] = avn_to_stripe[1];
        this->stripe_to_avn[0] = stripe_to_avn[0];
        this->stripe_to_avn[1] = stripe_to_avn[1];
        this->stripe_to_airline[0] = stripe_to_airline[0];
        this->stripe_to_airline[1] = stripe_to_airline[1];
    }

    void run() {
        // Process payments received from airline portal
        std::cout << "StripePayment: Payment processing service started" << std::endl;
        
        while(1) {
            // Read payment request from Airline Portal
            PaymentRequest paymentRequest;
            int bytesRead = read(avn_to_stripe[0], &paymentRequest, sizeof(paymentRequest));
            
            if (bytesRead > 0) {
                // Process the payment
                std::cout << "StripePayment: Processing payment for AVN ID: " << paymentRequest.avnId 
                          << ", Aircraft: " << paymentRequest.aircraftId
                          << ", Amount: $" << paymentRequest.totalFine << std::endl;
                
                // Simulate payment processing delay
                sleep(2);
                
                // Create payment confirmation
                PaymentConfirmation confirmation;
                confirmation.avnId = paymentRequest.avnId;
                strncpy(confirmation.status, "paid", sizeof(confirmation.status) - 1);
                confirmation.status[sizeof(confirmation.status) - 1] = '\0'; // Ensure null termination
                
                // Send confirmation to AVN Generator
                write(stripe_to_avn[1], &confirmation, sizeof(confirmation));
                std::cout << "StripePayment: Payment confirmation sent to AVN Generator for AVN ID: " 
                          << confirmation.avnId << std::endl;
                
                // Send confirmation to Airline Portal
                write(stripe_to_airline[1], &confirmation, sizeof(confirmation));
                std::cout << "StripePayment: Payment confirmation sent to Airline Portal for AVN ID: " 
                          << confirmation.avnId << std::endl;
                
                // Record payment in a log file
                FILE* file = fopen("payment_log.txt", "a");
                if (file) {
                    time_t now = time(0);
                    fprintf(file, "Payment processed - AVN ID: %d, Aircraft ID: %d, Type: %s, Amount: $%.2f, Time: %s",
                            confirmation.avnId, paymentRequest.aircraftId, paymentRequest.aircraftType, 
                            paymentRequest.totalFine, ctime(&now));
                    fclose(file);
                } else {
                    std::cerr << "Failed to open payment_log.txt for writing" << std::endl;
                }
            }
            
            // Small delay to avoid busy waiting
            usleep(100000);
        }
    }
};