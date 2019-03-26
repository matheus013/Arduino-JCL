#include "context.h"
#include "action.h"

using namespace std;

Context::Context()
{
    this->m_mqttContext = false;
    this->m_operators = new char *[MAX_ACTIONS_PER_CONTEXT];
    for (int i = 0; i < MAX_ACTIONS_PER_CONTEXT; i++)
        this->m_operators[i] = new char[OPERATORS_MAX_SIZE];
    this->m_threshold = new char *[MAX_ACTIONS_PER_CONTEXT];
    for (int i = 0; i < MAX_ACTIONS_PER_CONTEXT; i++)
        this->m_threshold[i] = new char[MAX_NUMBER_DIGITS];

    this->m_numExpressions = 0;
    for (uint8_t x = 0; x < MAX_ACTIONS_PER_CONTEXT; x++)
        this->enabledActions[x] = NULL;
    this->m_numActions = 0;
    this->m_triggered = false;
}

void Context::deleteContext()
{
    for (int i = 0; i < m_numActions; i++)
        enabledActions[i]->deleteAction();
    free(this->m_nickname);
    free(this->m_expression);
    for (int i = 0; i < MAX_ACTIONS_PER_CONTEXT; i++)
    {
        free(this->m_operators[i]);
        free(this->m_threshold[i]);
    }
    free(this->m_operators);
    free(this->m_threshold);
    free(this);
}

Action **Context::getEnabledActions()
{
    return this->enabledActions;
}

String Context::toString()
{
    String str = "";
    str += get_nickname();
    str += " ";
    str += get_expression();
    str += " ";
    str.concat(get_numActions());
    str += " ";
    str.concat(get_numExpressions());
    return str;
}