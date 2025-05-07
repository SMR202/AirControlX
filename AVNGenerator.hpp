#pragma once
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include <fstream>
#include "MsgStructs.hpp"

using namespace std;

class AVNGenerator {
    private:
    int atcs_to_avn[2];
    int avn_to_atcs[2];
    int avn_to_airline[2];
    int avn_to_stripe[2];
    int stripe_to_avn[2];
    vector<AVNNotice> avnNotices;
    
    
    public:
    AVNGenerator(int* atcs_to_avn, int* avn_to_atcs, int* avn_to_airline, int* avn_to_stripe, int* stripe_to_avn) {
        // Constructor code here
        this->atcs_to_avn[0] = atcs_to_avn[0];
        this->atcs_to_avn[1] = atcs_to_avn[1];
        this->avn_to_atcs[0] = avn_to_atcs[0];
        this->avn_to_atcs[1] = avn_to_atcs[1];
        this->avn_to_airline[0] = avn_to_airline[0];
        this->avn_to_airline[1] = avn_to_airline[1];
        this->avn_to_stripe[0] = avn_to_stripe[0];
        this->avn_to_stripe[1] = avn_to_stripe[1];
        this->stripe_to_avn[0] = stripe_to_avn[0];
        this->stripe_to_avn[1] = stripe_to_avn[1];

    }

    void run() {
        // Simulate AVN generation        
        // Close the write end of the pipe only if we created it
        // DO NOT close the write end if it was passed to us
        // close(atcs_to_avn[1]); // Remove this line - it's closed in the wrong place
        
        // Open the file for appending, not just writing
        FILE* file = fopen("violations.txt", "a");
        if (!file) {
            std::cerr << "Failed to open violations.txt for writing" << std::endl;
        }
        
        while(1){
            // Read from ATCS to AVN pipe
            AVNNotice details;
            int bytesread = read(atcs_to_avn[0], &details, sizeof(details));
            
            if (bytesread > 0){
                if(fork() == 0)
                    execl("/usr/bin/bash", "bash", "-c", "espeak 'Violation detected!'", NULL);
                
                // Process the violation details
                std::cout << "AVN Generator: Processing violation for flight " << details.aircraftId << std::endl;
                avnNotices.push_back(details);
                // Use proper C-style strings instead of trying to use std::string methods
                if (file) {
                    fprintf(file, "AVN ID: %d, Aircraft ID: %s, Airline: %s, Flight: %s, Type: %s, Recorded Speed: %.2f, Allowed Speed: %.2f, Fine: $%.2f, Timestamp: %s",
                        details.avnId, details.aircraftId, details.AirlineName, details.flightNumber, details.aircraftType,
                        details.recordedSpeed, details.allowedSpeed, details.totalFine,
                        ctime(&details.timestamp));
                    // Flush the file but don't close it each time
                    fflush(file);
                } else {
                    std::cerr << "Error: file handle is null" << std::endl;
                }
                close(avn_to_airline[0]);
                // Send the violation details to the airline portal
                write(avn_to_airline[1], &details, sizeof(details));
                
                // Also communicate with the Stripe payment system
                write(avn_to_stripe[1], &details, sizeof(details));
                
                // Send acknowledgment back to ATCS
                bool ack = true;
                write(avn_to_atcs[1], &ack, sizeof(ack));
            }
            else{
                if (fork() == 0)
                    execl("/usr/bin/bash", "bash", "-c", "espeak 'You failed!'", NULL);
            }
            // Simulate some processing time
            sleep(1);
        }
        
        // Close the file at the end (though this code is unreachable in the current implementation)
        if (file) {
            fclose(file);
        }
    }
};