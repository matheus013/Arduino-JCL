//
// Created by Matheus Inácio on 2019-03-19.
//

#include "sensor.h"
#include "constants.h"
#include "jcl.h"
#include <Servo.h>

// using namespace std;

Sensor::Sensor()
{
    m_lastExecuted = 0;
    count = 0;
    this->m_numContexts = 0;
    for (uint8_t x = 0; x < MAX_CONTEXTS; x++)
        enabledContexts[x] = NULL;
}

bool Sensor::acting(float value)
{
    if (this->m_type == Constants::SERVO_ACTUATOR)
    {
        Servo servo;
        servo.attach(atoi(this->m_pin));
        servo.write(value);
        return true;
    }
    else if (digitalPin())
    {
        if (value != LOW && value != HIGH)
            return false;
        digitalWrite(atoi(this->m_pin), value);
    }
    else
    {
        if (value < 0 || value > 255)
            return false;
        analogWrite(getAnalogPin(), value);
    }
    return true;
}

void Sensor::deleteSensor()
{
    for (int i = 0; i < m_numContexts; i++)
    {
        if (enabledContexts[i] != NULL)
        {
            enabledContexts[i]->deleteContext();
            enabledContexts[i] = NULL;
        }
    }
    free(this->m_pin);
    free(this->m_sensorNickname);
    free(this);
}

void Sensor::configurePinMode()
{
    if (this->m_type == Constants::SERVO_ACTUATOR)
    {
        Servo servo;
        servo.attach(atoi(m_pin));
    }
    else if (digitalPin())
    {
        if (m_typeIO == Constants::CHAR_OUTPUT)
            pinMode(atoi(m_pin), OUTPUT);
        else
            pinMode(atoi(m_pin), INPUT);
    }
}

bool Sensor::digitalPin()
{
    return atoi(m_pin) < JCL::get_totalDigitalSensors();
}

bool Sensor::validPin(int pin)
{
    return !(pin < 0 || pin >= JCL::get_totalSensors());
}

int Sensor::getAnalogPin()
{
    return atoi(m_pin) - JCL::get_totalDigitalSensors();
}

String getString(char x)
{
    // string class has a constructor
    // that allows us to specify size of
    // string as first parameter and character
    // to be filled in given size as second
    // parameter.
    // String s(x);
    return String(x);
}

String Sensor::toString()
{
    String str;
    str += get_pin();
    str += " ";
    str += get_sensorNickname();
    str += " ";
    str += get_sensorSize();
    str += " ";
    str += getString(get_typeIO());
    str += " ";
    str.concat(get_type());
    str += " ";
    str.concat(get_numContexts());
    str += " contexts";
    return str;
}

Context **Sensor::getEnabledContexts()
{
    return this->enabledContexts;
}