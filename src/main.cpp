#include <Arduino.h>
#include "Valve.h"

uint8_t potentiometerPin = 17;

uint8_t openRelayPin = 12;
uint8_t closeRelayPin = 13;
uint8_t targetPrecision = 0.05;
uint8_t targetHysteresis = 0.15;

ValvePotentiometer valvePotentiometer(potentiometerPin);
Valve valve(openRelayPin, closeRelayPin, targetPrecision, targetHysteresis, &valvePotentiometer);

void setup() {
	delay(1000);
	Serial.begin(9600); Serial.println("");
	valve.begin();

	valve.calibrate();
}

void loop() {
	// TODO: get temp and decide on a new target
	// für temp evtl rounden dann lookup + clamp? RÜCKSPRACHE

	Serial.println(valve.currentState);
	valve.update();
}