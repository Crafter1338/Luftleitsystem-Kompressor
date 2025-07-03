#include <Arduino.h>
#include "Valve.h"

ValvePotentiometer valvePotentiometer(17);
Valve valve(12, 13, 0.05, 0.1, &valvePotentiometer);

void setup() {
	valve.begin();

	valve.calibrate();
}

void loop() {
	// TODO: get temp and decide on a new target

	valve.update();
}