//
// Created by Matheus Inácio on 2019-03-19.
//

#ifndef Metadata_h
#define Metadata_h

#include <Arduino.h>
#include "helpers.h"

class Metadata {
public:
    Metadata();

private:
ATTRIBUTE_OBJECT(char*, boardName)
ATTRIBUTE_OBJECT(char*, numConfiguredSensors)
ATTRIBUTE_OBJECT(char*, hostIP)
ATTRIBUTE_OBJECT(char*, hostPort)

ATTRIBUTE_OBJECT(char*, serverIP)
ATTRIBUTE_OBJECT(char*, serverPort)
ATTRIBUTE_OBJECT(char*, mac)

ATTRIBUTE_OBJECT(char*, brokerIP)
ATTRIBUTE_OBJECT(int, brokerPort)
ATTRIBUTE_BOOL(standBy)
};

#endif