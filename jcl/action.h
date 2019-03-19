#ifndef Action_h
#define Action_h

#include <Arduino.h>
#include "helpers.h"

class Action {
ATTRIBUTE_OBJECT(char*, className)
ATTRIBUTE_OBJECT(char*, methodName)
ATTRIBUTE_OBJECT(char*, hostIP)
ATTRIBUTE_OBJECT(char*, hostPort)
ATTRIBUTE_OBJECT(char*, hostMac)
ATTRIBUTE_OBJECT(char*, superPeerPort)
ATTRIBUTE_OBJECT(char*, ticket)
ATTRIBUTE_OBJECT(char*, param)
ATTRIBUTE_OBJECT(uint16_t, paramSize)
ATTRIBUTE_OBJECT(uint8_t, classNameSize)
ATTRIBUTE_OBJECT(uint8_t, methodNameSize)
ATTRIBUTE_BOOL(useSensorValue)
ATTRIBUTE_BOOL(acting)
public:
    void deleteAction();
};

#endif // Action_h
