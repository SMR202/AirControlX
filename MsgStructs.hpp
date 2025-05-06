#pragma once

#include <iostream>
#include <string>
#include <ctime>
// Message structures for IPC
struct ViolationDetails {
    int aircraftId;
    char aircraftType[20];
    double speed;
    char position[20];
    time_t timestamp;
};

struct AVNNotice {
    int avnId;
    int aircraftId;
    char aircraftType[20];
    double recordedSpeed;
    double allowedSpeed;
    char airlineName[30];
    char flightNumber[20];
};

struct PaymentRequest {
    int avnId;
    int aircraftId;
    char aircraftType[20];
    double totalFine;
};

struct PaymentConfirmation {
    int avnId;
    char status[10]; // "paid"
};

struct ViolationClearance {
    int avnId;
    int aircraftId;
    char status[20]; // "cleared"
};