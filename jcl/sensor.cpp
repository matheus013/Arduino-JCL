//
// Created by Matheus Inácio on 2019-03-19.
//

#include "sensor.h"
#include "constants.h"
#include "jcl.h"
#include <Servo.h>

Sensor::Sensor() {
    lastExecuted = 0;
    count = 0;
    this->numContexts = 0;
    for (uint8_t x = 0; x < MAX_CONTEXTS; x++)
        enabledContexts[x] = NULL;
}

bool Sensor::acting(float value) {
    if (this->type == Constants::SERVO_ACTUATOR) {
        Servo servo;
        servo.attach(atoi(this->pin));
        servo.write(value);
        return true;
    } else if (digitalPin()) {
        if (value != LOW && value != HIGH)
            return false;
        digitalWrite(atoi(this->pin), value);
    } else {
        if (value < 0 || value > 255)
            return false;
        analogWrite(getAnalogPin(), value);
    }
    return true;
}

void Sensor::deleteSensor() {
    for (int i = 0; i < numContexts; i++) {
        if (enabledContexts[i] != NULL) {
            enabledContexts[i]->deleteContext();
            enabledContexts[i] = NULL;
        }
    }
    free(this->pin);
    free(this->sensorNickname);
    free(this);
}

void Sensor::configurePinMode() {
    if (this->type == Constants::SERVO_ACTUATOR) {
        Servo servo;
        servo.attach(atoi(pin));
    } else if (digitalPin()) {
        if (typeIO == Constants::CHAR_OUTPUT)
            pinMode(atoi(pin), OUTPUT);
        else
            pinMode(atoi(pin), INPUT);
    }
}

boolean Sensor::digitalPin() {
    return atoi(pin) < JCL::get_totalDigitalSensors();
}

boolean Sensor::validPin(int pin) {
    return !(pin < 0 || pin >= JCL::get_totalSensors());
}

int Sensor::getAnalogPin() {
    return atoi(pin) - JCL::get_totalDigitalSensors();
}

string Sensor::toString() {
    string str = "";
    str.append(get_pin()).append(" ");
    str.append(get_sensorNickname()).append(" ");
    str.append(get_sensorSize()).append(" ");
    str.append(get_typeIO()).append(" ");
    str.append(get_type()).append(" ");
    str.append(get_numContexts()).append(" contexts");
    return str;
}