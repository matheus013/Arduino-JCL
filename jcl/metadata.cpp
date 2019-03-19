//
// Created by Matheus Inácio on 2019-03-19.
//

#include "metadata.h"

Metadata::Metadata(){
    char name[] = "arduino", nSensors[] = "0";
    this->set_standBy(false);
    this->set_soardName(name);
    this->set_sumConfiguredSensors(nSensors);
}