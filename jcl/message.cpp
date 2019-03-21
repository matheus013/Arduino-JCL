//
// Created by Matheus Inácio on 2019-03-19.
//

#include "message.h"

void (*resetFunc1)(void) = 0;


Message::Message(JCL *jcl, int messageSize) {
    this->m_jcl = jcl;
    this->m_messageSize = messageSize;
}

Message::Message(JCL *jcl) {
    this->m_jcl = jcl;
}

void Message::sendMetadata(int messageType) {
    int totalSize;
    int size;
    int i;

    /* To calculate the size of the metadata ENABLE_SENSOR*/
    int enableSensorSize = 0;
    for (i = 0; i < JCL::get_totalSensors(); i++)
        if (m_jcl->get_sensors()[i] != NULL)
            enableSensorSize += strlen(m_jcl->get_sensors()[i]->get_pin());

    enableSensorSize += atoi(m_jcl->get_metadata()->get_numConfiguredSensors());
    enableSensorSize++;

    int totalSizePins = 0, totalSizeNameSensor = 0, totalTimeSensor = 0, totalDirSensor = 0, totalSensorSize = 0;
    for (i = 0; i < JCL::get_totalSensors(); i++) {
        if (m_jcl->get_sensors()[i] != NULL) {
            totalSizePins += strlen(m_jcl->get_sensors()[i]->get_pin());
            totalSizeNameSensor += strlen(m_jcl->get_sensors()[i]->get_sensorNickname());
            totalTimeSensor += strlen(m_jcl->get_sensors()[i]->get_delay());
            totalSensorSize += strlen(m_jcl->get_sensors()[i]->get_sensorSize());
            totalDirSensor++;
        }
    }

    int currentPosition = 0;
    m_jcl->message[currentPosition++] = 8; // Indicates first field

    /* The next fields indicates the value -1. It takes multiple bytes to indicate negative field because
    Protocol Buffer uses base 128 varints */
    if (messageType == -1) {
        m_jcl->message[currentPosition++] = -1;
        m_jcl->message[currentPosition++] = -1;
        m_jcl->message[currentPosition++] = -1;
        m_jcl->message[currentPosition++] = -1;
        m_jcl->message[currentPosition++] = -1;
        m_jcl->message[currentPosition++] = -1;
        m_jcl->message[currentPosition++] = -1;
        m_jcl->message[currentPosition++] = -1;
        m_jcl->message[currentPosition++] = -1;
        m_jcl->message[currentPosition++] = 1;
    } else
        m_jcl->message[currentPosition++] = messageType;

    m_jcl->message[currentPosition++] = 18; // Indicates second field
    int numberConfiguredSensors = atoi(m_jcl->get_metadata()->get_numConfiguredSensors());
    int sensorsSize = 0;
    if (numberConfiguredSensors != 0) {
        //13, 15, 13, 14, 18
        //13, 13, 11, 12, 16
        //strlen(Constants::enableSensorMessage), strlen(Constants::sensorAliasMessage), strlen(Constants::sensorDirMessage),
        //strlen(Constants::sensorSizeMessage), strlen(Constants::sensorSamplingMessage)
        sensorsSize += enableSensorSize + (76 * numberConfiguredSensors) +
                       totalSizeNameSensor + totalDirSensor + totalSensorSize + totalTimeSensor +
                       4 * totalSizePins + 19;
    } else
        sensorsSize += 19 + enableSensorSize;

    int standBySize = m_jcl->get_metadata()->is_standBy() ? 4 : 5; // srtlen("true") : strlen("false")
    // Calculate size to append to the message
    //4, 7, 1, 11, 1, 9, 3, 15, 12, 14, 7
    //strlen(Constants::portMessage), strlen(Constants::coreMessage), strlen(Constants::coreValue), strlen(Constants::deviceTypeMessage),
    //strlen(Constants::deviceTypeValue), strlen(Constants::deviceIDMessage), strlen(Constants::macMessage), strlen(Constants::devicePlatformValue)
    //strlen(Constants::numberSensorsMessage), strlen(Constants::standByMessage)
    //strlen(Constants::Constants::devicePlatformMessage)
    totalSize =
            strlen(m_jcl->get_metadata()->get_hostPort()) + strlen(m_jcl->get_metadata()->get_boardName()) +
            strlen(m_jcl->get_metadata()->get_mac()) + strlen(m_jcl->get_metadata()->get_numConfiguredSensors()) +
            standBySize + sensorsSize + 132;

    if (messageType == -1) {
        totalSize += strlen(m_jcl->get_metadata()->get_hostIP()) + 8;
    }
    /* If total size is less then 128 we only need one byte to store it */
    if (totalSize < 128)
        m_jcl->message[currentPosition++] = totalSize;
    else {
        int lastByte = (int) totalSize / 128;
        int rest = 128 - ((int) totalSize % 128);
        rest *= -1;
        m_jcl->message[currentPosition++] = rest;
        m_jcl->message[currentPosition++] = lastByte;
    }

    /* The following values are the Metadata in the Map format (Key, Value) */

    /* Setting the value of the DEVICE_ID */
    m_jcl->message[currentPosition++] = 10;
    m_jcl->message[currentPosition++] =
            strlen(m_jcl->get_metadata()->get_boardName()) + 13; // strlen(Constants::deviceIDMessage) = 9
    m_jcl->message[currentPosition++] = 10;
    m_jcl->message[currentPosition++] = 9;
    for (unsigned int i = 0; i < 9; i++)
        m_jcl->message[currentPosition++] = Constants::deviceIDMessage[i];
    m_jcl->message[currentPosition++] = 18;
    int boardNameSize = m_jcl->message[currentPosition++] = strlen(m_jcl->get_metadata()->get_boardName());
    for (unsigned int i = 0; i < boardNameSize; i++)
        m_jcl->message[currentPosition++] = m_jcl->get_metadata()->get_boardName()[i];

    /* Setting the value of the DEVICE_TYPE */
    m_jcl->message[currentPosition++] = 10;
    m_jcl->message[currentPosition++] = strlen(Constants::deviceTypeMessage) + strlen(Constants::deviceTypeValue) + 4;
    m_jcl->message[currentPosition++] = 10;
    m_jcl->message[currentPosition++] = strlen(Constants::deviceTypeMessage);
    for (unsigned int i = 0; i < strlen(Constants::deviceTypeMessage); i++)
        m_jcl->message[currentPosition++] = Constants::deviceTypeMessage[i];
    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = strlen(Constants::deviceTypeValue);
    for (unsigned int i = 0; i < strlen(Constants::deviceTypeValue); i++)
        m_jcl->message[currentPosition++] = Constants::deviceTypeValue[i];

    /* Setting the value of the PORT */
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] = strlen(Constants::portMessage) + strlen(jcl->getMetadata()->getHostPort()) + 4;
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] = strlen(Constants::portMessage);
    for (unsigned int i = 0; i < strlen(Constants::portMessage); i++)
        jcl->message[currentPosition++] = Constants::portMessage[i];
    jcl->message[currentPosition++] = 18;
    jcl->message[currentPosition++] = strlen(jcl->getMetadata()->getHostPort());
    for (unsigned int i = 0; i < strlen(jcl->getMetadata()->getHostPort()); i++)
        jcl->message[currentPosition++] = jcl->getMetadata()->getHostPort()[i];

    if (messageType == -1) {
        /* Setting the value of the IP */
        jcl->message[currentPosition++] = 10;
        jcl->message[currentPosition++] = strlen(Constants::ipMessage) + strlen(jcl->getMetadata()->getHostIP()) + 4;
        jcl->message[currentPosition++] = 10;
        jcl->message[currentPosition++] = strlen(Constants::ipMessage);
        for (unsigned int i = 0; i < strlen(Constants::ipMessage); i++)
            jcl->message[currentPosition++] = Constants::ipMessage[i];
        jcl->message[currentPosition++] = 18;
        jcl->message[currentPosition++] = strlen(jcl->getMetadata()->getHostIP());
        for (unsigned int i = 0; i < strlen(jcl->getMetadata()->getHostIP()); i++)
            jcl->message[currentPosition++] = jcl->getMetadata()->getHostIP()[i];
    }

    /* Setting the value of the CORE(S) */
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] = strlen(Constants::coreMessage) + strlen(Constants::coreValue) + 4;
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] = strlen(Constants::coreMessage);
    for (unsigned int i = 0; i < strlen(Constants::coreMessage); i++)
        jcl->message[currentPosition++] = Constants::coreMessage[i];
    jcl->message[currentPosition++] = 18;
    jcl->message[currentPosition++] = strlen(Constants::coreValue);
    for (unsigned int i = 0; i < strlen(Constants::coreValue); i++)
        jcl->message[currentPosition++] = Constants::coreValue[i];

    /* Setting the value of the MAC */
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] = strlen(Constants::macMessage) + strlen(jcl->getMetadata()->getMAC()) + 4;
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] = strlen(Constants::macMessage);
    for (unsigned int i = 0; i < strlen(Constants::macMessage); i++)
        jcl->message[currentPosition++] = Constants::macMessage[i];
    jcl->message[currentPosition++] = 18;
    jcl->message[currentPosition++] = strlen(jcl->getMetadata()->getMAC());
    for (unsigned int i = 0; i < strlen(jcl->getMetadata()->getMAC()); i++)
        jcl->message[currentPosition++] = jcl->getMetadata()->getMAC()[i];

    /* Setting the value of the Device Platform */
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] =
            strlen(Constants::devicePlatformMessage) + strlen(Constants::devicePlatformValue) + 4;
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] = strlen(Constants::devicePlatformMessage);
    for (unsigned int i = 0; i < strlen(Constants::devicePlatformMessage); i++)
        jcl->message[currentPosition++] = Constants::devicePlatformMessage[i];
    jcl->message[currentPosition++] = 18;
    jcl->message[currentPosition++] = strlen(Constants::devicePlatformValue);
    for (unsigned int i = 0; i < strlen(Constants::devicePlatformValue); i++)
        jcl->message[currentPosition++] = Constants::devicePlatformValue[i];

    /* Setting the value of the NUMBER_SENSORS */
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] =
            strlen(Constants::numberSensorsMessage) + strlen(jcl->getMetadata()->getNumConfiguredSensors()) + 4;
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] = strlen(Constants::numberSensorsMessage);
    for (unsigned int k = 0; k < strlen(Constants::numberSensorsMessage); k++)
        jcl->message[currentPosition++] = Constants::numberSensorsMessage[k];
    jcl->message[currentPosition++] = 18;
    jcl->message[currentPosition++] = strlen(jcl->getMetadata()->getNumConfiguredSensors());
    for (unsigned int k = 0; k < strlen(jcl->getMetadata()->getNumConfiguredSensors()); k++)
        jcl->message[currentPosition++] = jcl->getMetadata()->getNumConfiguredSensors()[k];

    /* Setting the value of the STANDBY */
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] = strlen(Constants::standByMessage) + standBySize + 4;
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] = strlen(Constants::standByMessage);
    for (unsigned int k = 0; k < strlen(Constants::standByMessage); k++)
        jcl->message[currentPosition++] = Constants::standByMessage[k];
    jcl->message[currentPosition++] = 18;
    if (jcl->getMetadata()->isStandBy()) {
        jcl->message[currentPosition++] = strlen(Constants::trueMessage);
        for (unsigned int k = 0; k < strlen(Constants::trueMessage); k++)
            jcl->message[currentPosition++] = Constants::trueMessage[k];
    } else {
        jcl->message[currentPosition++] = strlen(Constants::falseMessage);
        for (unsigned int k = 0; k < strlen(Constants::falseMessage); k++)
            jcl->message[currentPosition++] = Constants::falseMessage[k];
    }

//  if (atoi(jcl->getMetadata()->getNumConfiguredSensors()) != 0){

    /* Setting the value of the ENABLE_SENSOR */
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] = strlen(Constants::enableSensorMessage) + enableSensorSize + 4;
    jcl->message[currentPosition++] = 10;
    jcl->message[currentPosition++] = strlen(Constants::enableSensorMessage);
    for (unsigned int k = 0; k < strlen(Constants::enableSensorMessage); k++)
        jcl->message[currentPosition++] = Constants::enableSensorMessage[k];
    jcl->message[currentPosition++] = 18;
    jcl->message[currentPosition++] = enableSensorSize;

    for (int k = 0; k < JCL::getTotalSensors(); k++) {
        if (jcl->getSensors()[k] != NULL) {
            for (unsigned i = 0; i < strlen(jcl->getSensors()[k]->getPin()); i++)
                jcl->message[currentPosition++] = jcl->getSensors()[k]->getPin()[i];
            jcl->message[currentPosition++] = ';';
        }
    }
    jcl->message[currentPosition++] = ';';

    /* Setting the value of the SENSOR_ALIAS */
    for (i = 0; i < JCL::getTotalSensors(); i++) {
        if (jcl->getSensors()[i] != NULL) {
            jcl->message[currentPosition++] = 10;
            jcl->message[currentPosition++] =
                    strlen(Constants::sensorAliasMessage) + strlen(jcl->getSensors()[i]->getPin()) +
                    strlen(jcl->getSensors()[i]->getSensorNickname()) + 4;
            jcl->message[currentPosition++] = 10;
            jcl->message[currentPosition++] =
                    strlen(Constants::sensorAliasMessage) + strlen(jcl->getSensors()[i]->getPin());
            for (unsigned int k = 0; k < strlen(Constants::sensorAliasMessage); k++)
                jcl->message[currentPosition++] = Constants::sensorAliasMessage[k];
            for (unsigned int k = 0; k < strlen(jcl->getSensors()[i]->getPin()); k++)
                jcl->message[currentPosition++] = (char) jcl->getSensors()[i]->getPin()[k];
            jcl->message[currentPosition++] = 18;
            jcl->message[currentPosition++] = strlen(jcl->getSensors()[i]->getSensorNickname());
            for (unsigned int k = 0; k < strlen(jcl->getSensors()[i]->getSensorNickname()); k++)
                if (jcl->getSensors()[i]->getSensorNickname()[k] != 0)
                    jcl->message[currentPosition++] = jcl->getSensors()[i]->getSensorNickname()[k];
        }
    }

    /* Setting the value of the SENSOR_SIZE */
    for (i = 0; i < JCL::getTotalSensors(); i++) {
        if (jcl->getSensors()[i] != NULL) {
            jcl->message[currentPosition++] = 10;
            jcl->message[currentPosition++] =
                    strlen(Constants::sensorSizeMessage) + strlen(jcl->getSensors()[i]->getPin()) +
                    strlen(jcl->getSensors()[i]->getSensorSize()) + 4;
            jcl->message[currentPosition++] = 10;
            jcl->message[currentPosition++] =
                    strlen(Constants::sensorSizeMessage) + strlen(jcl->getSensors()[i]->getPin());
            for (unsigned int k = 0; k < strlen(Constants::sensorSizeMessage); k++)
                jcl->message[currentPosition++] = Constants::sensorSizeMessage[k];
            for (unsigned int k = 0; k < strlen(jcl->getSensors()[i]->getPin()); k++)
                jcl->message[currentPosition++] = (char) jcl->getSensors()[i]->getPin()[k];
            jcl->message[currentPosition++] = 18;
            jcl->message[currentPosition++] = strlen(jcl->getSensors()[i]->getSensorSize());
            for (unsigned int k = 0; k < strlen(jcl->getSensors()[i]->getSensorSize()); k++)
                if (jcl->getSensors()[i]->getSensorSize()[k] != 0)
                    jcl->message[currentPosition++] = jcl->getSensors()[i]->getSensorSize()[k];
        }
    }

    /* Setting the value of the SENSOR_DIR */
    for (i = 0; i < JCL::getTotalSensors(); i++) {
        if (jcl->getSensors()[i] != NULL) {
            jcl->message[currentPosition++] = 10;
            jcl->message[currentPosition++] =
                    strlen(Constants::sensorDirMessage) + strlen(jcl->getSensors()[i]->getPin()) + 1 + 4;
            jcl->message[currentPosition++] = 10;
            jcl->message[currentPosition++] =
                    strlen(Constants::sensorDirMessage) + strlen(jcl->getSensors()[i]->getPin());
            for (unsigned int k = 0; k < strlen(Constants::sensorDirMessage); k++)
                jcl->message[currentPosition++] = Constants::sensorDirMessage[k];
            for (unsigned int k = 0; k < strlen(jcl->getSensors()[i]->getPin()); k++)
                jcl->message[currentPosition++] = (char) jcl->getSensors()[i]->getPin()[k];
            jcl->message[currentPosition++] = 18;
            jcl->message[currentPosition++] = 1;
            jcl->message[currentPosition++] = jcl->getSensors()[i]->getTypeIO();
        }
    }

    /* Setting the value of the SENSOR_SAMPLING */
    for (i = 0; i < JCL::getTotalSensors(); i++) {
        if (jcl->getSensors()[i] != NULL) {
            jcl->message[currentPosition++] = 10;
            jcl->message[currentPosition++] =
                    strlen(Constants::sensorSamplingMessage) + strlen(jcl->getSensors()[i]->getPin()) +
                    strlen(jcl->getSensors()[i]->getDelay()) + 4;
            jcl->message[currentPosition++] = 10;
            jcl->message[currentPosition++] =
                    strlen(Constants::sensorSamplingMessage) + strlen(jcl->getSensors()[i]->getPin());
            for (unsigned int k = 0; k < strlen(Constants::sensorSamplingMessage); k++)
                jcl->message[currentPosition++] = Constants::sensorSamplingMessage[k];
            for (unsigned int k = 0; k < strlen(jcl->getSensors()[i]->getPin()); k++)
                jcl->message[currentPosition++] = (char) jcl->getSensors()[i]->getPin()[k];
            jcl->message[currentPosition++] = 18;
            jcl->message[currentPosition++] = strlen(jcl->getSensors()[i]->getDelay());
            for (unsigned int k = 0; k < strlen(jcl->getSensors()[i]->getDelay()); k++)
                if (jcl->getSensors()[i]->getDelay()[k] != 0)
                    jcl->message[currentPosition++] = jcl->getSensors()[i]->getDelay()[k];
        }
    }
//  }
    boolean activateEncryption = false;
    if (jcl->isEncryption() && messageType == -1) {
        jcl->setEncryption(false);
        activateEncryption = true;
    }
    size = completeHeader(currentPosition, 16, true, jcl->getMetadata()->getMAC(), 0);

    if (activateEncryption && messageType == -1)
        jcl->setEncryption(true);

    jcl->getClient().write(jcl->message, size);
    jcl->getClient().flush();
//Serial.write(jcl->message, size);
    if (messageType == -1)
        receiveRegisterServerAnswer();
    else
        receiveServerAnswer();
}