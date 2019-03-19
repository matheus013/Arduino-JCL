#ifndef Sensor_h
#define Sensor_h

#include <Arduino.h>
#include "context.h"
#include <string>
#include "helpers.h"

const int MAX_CONTEXTS = 15;
using namespace std;

class Sensor {
public:
    Sensor();

    void deleteSensor();

    bool acting(float value);

    void configurePinMode();

    boolean digitalPin();

    static boolean validPin(int pin);

    int getAnalogPin();

    long count;

    string toString();

private:

ATTRIBUTE_OBJECT(char*, sensorNickname)
ATTRIBUTE_OBJECT(char, typeIO)
ATTRIBUTE_OBJECT(char*, pin)
ATTRIBUTE_OBJECT(char*, delay)
ATTRIBUTE_OBJECT(long, lastExecuted)
ATTRIBUTE_OBJECT(float, value)
ATTRIBUTE_OBJECT(int, type)
ATTRIBUTE_OBJECT(char*, sensorSize)
ATTRIBUTE_OBJECT(uint8_t, numContexts)
    Context *enabledContexts[MAX_CONTEXTS];


};

#endif
