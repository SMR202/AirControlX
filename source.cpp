#include <iostream>
#include <cstdlib>
#include <ctime>
#include "ATCSystem.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include "MsgStructs.hpp"
#include "AVNGenerator.hpp"
#include "AirlinePortal.hpp"
#include "StripePayment.hpp"
using namespace std;

// Pipe file descriptors
int atcs_to_avn[2];      // ATCS Controller to AVN Generator
int avn_to_airline[2];   // AVN Generator to Airline Portal
int avn_to_stripe[2];    // AVN Generator to StripePay Process
int stripe_to_avn[2];    // StripePay Process to AVN Generator
int stripe_to_airline[2]; // StripePay Process to Airline Portal
int avn_to_atcs[2];      // AVN Generator to ATCS Controller



int main() {
    // Initialize random number generator
    srand(time(0));
    
    std::cout << "Starting AirControlX Simulation...\n";
    
    // Create all IPC pipes
    if (pipe(atcs_to_avn) < 0 || pipe(avn_to_airline) < 0 || 
        pipe(avn_to_stripe) < 0 || pipe(stripe_to_avn) < 0 || 
        pipe(stripe_to_airline) < 0 || pipe(avn_to_atcs) < 0) {
        std::cerr << "Pipe creation failed\n";
        return 1;
    }

    // Set non-blocking mode for reading from pipes
    fcntl(atcs_to_avn[0], F_SETFL, O_NONBLOCK);
    fcntl(avn_to_airline[0], F_SETFL, O_NONBLOCK);
    fcntl(avn_to_stripe[0], F_SETFL, O_NONBLOCK);
    fcntl(stripe_to_avn[0], F_SETFL, O_NONBLOCK);
    fcntl(stripe_to_airline[0], F_SETFL, O_NONBLOCK);
    fcntl(avn_to_atcs[0], F_SETFL, O_NONBLOCK);
    
    pid_t ATCS_id = fork();
    if (ATCS_id == 0){
        // ATCS Controller process
        // Close unused ends of pipes
        close(atcs_to_avn[0]);      // Close reading end
        close(avn_to_atcs[1]);      // Close writing end
        close(avn_to_airline[0]);   // Close unused pipes
        close(avn_to_airline[1]);
        close(avn_to_stripe[0]);
        close(avn_to_stripe[1]);
        close(stripe_to_avn[0]);
        close(stripe_to_avn[1]);
        close(stripe_to_airline[0]);
        close(stripe_to_airline[1]);
        
        ATCSystem atc(atcs_to_avn, avn_to_atcs);
        atc.startSimulation();
        
        close(atcs_to_avn[1]);
        close(avn_to_atcs[0]);
        
        exit(0); // Exit child process
    }

    pid_t AVN_Generator_id = fork();
    if (AVN_Generator_id == 0){
        // AVN Generator process
        
        // Close unused ends of pipes
        close(atcs_to_avn[1]);      // Close writing end
        close(avn_to_atcs[0]);      // Close reading end
        close(avn_to_airline[0]);   // Close reading end
        close(avn_to_stripe[0]);    // Close reading end
        close(stripe_to_avn[1]);    // Close writing end
        close(stripe_to_airline[0]); // Close unused pipe
        close(stripe_to_airline[1]);
        
        AVNGenerator avnGen(atcs_to_avn, avn_to_atcs, avn_to_airline, avn_to_stripe, stripe_to_avn);
        avnGen.generateAVN();
        
        close(atcs_to_avn[0]);
        close(avn_to_atcs[1]);
        close(avn_to_airline[1]);
        close(avn_to_stripe[1]);
        close(stripe_to_avn[0]);
        
        exit(0); // Exit child process
    }

    pid_t ALPortal_id = fork();
    if (ALPortal_id == 0){
        // Airline portal process
        
        // Close unused ends of pipes
        close(atcs_to_avn[0]);
        close(atcs_to_avn[1]);
        close(avn_to_atcs[0]);
        close(avn_to_atcs[1]);
        close(avn_to_airline[1]);   // Close writing end
        close(avn_to_stripe[0]);
        close(avn_to_stripe[1]);
        close(stripe_to_avn[0]);
        close(stripe_to_avn[1]);
        close(stripe_to_airline[1]); // Close writing end
        
        AirlinePortal airlinePortal(avn_to_airline, stripe_to_airline);
        airlinePortal.processPayment();
        
        close(avn_to_airline[0]);
        close(stripe_to_airline[0]);
        
        exit(0); // Exit child process
    }

    pid_t StripePayment_id = fork();
    if (StripePayment_id == 0){
        // Stripe payment process
        
        // Close unused ends of pipes
        close(atcs_to_avn[0]);
        close(atcs_to_avn[1]);
        close(avn_to_atcs[0]);
        close(avn_to_atcs[1]);
        close(avn_to_airline[0]);
        close(avn_to_airline[1]);
        close(avn_to_stripe[1]);    // Close writing end
        close(stripe_to_avn[0]);    // Close reading end
        close(stripe_to_airline[0]); // Close reading end
        
        StripePayment stripePayment(avn_to_stripe, stripe_to_avn, stripe_to_airline);
        stripePayment.confirmPayment();
        
        close(avn_to_stripe[0]);
        close(stripe_to_avn[1]);
        close(stripe_to_airline[1]);
        
        exit(0); // Exit child process
    }

    // Parent process - close all pipe ends since they are only used by children
    close(atcs_to_avn[0]);
    close(atcs_to_avn[1]);
    close(avn_to_airline[0]);
    close(avn_to_airline[1]);
    close(avn_to_stripe[0]);
    close(avn_to_stripe[1]);
    close(stripe_to_avn[0]);
    close(stripe_to_avn[1]);
    close(stripe_to_airline[0]);
    close(stripe_to_airline[1]);
    close(avn_to_atcs[0]);
    close(avn_to_atcs[1]);

    // Wait for child processes to finish
    waitpid(ATCS_id, NULL, 0);
    waitpid(AVN_Generator_id, NULL, 0);
    waitpid(ALPortal_id, NULL, 0);
    waitpid(StripePayment_id, NULL, 0);
    
    std::cout << "Simulation completed.\n";
    return 0;
}