#ifndef Context_h
#define Context_h

#define MAX_NUMBER_DIGITS 8
#define MAX_ACTIONS_PER_CONTEXT 6
#define OPERATORS_MAX_SIZE 3

#include "action.h"
#include "helpers.h"
#include <string>

using namespace std;

class Context {
public:
    Context();

    Action **getEnabledActions();

    string toString();

    void deleteContext();

private:
ATTRIBUTE_OBJECT(char*, nickname)
ATTRIBUTE_OBJECT(char*, expression)
ATTRIBUTE_OBJECT(char**, operators)
ATTRIBUTE_OBJECT(char**, threshold)
ATTRIBUTE_OBJECT(int, numExpressions)
ATTRIBUTE_OBJECT(int, numActions)
ATTRIBUTE_BOOL(triggered)
ATTRIBUTE_BOOL(mqttContext)
    Action *enabledActions[MAX_ACTIONS_PER_CONTEXT];
};

#endif
