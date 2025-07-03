#include "Arduino.h"

#ifndef VALVE_H
#define VALVE_H

class ValvePotentiometer {
    private:
        bool begun;

        uint8_t pin;

    public:
        ValvePotentiometer(uint8_t pin);

        uint16_t openedAnalog;
        uint16_t closedAnalog;
        uint16_t currentAnalog;

        void begin();
        void update();

        float getState();
};

class Valve {
    private:
        bool begun;
        bool calibrated;
        
        bool isActing;
        int actionDirection;

        uint8_t openRelay;
        uint8_t closeRelay;

        float targetPrecision;
        float targetHysteresis;

        ValvePotentiometer *potentiometer;

        float targetDifference();
        float absTargetDifference();
    
    public:
        Valve(uint8_t openRelay, uint8_t closeRelay, float targetPrecision, float targetHysteresis, ValvePotentiometer *potentiometer);

        float currentState;
        float targetState;

        void begin();
        void calibrate();
        void update();

        void setTargetState(float newTargetState);

        void startOpenAction();
        void startCloseAction();
        void stopAction();
};

#endif