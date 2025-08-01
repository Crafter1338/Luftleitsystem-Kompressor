#include "Arduino.h"
#include "Valve.h"

// | ValvePotentiometer Implementation
// Das ValvePotentiometer erfasst die Position des Stellmotors und verarbeitet diese für das Valve
ValvePotentiometer::ValvePotentiometer(uint8_t pin) // Konstruktor --> Variablen initialisieren
    : begun(false), pin(pin), openedAnalog(0), closedAnalog(0), currentAnalog(0) {}

void ValvePotentiometer::begin() { // Pins initialisieren
    if (this->begun) return; // nicht mehrmals beginnen

    pinMode(this->pin, INPUT);

    this->begun = true;
}

float ValvePotentiometer::getState() {
    if (!this->begun) return 0.0; // Protection

    if (openedAnalog == closedAnalog) return 0.0; // Nicht durch 0 teilen

    float state = float(currentAnalog - closedAnalog) / float(openedAnalog - closedAnalog);

    if (closedAnalog > openedAnalog) state = 1.0 - state; // inversion falls closed aus irgendeinem Grund höher ist als opened --> 0.0 immer ganz zu, 1.0 immer ganz offen

    if (state < 0.0) state = 0.0; // clamp lower
    if (state > 1.0) state = 1.0; // clamp upper

    return state;
}

void ValvePotentiometer::update() {
    if (!this->begun) return; // Protection

    this->currentAnalog = analogRead(this->pin);
}
//



// | Valve Implementation
// Das Valve steuert den Stellmotor über .setTargetState()
Valve::Valve(uint8_t openRelay, uint8_t closeRelay, float targetPrecision, float targetHysteresis, ValvePotentiometer *potentiometer) // Konstruktor --> Variablen initialisieren
    : begun(false), isActing(false), calibrated(false), actionDirection(0), currentState(0.0), targetState(0.0),
    openRelay(openRelay), closeRelay(closeRelay), targetPrecision(targetPrecision), targetHysteresis(targetHysteresis), potentiometer(potentiometer) {}

void Valve::begin() { // Pins initialisieren, falls noch nicht geschehen: Potentiometer initialisieren
    if (this->begun) return; // nicht mehrmals beginnen

    pinMode(this->openRelay, OUTPUT);
    pinMode(this->closeRelay, OUTPUT);

    this->potentiometer->begin();

    this->begun = true;
}

float Valve::targetDifference() { // Differenz zwischen Soll- und Istwert
    if (!this->begun) return; // Protection

    return (float) this->targetState - this->currentState;
}

float Valve::absTargetDifference() { // Differenz zwischen Soll- und Istwert (absolut)
    if (!this->begun) return; // Protection

    return fabs(this->targetDifference());
}

void Valve::calibrate() { // Kalibrationsfahrt um den Potentiometer einzustellen
    if (!this->begun) return; // Protection

    const uint8_t stableThreshold = 10; // Differenz zwischen Messungen, ab wann Stillstand ist --> Achtung: Noise
    const uint8_t stableCountRequired = 15; // Anzahl stabiler Messungen in Folge
    const uint8_t delayMs = 50; // Wartezeit zwischen Messungen

    // Öffnen
    this->startOpenAction();
    int lastAnalog = -1;
    int stableCount = 0;
    while (true) {
        this->potentiometer->update();
        int currentAnalog = this->potentiometer->currentAnalog;

        if (lastAnalog >= 0 && abs(currentAnalog - lastAnalog) < stableThreshold) {
            stableCount++;
        } else {
            stableCount = 0;
        }
        // TODO: evtl max zeit begrezung? RÜCKSPRACHE

        if (stableCount >= stableCountRequired) {
            break; // stabile Werte --> Ende erreicht
        }

        lastAnalog = currentAnalog;
        delay(delayMs);
    }
    this->stopAction();
    this->potentiometer->openedAnalog = this->potentiometer->currentAnalog;

    delay(1000);

    // Schließen
    this->startCloseAction();
    lastAnalog = -1;
    stableCount = 0;
    while (true) {
        this->potentiometer->update();
        int currentAnalog = this->potentiometer->currentAnalog;

        if (lastAnalog >= 0 && abs(currentAnalog - lastAnalog) < stableThreshold) {
            stableCount++;
        } else {
            stableCount = 0;
        }

        if (stableCount >= stableCountRequired) {
            break; // stabile Werte --> Ende erreicht
        }

        lastAnalog = currentAnalog;
        delay(delayMs);
    }
    this->stopAction();
    this->potentiometer->closedAnalog = this->potentiometer->currentAnalog;

    this->calibrated = true;
}

void Valve::setTargetState(float newTargetState) {
    if (!this->calibrated) return; // Protection

    this->targetState = newTargetState;
}

void Valve::startOpenAction() { // Klappe öffnen --> vorherige Aktion beenden
    if (!this->calibrated) return; // Protection

    digitalWrite(this->closeRelay, LOW);
    digitalWrite(this->openRelay, HIGH);

    this->isActing = true;
    this->actionDirection = 1;
}

void Valve::startCloseAction() { // Klappe schließen --> vorherige Aktion beenden
    if (!this->calibrated) return; // Protection

    digitalWrite(this->openRelay, LOW);
    digitalWrite(this->closeRelay, HIGH);

    this->isActing = true;
    this->actionDirection = -1;
}

void Valve::stopAction() { // Klappe stillstand --> laufende Aktion beenden 
    if (!this->begun) return; // Protection

    digitalWrite(this->openRelay, LOW);
    digitalWrite(this->closeRelay, LOW);

    this->isActing = false;
    this->actionDirection = 0;
}

void Valve::update() {
    if (!this->calibrated) return; // Protection

    this->potentiometer->update();
    this->currentState = this->potentiometer->getState();

    float difference = this->targetDifference();

    if (this->isActing){ // Wenn eine Aktion ausgeführt wird (öffnen / schließen)
        // Prüfen, ob wir die Toleranz in der richtigen Richtung erreicht haben
        if (this->actionDirection > 0 && difference <= this->targetPrecision) { 
            this->stopAction();
            return;
        }
        if (this->actionDirection < 0 && difference >= -this->targetPrecision) {
            this->stopAction();
            return;
        }
    } else {
        if (difference > this->targetHysteresis) { // Wenn die Differenz größer als der Hysteresenwert ist, ist der Sollwert größer als der Istwert --> Öffnen
            this->startOpenAction();
            return;
        }

        if (difference < -this->targetHysteresis) { // Wenn die Differenz kleiner als der Hysteresenwert ist, ist der Sollwert kleiner als der Istwert --> Schließen
            this->startCloseAction();
            return;
        }
    }
}
///