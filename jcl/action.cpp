#include "action.h"

void Action::deleteAction(){
  free(m_className);
  free(m_methodName);
  free(m_hostIP);
  free(m_hostPort);
  free(m_hostMac);
  free(m_ticket);
  free(m_param);
  free(this);
}