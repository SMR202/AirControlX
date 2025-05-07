#pragma once

#include <iostream>
#include <string>
#include <ctime>

// Message structures for IPC
struct ViolationDetails {
    int avnId;
    char aircraftId[20];
    char AirlineName[20];
    double speed;
    double allowedSpeed;
    time_t timestamp;
};

struct AVNNotice {
    int avnId;
    char aircraftId[20];
    char AirlineName[30];
    double recordedSpeed;
    double allowedSpeed;
    char aircraftType[20];
    char flightNumber[20];
    double totalFine;
    time_t timestamp;
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