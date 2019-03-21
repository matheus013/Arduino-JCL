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
    //strlen(Constants::deviceTypeMessage) = 11, strlen(Constants::deviceTypeValue) = 1
    m_jcl->message[currentPosition++] = 16;
    m_jcl->message[currentPosition++] = 10;
    m_jcl->message[currentPosition++] = 11;
    for (unsigned int i = 0; i < 11; i++)
        m_jcl->message[currentPosition++] = Constants::deviceTypeMessage[i];
    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = 1;
    //for (unsigned int i = 0; i < 1; i++)
    m_jcl->message[currentPosition++] = Constants::deviceTypeValue[0];

    /* Setting the value of the PORT */
    m_jcl->message[currentPosition++] = 10;
    //strlen(Constants::portMessage) = 4
    int hostPortLength = strlen(m_jcl->get_metadata()->get_hostPort());
    m_jcl->message[currentPosition++] = hostPortLength + 8;
    m_jcl->message[currentPosition++] = 10;
    m_jcl->message[currentPosition++] = 4;
    for (unsigned int i = 0; i < 4; i++)
        m_jcl->message[currentPosition++] = Constants::portMessage[i];
    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = hostPortLength;
    for (unsigned int i = 0; i < hostPortLength; i++)
        m_jcl->message[currentPosition++] = m_jcl->get_metadata()->get_hostPort()[i];

    if (messageType == -1) {
        /* Setting the value of the IP */
        m_jcl->message[currentPosition++] = 10;
        //strlen(Constants::ipMessage) = 2
        int hostIpLength = strlen(m_jcl->get_metadata()->get_hostIP());
        m_jcl->message[currentPosition++] = hostIpLength + 6;
        m_jcl->message[currentPosition++] = 10;
        m_jcl->message[currentPosition++] = 2;
        for (unsigned int i = 0; i < 2; i++)
            m_jcl->message[currentPosition++] = Constants::ipMessage[i];
        m_jcl->message[currentPosition++] = 18;
        m_jcl->message[currentPosition++] = hostIpLength;
        for (unsigned int i = 0; i < hostIpLength; i++)
            m_jcl->message[currentPosition++] = m_jcl->get_metadata()->get_hostIP()[i];
    }

    /* Setting the value of the CORE(S) */
    m_jcl->message[currentPosition++] = 10;
    //strlen(Constants::coreMessage) = 7, strlen(Constants::coreValue) = 1
    m_jcl->message[currentPosition++] = 12;
    m_jcl->message[currentPosition++] = 10;
    m_jcl->message[currentPosition++] = 7;
    for (unsigned int i = 0; i < 7; i++)
        m_jcl->message[currentPosition++] = Constants::coreMessage[i];
    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = 1;
//    for (unsigned int i = 0; i < strlen(Constants::coreValue); i++)
    m_jcl->message[currentPosition++] = Constants::coreValue[0];

    /* Setting the value of the MAC */
    m_jcl->message[currentPosition++] = 10;
    //strlen(Constants::macMessage) = 3
    int macLength = strlen(m_jcl->get_metadata()->get_mac());
    m_jcl->message[currentPosition++] = macLength + 7;
    m_jcl->message[currentPosition++] = 10;
    m_jcl->message[currentPosition++] = 3;
    for (unsigned int i = 0; i < 3; i++)
        m_jcl->message[currentPosition++] = Constants::macMessage[i];
    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = macLength;
    for (unsigned int i = 0; i < macLength; i++)
        m_jcl->message[currentPosition++] = m_jcl->get_metadata()->get_mac()[i];

    /* Setting the value of the Device Platform */
    m_jcl->message[currentPosition++] = 10;
    //strlen(Constants::devicePlatformMessage) = 15,strlen(Constants::devicePlatformValue) = 12
    m_jcl->message[currentPosition++] = 21;
    m_jcl->message[currentPosition++] = 10;
    m_jcl->message[currentPosition++] = 15;
    for (unsigned int i = 0; i < 15; i++)
        m_jcl->message[currentPosition++] = Constants::devicePlatformMessage[i];
    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = 12;
    for (unsigned int i = 0; i < 12; i++)
        m_jcl->message[currentPosition++] = Constants::devicePlatformValue[i];

    /* Setting the value of the NUMBER_SENSORS */
    m_jcl->message[currentPosition++] = 10;
    //strlen(Constants::numberSensorsMessage) = 14
    int numberConfiguredSensorsLength = strlen(m_jcl->get_metadata()->get_numConfiguredSensors());
    m_jcl->message[currentPosition++] = 18 + numberConfiguredSensorsLength;
    m_jcl->message[currentPosition++] = 10;
    m_jcl->message[currentPosition++] = 14;
    for (unsigned int k = 0; k < 14; k++)
        m_jcl->message[currentPosition++] = Constants::numberSensorsMessage[k];
    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = numberConfiguredSensorsLength;
    for (unsigned int k = 0; k < numberConfiguredSensorsLength; k++)
        m_jcl->message[currentPosition++] = m_jcl->get_metadata()->get_numConfiguredSensors()[k];

    /* Setting the value of the STANDBY */
    m_jcl->message[currentPosition++] = 10;
    //strlen(Constants::standByMessage) = 7
    m_jcl->message[currentPosition++] = standBySize + 11;
    m_jcl->message[currentPosition++] = 10;
    m_jcl->message[currentPosition++] = 7;
    for (unsigned int k = 0; k < 7; k++)
        m_jcl->message[currentPosition++] = Constants::standByMessage[k];
    m_jcl->message[currentPosition++] = 18;
    if (m_jcl->get_metadata()->is_standBy()) {
        //strlen(Constants::trueMessage) = 4
        m_jcl->message[currentPosition++] = 4;
        for (unsigned int k = 0; k < 4; k++)
            m_jcl->message[currentPosition++] = Constants::trueMessage[k];
    } else {
        //strlen(Constants::falseMessage) = 5
        m_jcl->message[currentPosition++] = 5;
        for (unsigned int k = 0; k < 5; k++)
            m_jcl->message[currentPosition++] = Constants::falseMessage[k];
    }

//  if (atoi(jcl->getMetadata()->getNumConfiguredSensors()) != 0){

    /* Setting the value of the ENABLE_SENSOR */
    m_jcl->message[currentPosition++] = 10;
    //strlen(Constants::enableSensorMessage) = 13
    m_jcl->message[currentPosition++] = 17 + enableSensorSize;
    m_jcl->message[currentPosition++] = 10;
    m_jcl->message[currentPosition++] = 13;
    for (unsigned int k = 0; k < 13; k++)
        m_jcl->message[currentPosition++] = Constants::enableSensorMessage[k];
    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = enableSensorSize;

    for (int k = 0; k < JCL::get_totalSensors(); k++) {
        if (m_jcl->get_sensors()[k] != NULL) {
            for (unsigned i = 0; i < strlen(m_jcl->get_sensors()[k]->get_pin()); i++)
                m_jcl->message[currentPosition++] = m_jcl->get_sensors()[k]->get_pin()[i];
            m_jcl->message[currentPosition++] = ';';
        }
    }
    m_jcl->message[currentPosition++] = ';';

    /* Setting the value of the SENSOR_ALIAS */
    for (i = 0; i < JCL::get_totalSensors(); i++) {
        if (m_jcl->get_sensors()[i] != NULL) {
            m_jcl->message[currentPosition++] = 10;
            //strlen(Constants::sensorAliasMessage) = 13
            int sensorPinLength = strlen(m_jcl->get_sensors()[i]->get_pin());
            int sensorNicknameLength = strlen(m_jcl->get_sensors()[i]->get_sensorNickname());
            m_jcl->message[currentPosition++] = 17 + sensorPinLength + sensorNicknameLength;
            m_jcl->message[currentPosition++] = 10;
            m_jcl->message[currentPosition++] = 13 + sensorPinLength;
            for (unsigned int k = 0; k < 13; k++)
                m_jcl->message[currentPosition++] = Constants::sensorAliasMessage[k];
            for (unsigned int k = 0; k < sensorPinLength; k++)
                m_jcl->message[currentPosition++] = (char) m_jcl->get_sensors()[i]->get_pin()[k];
            m_jcl->message[currentPosition++] = 18;
            m_jcl->message[currentPosition++] = sensorNicknameLength;
            for (unsigned int k = 0; k < sensorNicknameLength; k++)
                if (m_jcl->get_sensors()[i]->get_sensorNickname()[k] != 0)
                    m_jcl->message[currentPosition++] = m_jcl->get_sensors()[i]->get_sensorNickname()[k];
        }
    }

    /* Setting the value of the SENSOR_SIZE */
    for (i = 0; i < JCL::get_totalSensors(); i++) {
        if (m_jcl->get_sensors()[i] != NULL) {
            m_jcl->message[currentPosition++] = 10;
            //strlen(Constants::sensorSizeMessage) = 12
            int sensorPinLength = strlen(m_jcl->get_sensors()[i]->get_pin());
            m_jcl->message[currentPosition++] =
                    16 + sensorPinLength +
                    strlen(m_jcl->get_sensors()[i]->get_sensorSize());
            m_jcl->message[currentPosition++] = 10;
            m_jcl->message[currentPosition++] = 12 + sensorPinLength;
            for (unsigned int k = 0; k < 12; k++)
                m_jcl->message[currentPosition++] = Constants::sensorSizeMessage[k];
            for (unsigned int k = 0; k < sensorPinLength; k++)
                m_jcl->message[currentPosition++] = (char) m_jcl->get_sensors()[i]->get_pin()[k];
            m_jcl->message[currentPosition++] = 18;
            int sensorSizeLength = strlen(m_jcl->get_sensors()[i]->get_sensorSize());
            m_jcl->message[currentPosition++] = sensorSizeLength;
            for (unsigned int k = 0; k < sensorSizeLength; k++)
                if (m_jcl->get_sensors()[i]->get_sensorSize()[k] != 0)
                    m_jcl->message[currentPosition++] = m_jcl->get_sensors()[i]->get_sensorSize()[k];
        }
    }

    /* Setting the value of the SENSOR_DIR */
    for (i = 0; i < JCL::get_totalSensors(); i++) {
        if (m_jcl->get_sensors()[i] != NULL) {
            m_jcl->message[currentPosition++] = 10;
            //strlen(Constants::sensorDirMessage) = 11
            int sensorPinLength = strlen(m_jcl->get_sensors()[i]->get_pin());
            m_jcl->message[currentPosition++] = 16 + sensorPinLength;
            m_jcl->message[currentPosition++] = 10;
            m_jcl->message[currentPosition++] = 11 + sensorPinLength;
            for (unsigned int k = 0; k < 11; k++)
                m_jcl->message[currentPosition++] = Constants::sensorDirMessage[k];
            for (unsigned int k = 0; k < sensorPinLength; k++)
                m_jcl->message[currentPosition++] = (char) m_jcl->get_sensors()[i]->get_pin()[k];
            m_jcl->message[currentPosition++] = 18;
            m_jcl->message[currentPosition++] = 1;
            m_jcl->message[currentPosition++] = m_jcl->get_sensors()[i]->get_typeIO();
        }
    }

    /* Setting the value of the SENSOR_SAMPLING */
    for (i = 0; i < JCL::get_totalSensors(); i++) {
        if (m_jcl->get_sensors()[i] != NULL) {
            m_jcl->message[currentPosition++] = 10;
            //strlen(Constants::sensorSamplingMessage) = 16
            int sensorPinLength = strlen(m_jcl->get_sensors()[i]->get_pin());
            int sensorDelayLength = strlen(m_jcl->get_sensors()[i]->get_delay());
            m_jcl->message[currentPosition++] = 20 + sensorPinLength + sensorDelayLength;
            m_jcl->message[currentPosition++] = 10;
            m_jcl->message[currentPosition++] = 16 + sensorPinLength;
            for (unsigned int k = 0; k < 16; k++)
                m_jcl->message[currentPosition++] = Constants::sensorSamplingMessage[k];
            for (unsigned int k = 0; k < sensorPinLength; k++)
                m_jcl->message[currentPosition++] = (char) m_jcl->get_sensors()[i]->get_pin()[k];
            m_jcl->message[currentPosition++] = 18;
            m_jcl->message[currentPosition++] = sensorDelayLength;
            for (unsigned int k = 0; k < sensorDelayLength; k++)
                if (m_jcl->get_sensors()[i]->get_delay()[k] != 0)
                    m_jcl->message[currentPosition++] = m_jcl->get_sensors()[i]->get_delay()[k];
        }
    }
//  }
    bool activateEncryption = false;
    if (m_jcl->is_encryption() && messageType == -1) {
        m_jcl->set_encryption(false);
        activateEncryption = true;
    }
    size = completeHeader(currentPosition, 16, true, m_jcl->get_metadata()->get_mac(), 0);

    if (activateEncryption && messageType == -1)
        m_jcl->set_encryption(true);

    m_jcl->get_client().write(m_jcl->message, size);
    m_jcl->get_client().flush();
//Serial.write(m_jcl->message, size);
    if (messageType == -1)
        receiveRegisterServerAnswer();
    else
        receiveServerAnswer();
}