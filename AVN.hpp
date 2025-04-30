#pragma once

#include <string>
#include "Flight.hpp"

struct AVN{
    int id;
    Flight* flight;
    double recordedSpeed;
    double allowedSpeed;
    bool isPaid;
    time_t issueTime;
    AVN(Flight* flight, double recordedSpeed, double allowedSpeed, bool isPaid = false)
        : flight(flight), recordedSpeed(recordedSpeed), allowedSpeed(allowedSpeed), isPaid(isPaid) {
        static int nextId = 1;
        id = nextId++;
        issueTime = time(nullptr);
    }

};