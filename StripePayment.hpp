#pragma once

#include <iostream>
#include <unistd.h>

class StripePayment{
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

    void run(){
        std::cout<<"Checking Payment in Stripe\n";
        sleep(1);
    }
};