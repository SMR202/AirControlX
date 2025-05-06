#pragma once
#include <iostream>
#include <unistd.h>

using namespace std;

class AVNGenerator {
    private:
    int atcs_to_avn[2];
    int avn_to_atcs[2];
    int avn_to_airline[2];
    int avn_to_stripe[2];
    int stripe_to_avn[2];
    
    
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

    void generateAVN() {
        // Simulate AVN generation
        cout << "Generating AVN..." << endl;
        sleep(1); // Simulate time taken to generate AVN
        cout << "AVN generated." << endl;
    }
};