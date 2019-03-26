//
// Created by Matheus Inácio on 2019-03-19.
//

#include "m_jcl.h"
#include "message.h"
#include "sensor.h"
#include "crypt.h"
#include "constants.h"
#include <Regexp.h>
#include "context.h"
#include "utils.h"

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

int Message::completeHeader(int messageSize, int messageType, bool sendMAC, char *hostMAC, int superPeerPort) {
    int addBytes;
    char *regKey;
    Crypt c;
    char *iv;;
    if (m_jcl->is_encryption()) {
        iv = c.generateIV();
        messageSize = c.cryptMessage(messageSize, m_jcl, iv, Crypt::hash1);
        regKey = c.generateRegistrationKey(m_jcl->message, iv, messageSize, Crypt::hash2);
    }

    if (sendMAC && m_jcl->is_encryption())
        addBytes = 61;
    else if (!sendMAC && m_jcl->is_encryption())
        addBytes = 53;
    else if (sendMAC && !m_jcl->is_encryption())
        addBytes = 13;
    else
        addBytes = 5;

    for (int k = messageSize - 1; k >= 0; k--)
        m_jcl->message[k + addBytes] = m_jcl->message[k];

    uint8_t currentPosition = 4;
    if (m_jcl->is_encryption())
        m_jcl->message[currentPosition++] = messageType + 64;
    else
        m_jcl->message[currentPosition++] = messageType;

    if (sendMAC) {
        if (superPeerPort == 0) {
            for (int i = 0; i < 8; i++)
                m_jcl->message[currentPosition++] = 0;
        } else {
            char *a = (char *) &superPeerPort;
            m_jcl->message[currentPosition++] = a[1];
            m_jcl->message[currentPosition++] = a[0];

            unsigned char mac[6];
            sscanf(hostMAC, "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
            for (int i = 0; i < 6; i++)
                m_jcl->message[currentPosition++] = mac[i];
        }
    }

    if (m_jcl->is_encryption()) {
        for (uint8_t it = 0; it < 16; it++)
            m_jcl->message[currentPosition++] = iv[it];
        for (uint8_t it = 0; it < 32; it++)
            m_jcl->message[currentPosition++] = regKey[it];
    }

    int totalSize = messageSize + addBytes - 4;
    char *value = (char *) &totalSize;

    if (totalSize <= 255) {  // If the size is less then 256, we only need one byte to store the size
        m_jcl->message[0] = 0;
        m_jcl->message[1] = 0;
        m_jcl->message[2] = 0;
        m_jcl->message[3] = value[0];
    } else {       // Otherwise we use two bytes to store the size (up to 32000 bytes)
        m_jcl->message[0] = 0;
        m_jcl->message[1] = 0;
        m_jcl->message[2] = value[1];
        m_jcl->message[3] = value[0];
    }
    return messageSize + addBytes;
}

void Message::receiveRegisterServerAnswer() {
    char *key;
    if (m_jcl->get_client()) {
        int16_t pos = 0;
        while (!m_jcl->get_client().available());
        while (m_jcl->get_client().available())
            m_jcl->message[pos++] = (char) m_jcl->get_client().read();

        key = (char *) malloc(sizeof(char) * 18);
        for (uint16_t x = 0; x < 17; x++)
            key[x] = m_jcl->message[pos - 17 + x];
        key[17] = '\0';
    }
    m_jcl->set_key(key);
    Crypt::update(m_jcl);
}

void Message::receiveServerAnswer() {
    while (!m_jcl->get_client().available());
    while (m_jcl->get_client().available())
        m_jcl->get_client().read();
}

void Message::sendResultBool(bool result) {
    int size;
    int currentPosition = 0;

    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = 4;
    m_jcl->message[currentPosition++] = 8;
    m_jcl->message[currentPosition++] = 1;
    m_jcl->message[currentPosition++] = 16;
    if (result)
        m_jcl->message[currentPosition++] = 1;
    else
        m_jcl->message[currentPosition++] = 0;

    size = completeHeader(currentPosition, 11, false, m_jcl->get_metadata()->get_mac(), 0);

    m_jcl->get_requestListener().write(m_jcl->message, size);
    m_jcl->get_requestListener().flush();
}

void Message::sensing(int pin, bool sensorNow) {
//Serial.print("sensing: "); Serial.println(pin);
    int size;
    unsigned int i;
    int currentPosition = 0;
    if (m_jcl->get_sensors()[pin] == NULL || m_jcl->get_sensors()[pin]->get_typeIO() == Constants::CHAR_OUTPUT)
        return;

    m_jcl->message[currentPosition++] = 8; // Indicates first field
    m_jcl->message[currentPosition++] = 27;  // Fix field. Also indicates sensing data

    m_jcl->message[currentPosition++] = 18; // Indicates second field. In this case is a string with MAC and PORT
    int metadataHostPortLength = strlen(m_jcl->get_metadata()->get_hostPort());
    int metadataMacLength = strlen(m_jcl->get_metadata()->get_mac());
    m_jcl->message[currentPosition++] = metadataHostPortLength + metadataMacLength; // Indicates the size of the string

    for (i = 0; i < metadataMacLength; i++)
        m_jcl->message[currentPosition++] = m_jcl->get_metadata()->get_mac()[i];

    for (i = 0; i < metadataHostPortLength; i++)
        m_jcl->message[currentPosition++] = m_jcl->get_metadata()->get_hostPort()[i];

    m_jcl->message[currentPosition++] = 24; // Indicates third field. In this case is the ID of the sensor
    m_jcl->message[currentPosition++] = pin;

    m_jcl->message[currentPosition++] = 34; // Indicates fourth field. In this case the value of the sensing

    uint8_t array[10];
    int sensorValue;
    if (pin >= JCL::get_totalDigitalSensors()) {
        sensorValue = analogRead(pin - JCL::get_totalDigitalSensors());
    } else
        sensorValue = digitalRead(pin);

    int totalBytes = encode_unsigned_varint(array, sensorValue);
    m_jcl->message[currentPosition++] = totalBytes + 1;
    m_jcl->message[currentPosition++] = 40;
    for (int i = 0; i < totalBytes; i++)
        m_jcl->message[currentPosition++] = (int) array[i];

    m_jcl->message[currentPosition++] = 40; // Indicates fifth field. In this case the time in long
    uint8_t arrayInt[10];
    int total = encode_signed_varint(arrayInt, millis());
    for (int i = 0; i < total; i++)
        m_jcl->message[currentPosition++] = arrayInt[i];

    m_jcl->message[currentPosition++] = 50; // Indicates sixth field. In this case the data type in a String format
    //strlen(Constants::dataTypeValue) = 3
    m_jcl->message[currentPosition++] = 3;     // Indicates the size of the String
    for (i = 0; i < 3; i++)
        m_jcl->message[currentPosition++] = Constants::dataTypeValue[i];

    if (sensorNow) {
        size = completeHeader(currentPosition, 15, false, m_jcl->get_metadata()->get_mac(), 0);
        m_jcl->get_requestListener().write(m_jcl->message, size);
        m_jcl->get_requestListener().flush();
    } else {
        size = completeHeader(currentPosition, 15, true, m_jcl->get_metadata()->get_mac(), 0);

        m_jcl->get_sensors()[pin]->count++;
        m_jcl->get_client().write(m_jcl->message, size);
        m_jcl->get_client().flush();
        m_jcl->get_sensors()[pin]->set_lastExecuted(millis());
    }
    m_jcl->get_sensors()[pin]->set_value(sensorValue);
    if (!sensorNow)
        receiveServerAnswer();
}

int Message::encode_unsigned_varint(uint8_t *const buffer, uint64_t value) {
    int encoded = 0;
    do {
        uint8_t next_byte = value & 0x7F;
        value >>= 7;
        if (value)
            next_byte |= 0x80;
        buffer[encoded++] = next_byte;
    } while (value);
    return encoded;
}

int Message::encode_signed_varint(uint8_t *const buffer, int64_t value) {
    uint64_t uvalue;
    uvalue = uint64_t(value < 0 ? ~(value << 1) : (value << 1));
    return encode_unsigned_varint(buffer, uvalue);
}

bool Message::setMetadata() {
    char *pointer;
    char *pin;
    char value[4];
    int pos = 0;
    Sensor *s;

    for (int i = 0; i < JCL::get_totalSensors(); i++)
        if (m_jcl->get_sensors()[i] != NULL) {
            m_jcl->get_sensors()[i]->deleteSensor();
            m_jcl->get_sensors()[i] = NULL;
        }

    m_jcl->message[0] = 52;
    m_jcl->message[1] = 52;
    for (int i = 0; i < 17; i++)
        m_jcl->message[i] = 52;

    pointer = strstr(m_jcl->message, "DEVICE_ID");
    if (pointer != NULL) {
        int c = 0;
        while (pointer[c] != 18)
            c++;
        c++;
        int l = pointer[c];
        c++;
        char *nameBoard = (char *) malloc(sizeof(char) * l + 1);
        for (int i = 0; i < l; i++)
            nameBoard[i] = pointer[c++];
        nameBoard[l] = '\0';
        m_jcl->get_metadata()->set_boardName(nameBoard);
    }

    pointer = strstr(m_jcl->message, "ENABLE_SENSOR");
    if (pointer != NULL) {
        int c = 0;
        while (pointer[c] != 18)
            c++;
        c++;
        int l = pointer[c];
        c++;
        pos = 0;
        uint8_t numSensors = 0;

        pin = (char *) malloc(sizeof(char) * 4);

        for (int i = 0; i <= l; i++) {
            if (pointer[c] == ';' || i == l) {
                //pin[pos] = '\0';
                if (!Sensor::validPin(atoi(pin)))
                    return false;
                s = new Sensor();
                char *pinN = (char *) malloc(sizeof(char) * 4);
                strcpy(pinN, pin);
                int pinAux = atoi(pinN);
                m_jcl->get_sensors()[pinAux] = s;
                //m_jcl->getSensors()[atoi(pin)]->setPin(pin);
                s->set_pin(pinN);

                m_jcl->get_sensors()[pinAux]->set_lastExecuted(0);
                char defaultName[] = "not set",
                        defaultDelay[] = "60",
                        defaultSensorSize[] = "1";
                m_jcl->get_sensors()[pinAux]->set_sensorNickname(
                        defaultName);   // In case the user didn't set the sensor name we use the default
                m_jcl->get_sensors()[pinAux]->set_delay(
                        defaultDelay);   // In case the user didn't set the delay we use the default 60
                m_jcl->get_sensors()[pinAux]->set_typeIO(Constants::CHAR_INPUT);   // the default is input sensor
                m_jcl->get_sensors()[pinAux]->set_type(0);   // default is generic sensor
                m_jcl->get_sensors()[pinAux]->set_sensorSize(defaultSensorSize);   // default is 1MB
                pos = 0;
                c++;
                numSensors++;
            } else {
                pin[pos++] = pointer[c++];
                pin[pos] = '\0';
            }
        }
        char nSensors[4];
        sprintf(nSensors, "%d", numSensors);
        m_jcl->get_metadata()->set_numConfiguredSensors(nSensors);
    }

    pointer = m_jcl->message;

    while ((pointer = strstr(pointer, "SENSOR_SAMPLING_")) != NULL) {
        int c = 16;
        pos = 0;
        while (pointer[c] != 18) {
            pin[pos++] = pointer[c++];
        }
        pin[pos] = '\0';

        /* Verificar se é para criar mesmo quando não estiver no ENABLE_SENSOR ou se é para retornar false*/
        s = m_jcl->get_sensors()[atoi(pin)];
        if (s == NULL) {
            return false;
        }

        c++;
        int l = pointer[c++];
        char *delay = (char *) malloc(sizeof(char) * l + 1);
        pos = 0;
        for (int i = 0; i < l; i++) {
            delay[pos++] = pointer[c++];
        }
        delay[pos] = '\0';

        s->set_delay(delay);
        pointer++;
    }

    pointer = m_jcl->message;
    while ((pointer = strstr(pointer, "SENSOR_ALIAS_")) != NULL) {
        int c = 13;
        pos = 0;
        while (pointer[c] != 18) {
            pin[pos++] = pointer[c++];
        }
        pin[pos] = '\0';
        int pinAux = atoi(pin);
        if (!Sensor::validPin(pinAux) || m_jcl->get_sensors()[pinAux] == NULL) {
            return false;
        }
        s = m_jcl->get_sensors()[pinAux];
        c++;
        int l = pointer[c++];
        char *nameSensor = (char *) malloc(sizeof(char) * l + 1);
        pos = 0;
        for (int i = 0; i < l; i++) {
            nameSensor[pos++] = pointer[c++];
        }
        nameSensor[pos] = '\0';
        s->set_sensorNickname(nameSensor);
        pointer++;
    }

    pointer = m_jcl->message;
    while ((pointer = strstr(pointer, "SENSOR_SIZE_")) != NULL) {
        int c = 12;
        pos = 0;
        while (pointer[c] != 18) {
            pin[pos++] = pointer[c++];
        }
        pin[pos] = '\0';
        int pinAux = atoi(pin);
        if (!Sensor::validPin(pinAux) || m_jcl->get_sensors()[pinAux] == NULL) {
            return false;
        }
        s = m_jcl->get_sensors()[pinAux];
        c++;
        int l = pointer[c++];
        char *sensorSize = (char *) malloc(sizeof(char) * l + 1);
        pos = 0;
        for (int i = 0; i < l; i++) {
            sensorSize[pos++] = pointer[c++];
        }
        sensorSize[pos] = '\0';
        s->set_sensorSize(sensorSize);
        pointer++;
    }

    pointer = m_jcl->message;
    while ((pointer = strstr(pointer, "SENSOR_TYPE_")) != NULL) {
        int c = 12;
        pos = 0;
        while (pointer[c] != 18) {
            pin[pos++] = pointer[c++];
        }
        pin[pos] = '\0';
        int pinAux = atoi(pin);
        if (!Sensor::validPin(pinAux) || m_jcl->get_sensors()[pinAux] == NULL) {
            return false;
        }
        s = m_jcl->get_sensors()[pinAux];
        c++;
        int l = pointer[c++];

        pos = 0;
        for (int i = 0; i < l; i++) {
            value[pos++] = pointer[c++];
        }
        value[pos] = '\0';
        s->set_type(atoi(value));
        pointer++;
    }

    pointer = m_jcl->message;
    while ((pointer = strstr(pointer, "SENSOR_DIR_")) != NULL) {
        int c = 11;
        pos = 0;
        while (pointer[c] != 18) {
            pin[pos++] = pointer[c++];
        }
        pin[pos] = '\0';
        int pinAux = atoi(pin);
        if (!Sensor::validPin(pinAux) || m_jcl->get_sensors()[pinAux] == NULL) {
            return false;
        }
        s = m_jcl->get_sensors()[pinAux];
        c++;
        c++;

        pos = 0;
        s->set_typeIO(toUpperCase(pointer[c++]));

        if (s->get_typeIO() != Constants::CHAR_OUTPUT && s->get_typeIO() != Constants::CHAR_INPUT)
            s->set_typeIO(Constants::CHAR_INPUT);

        s->configurePinMode();
        pointer++;
    }

    m_jcl->listSensors();
    return true;
}

bool Message::registerContext(bool isMQTTContext) {
    int pos = 14;
    Context *ctx = new Context();
    while (m_jcl->message[pos] != 74)
        pos++;
    int nChars = m_jcl->message[++pos];
    char *expression = (char *) malloc(sizeof(char) * nChars + 1);
    for (uint8_t x = 0; x < nChars; x++)
        expression[x] = m_jcl->message[x + pos + 1];
    expression[nChars] = '\0';
    ctx->set_expression(expression);
    pos += nChars;

    while (m_jcl->message[pos] != 74)
        pos++;
    nChars = m_jcl->message[++pos];
    char pinC[nChars];
    for (uint8_t x = 0; x < nChars; x++)
        pinC[x] = m_jcl->message[x + pos + 1];
    pinC[nChars] = '\0';
    pos += nChars;
    int pin = atoi(pinC);

    if (m_jcl->get_sensors()[pin] == NULL) {
        ctx->deleteContext();
        sendResultBool(false);
    } else {
        while (m_jcl->message[pos] != 74)
            pos++;
        nChars = m_jcl->message[++pos];
        char *nickname = (char *) malloc(sizeof(char) * nChars + 1);
        for (uint8_t x = 0; x < nChars; x++)
            nickname[x] = m_jcl->message[x + pos + 1];
        nickname[nChars] = '\0';
        ctx->set_nickname(nickname);
        pos += nChars;

        char *token = strtok(ctx->get_expression(), ";");
        while (token != NULL) {
            MatchState ms;
            ms.Target(token);
            char res1 = ms.Match("^S[0-9]+[><=~]=?-?[0-9]+$");
            char res2 = ms.Match("^S[0-9]+[><=~]=?-?[0-9]+.[0-9]+$");
            if (res1 == REGEXP_MATCHED || res2 == REGEXP_MATCHED) {
                uint8_t current = 2, p = 0;;
                char *operators = (char *) malloc(sizeof(char) * OPERATORS_MAX_SIZE + 1);
                while (!((token[current] >= 48 && token[current] <= 57) || token[current] == 45))
                    operators[p++] = token[current++];
                operators[p] = '\0';
                ctx->get_operators()[ctx->get_numExpressions()] = operators;

                p = 0;
                char *threshold = (char *) malloc(sizeof(char) * OPERATORS_MAX_SIZE + 1);
                while (token[current] != '\0')
                    threshold[p++] = token[current++];
                threshold[p] = '\0';
                ctx->get_threshold()[ctx->get_numExpressions()] = threshold;
            } else
                return false;

            ctx->set_numExpressions(ctx->get_numExpressions() + 1);
            token = strtok(NULL, ";");

            ctx->set_mqttContext(isMQTTContext);
        }
        m_jcl->get_sensors()[pin]->getEnabledContexts()[m_jcl->get_sensors()[pin]->get_numContexts()] = ctx;
    }
    m_jcl->get_sensors()[pin]->set_numContexts(m_jcl->get_sensors()[pin]->get_numContexts() + 1);
    return true;
}

void Message::treatMessage() {
    int key = m_jcl->message[4] & 0x3F;
    int crypt = ((m_jcl->message[4] >> 6) & 0x03);
    int typePosition = 14;
    Crypt c;
    if (crypt == 1) {
        this->m_messageSize = c.decryptMessage(this->m_messageSize, m_jcl);
        if (m_messageSize == -1) {
            sendResultBool(false);
            return;
        }
        m_jcl->message[4] = key;
    }

    switch (m_jcl->message[typePosition]) {
        case 44: { // SensorNow
            Serial.println("SensorNow");
            int msgSize = m_jcl->message[3];
            int pos = msgSize - 1;
            while (m_jcl->message[pos] != 74)
                pos--;
            int nChars = m_jcl->message[++pos];
            char pinC[nChars + 1];
            pos++;
            for (uint8_t x = 0; x < nChars; x++)
                pinC[x] = m_jcl->message[x + pos];
            pinC[nChars] = '\0';
            sensing(atoi(pinC), true);
            break;
        }
        case 45: {   // Message TurnOn
            // printMessagePROGMEM(Constants::turnOnMessage);
            Serial.println("TurnOn");
            m_jcl->get_metadata()->set_standBy(false);

            sendMetadata(40);
            sendResult(101);
            break;
        }
        case 46: {  // Message StandBy
            // printMessagePROGMEM(Constants::standByMessageListen);
            Serial.println("StandBy");
            m_jcl->get_metadata()->set_standBy(true);
            sendMetadata(40);
            sendResult(102);
            break;
        }
        case 47: {   //SetMetadata
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::setMetadataMessage);
                Serial.println("SetMetadata");
                bool b = setMetadata();
                if (b) {
                    sendMetadata(40);
                    sendResultBool(true);
                    m_jcl->writeEprom();
                } else {
                    m_jcl->readEprom();
                    sendResultBool(false);
                }
            }
            break;
        }
        case 49: {  // Message SetSensor
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::setSensorMessage);
                // Serial.println("SetSensor");
                bool b = messageSetSensor();
                if (b) {
                    sendMetadata(40);
                    m_jcl->writeEprom();
                } else
                    m_jcl->readEprom();
                sendResultBool(b);
            }
            break;
        }
        case 50: { // RemoveSensor
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::removeSensorMessage);
                Serial.println("RemoveSensor");
                int msgSize = m_jcl->message[3];
                int pos = msgSize - 1;
                while (m_jcl->message[pos] != 74)
                    pos--;
                pos++;
                int nChars = m_jcl->message[pos];
                char pinC[nChars + 1];
                pos++;
                for (uint8_t x = 0; x < nChars; x++)
                    pinC[x] = m_jcl->message[x + pos];
                pinC[nChars] = '\0';

                Sensor *s = m_jcl->get_sensors()[atoi(pinC)];
                if (s == NULL)
                    sendResultBool(false);
                else {
                    s->deleteSensor();
                    char nSensors[4];
                    sprintf(nSensors, "%d", atoi(m_jcl->get_metadata()->get_numConfiguredSensors()) - 1);
                    m_jcl->get_metadata()->set_numConfiguredSensors(nSensors);
                    m_jcl->get_sensors()[atoi(pinC)] = NULL;
                    sendMetadata(40);
                    m_jcl->writeEprom();
                    sendResultBool(true);
                }
            }
            break;
        }
        case 51: {   // Acting
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::actingMessage);
                Serial.println("Acting");
                int msgSize = m_jcl->message[3];
                int pos = msgSize - 1;
                int value = 0;
                int pin = 0;
                int k, j;
                char command[12];
                pos = 0;
                while (m_jcl->message[pos] != 74)
                    pos++;
                k = m_jcl->message[++pos];

                if (k == 1)
                    pin = m_jcl->message[pos + 1] - 48;
                else {
                    pin = 10 * (m_jcl->message[pos + 1] - 48);
                    pin += m_jcl->message[pos + 2] - 48;
                }

                if (!Sensor::validPin(pin))
                    sendResultBool(false);
                if (m_jcl->get_sensors()[pin] == NULL ||
                    m_jcl->get_sensors()[pin]->getTypeIO() == Constants::CHAR_INPUT)
                    sendResultBool(false);

                while (m_jcl->message[pos] != 16)
                    pos++;
                pos++;
                pos++;
                uint8_t numCmd = (int) m_jcl->message[pos];
                pos++;
                pos++;
                bool b = true;
                Sensor *s = m_jcl->get_sensors()[pin];
                for (k = 0; k < numCmd; k++) {
                    uint8_t nChars = m_jcl->message[pos++];
                    for (j = 0; j < nChars && j < 12; j++)
                        command[j] = m_jcl->message[pos++];
                    command[j] = '\0';
                    pos++;
                    float f = atof(command);
                    value = (int) f;
                    if (!s->acting(value)) {
                        b = false;
                        break;
                    }
                }
                sendResultBool(b);
            }
            break;
        }
        case 52: {   // Restart
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResult(101);
            } else {
                sendResult(100);
                unregister();
                resetFunc1();
            }
            break;
        }
        case 53: {   // setEncryption
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::setEncryptionMessage);
                Serial.println("setEncryption");
                if (m_jcl->message[21] == 'f')
                    m_jcl->set_encryption(false);
                else
                    m_jcl->set_encryption(true);
                sendResultBool(true);
            }
            break;
        }
        case 54: {   // RegisterContext
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::registerContextMessage);
                Serial.println("RegisterContext");
                bool b = registerContext(false);
                if (b)
                    m_jcl->writeEprom();
                sendResultBool(b);
                m_jcl->listSensors();
            }
            break;
        }
        case 55: {   //  AddContextAction
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::addContextActionMessage);
                Serial.println("AddContextAction1");
                uint16_t pos = 17;
                while (m_jcl->message[pos] != 74)
                    pos++;
                pos++;
                uint8_t nChars = m_jcl->message[pos++];
                char contextName[nChars + 1];
                for (uint8_t x = 0; x < nChars; x++)
                    contextName[x] = m_jcl->message[pos + x];
                contextName[nChars] = '\0';

                Context *ctx = NULL;
                for (uint16_t x = 0; x < TOTAL_SENSORS; x++) {
                    Sensor *s = m_jcl->get_sensors()[x];
                    if (s->get_numContexts() > 0) {
                        for (uint8_t ctxNumber = 0; ctxNumber < s->get_numContexts(); ctxNumber++)
                            if (strcmp(s->getEnabledContexts()[ctxNumber]->get_nickname(), contextName) == 0) {
                                ctx = s->getEnabledContexts()[ctxNumber];
                                break;
                            }
                    }
                }

                if (ctx == NULL)
                    sendResultBool(false);
                else {
                    Action *act = new Action();
                    pos += nChars;
                    while (m_jcl->message[pos] != 74)
                        pos++;
                    pos++;
                    nChars = m_jcl->message[pos++];
                    char *hostIP = (char *) malloc(sizeof(char) * nChars + 1);
                    for (uint8_t x = 0; x < nChars; x++)
                        hostIP[x] = m_jcl->message[pos + x];
                    hostIP[nChars] = '\0';
                    act->set_hostIP(hostIP);

                    pos += nChars;
                    char *hostPort = new char[nChars + 1];
                    while (m_jcl->message[pos] != 74)
                        pos++;
                    pos++;
                    nChars = m_jcl->message[pos++];
                    for (uint8_t x = 0; x < nChars; x++)
                        hostPort[x] = m_jcl->message[pos + x];
                    hostPort[nChars] = '\0';
                    act->set_hostPort(hostPort);

                    pos += nChars;
                    while (m_jcl->message[pos] != 74)
                        pos++;
                    pos++;
                    nChars = m_jcl->message[pos++];
                    char *hostMAC = new char[nChars + 1];
                    for (uint8_t x = 0; x < nChars; x++)
                        hostMAC[x] = m_jcl->message[pos + x];
                    hostMAC[nChars] = '\0';
                    act->set_hostMac(hostMAC);

                    pos += nChars;
                    while (m_jcl->message[pos] != 74)
                        pos++;
                    pos++;
                    nChars = m_jcl->message[pos++];
                    char *superPeerPort = new char[nChars + 1];
                    for (uint8_t x = 0; x < nChars; x++)
                        superPeerPort[x] = m_jcl->message[pos + x];
                    superPeerPort[nChars] = '\0';
                    act->set_superPeerPort(superPeerPort);

                    pos += nChars;
                    while (m_jcl->message[pos] != 74)
                        pos++;
                    pos++;
                    nChars = m_jcl->message[pos++];
                    char *ticket = new char[nChars + 1];
                    for (uint8_t x = 0; x < nChars; x++)
                        ticket[x] = m_jcl->message[pos + x];
                    ticket[nChars] = '\0';
                    act->set_ticket(ticket);

                    pos += nChars;
                    while (m_jcl->message[pos] != 74)
                        pos++;
                    pos++;
                    nChars = m_jcl->message[pos++];
                    if (m_jcl->message[pos] == 't')
                        act->set_useSensorValue(true);
                    else
                        act->set_useSensorValue(false);

                    pos += nChars;
                    while (m_jcl->message[pos] != 74)
                        pos++;
                    pos++;
                    nChars = m_jcl->message[pos++];
                    char *className = new char[nChars + 1];
                    for (uint8_t x = 0; x < nChars; x++)
                        className[x] = m_jcl->message[pos + x];
                    className[nChars] = '\0';
                    act->set_className(className);
                    act->set_classNameSize(nChars);

                    pos += nChars;
                    while (m_jcl->message[pos] != 74)
                        pos++;
                    nChars = m_jcl->message[++pos];
                    char *methodName = new char[nChars + 1];
                    for (uint8_t x = 0; x < nChars; x++)
                        methodName[x] = m_jcl->message[pos + x + 1];
                    methodName[nChars] = '\0';
                    act->set_methodName(methodName);
                    act->set_methodNameSize(nChars);

                    while (m_jcl->message[pos] != 122)
                        pos++;
                    pos--;
                    char *param = new char[m_messageSize - pos + 1];
                    for (int x = pos; x < m_messageSize; x++)
                        param[x - pos] = m_jcl->message[x];
                    param[m_messageSize - pos] = '\0';
                    act->set_param(param);
                    act->set_paramSize(m_messageSize - pos);
                    act->set_acting(false);
                    ctx->getEnabledActions()[ctx->get_numActions()] = act;
                    ctx->set_numActions(ctx->get_numActions() + 1);
                    m_jcl->writeEprom();
                    sendResultBool(true);
                }
            }
            break;
        }
        case 56: {   // AddContextAction
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::addContextActionMessage);
                Serial.println("AddContextAction");
                /*uint8_t nChars=m_jcl->message[43];
                char contextName[nChars+1];
                for (uint8_t x=0; x < nChars; x++)
                contextName[x] = m_jcl->message[44 + x];
                contextName[nChars] = '\0';*/
                Context *ctx = NULL;
                uint16_t pos = 17;
                while (m_jcl->message[pos] != 74)
                    pos++;
                pos++;
                uint8_t nChars = m_jcl->message[pos++];
                char contextName[nChars + 1];
                for (uint8_t x = 0; x < nChars; x++)
                    contextName[x] = m_jcl->message[pos + x];
                contextName[nChars] = '\0';
                for (uint16_t x = 0; x < TOTAL_SENSORS; x++)
                    if (m_jcl->get_sensors()[x]->get_numContexts() > 0) {
                        for (uint8_t ctxNumber = 0; ctxNumber < m_jcl->get_sensors()[x]->get_numContexts(); ctxNumber++)
                            if (strcmp(m_jcl->get_sensors()[x]->getEnabledContexts()[ctxNumber]->get_nickname(),
                                       contextName) == 0) {
                                ctx = m_jcl->get_sensors()[x]->getEnabledContexts()[ctxNumber];
                                break;
                            }
                    }

                if (ctx == NULL)
                    sendResultBool(false);
                else {
                    Action *act = new Action();
                    uint16_t pos = 47 + nChars;
                    nChars = m_jcl->message[pos++];
                    char *hostIP = (char *) malloc(sizeof(char) * nChars + 1);
                    for (uint8_t x = 0; x < nChars; x++)
                        hostIP[x] = m_jcl->message[pos + x];
                    hostIP[nChars] = '\0';
                    act->set_hostIP(hostIP);

                    pos += nChars;
                    while (m_jcl->message[pos] != 74)
                        pos++;
                    nChars = m_jcl->message[++pos];
                    pos++;
                    char *hostPort = (char *) malloc(sizeof(char) * nChars + 1);
                    for (uint8_t x = 0; x < nChars; x++)
                        hostPort[x] = m_jcl->message[pos + x];
                    hostPort[nChars] = '\0';
                    act->set_hostPort(hostPort);

                    pos += nChars;
                    while (m_jcl->message[pos] != 74)
                        pos++;
                    nChars = m_jcl->message[++pos];
                    pos++;
                    char *portSuperPeer = (char *) malloc(sizeof(char) * nChars + 1);
                    for (uint8_t x = 0; x < nChars; x++)
                        portSuperPeer[x] = m_jcl->message[pos + x];
                    portSuperPeer[nChars] = '\0';
                    act->set_superPeerPort(portSuperPeer);

                    pos += nChars + 95;
                    while (m_jcl->message[pos] != 3)
                        pos++;
                    /*  pos++;
                      while (m_jcl->message[pos] != 3)
                        pos++;*/

                    nChars = m_messageSize - pos;
                    char *param = (char *) malloc(sizeof(char) * nChars + 1);
                    for (uint16_t x = 0; x < nChars; x++)
                        param[x] = m_jcl->message[pos + x];
                    param[nChars] = '\0';
                    act->set_param(param);
                    act->set_paramSize(nChars);
/*for (int jj=0; jj<nChars; jj++){
  Serial.print((int) act->getParam()[jj]);
  Serial.print("  ");
  Serial.print((char) act->getParam()[jj]);
  Serial.println();
}

Serial.println();*/
                    act->set_acting(true);
                    ctx->getEnabledActions()[ctx->get_numActions()] = act;
                    ctx->set_numActions(ctx->get_numActions() + 1);
                    m_jcl->writeEprom();
                    sendResultBool(true);
                }
            }
            break;
        }
        case 61: {
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::registerContextMessage);
                Serial.println("RegisterContext");
                bool b = registerContext(true);
                if (b)
                    m_jcl->writeEprom();
                sendResultBool(b);
                m_jcl->listSensors();
            }
            break;
        }
        case 62: {
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::registerContextMessage);
                Serial.println("UnregisterContext");
                bool b = unregisterContext(false);
                if (b)
                    m_jcl->writeEprom();
                sendResultBool(b);
                m_jcl->listSensors();
            }
            break;
        }
        case 63: {
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::registerContextMessage);
                Serial.println("UnregisterMQTTContext");
                bool b = unregisterContext(true);
                if (b)
                    m_jcl->writeEprom();
                sendResultBool(b);
                m_jcl->listSensors();
            }
            break;
        }
        case 64: {
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::registerContextMessage);
                Serial.println("remove Context action");
                bool b = removeContextAction(true);
                if (b)
                    m_jcl->writeEprom();
                sendResultBool(b);
                m_jcl->listSensors();
            }
            break;
        }
        case 65: {
            if (m_jcl->get_metadata()->is_standBy()) {
                sendResultBool(false);
            } else {
                // printMessagePROGMEM(Constants::registerContextMessage);
                Serial.println("remove Context action");
                bool b = removeContextAction(false);
                if (b)
                    m_jcl->writeEprom();
                sendResultBool(b);
                m_jcl->listSensors();
            }
            break;
        }
        default: {
            // printMessagePROGMEM(Constants::commandUnkwownMessage);
            Serial.println("Command unkwnown");
            Serial.println((int) m_jcl->message[typePosition]);
            break;
        }
    }
}

void Message::sendResult(int result) {
    int size;
    int currentPosition = 0;

    m_jcl->message[currentPosition++] = 8;
    m_jcl->message[currentPosition++] = result;

    size = completeHeader(currentPosition, 0, false, m_jcl->get_metadata()->get_mac(), 0);
    m_jcl->get_requestListener().write(m_jcl->message, size);
    m_jcl->get_requestListener().flush();
}


bool Message::messageSetSensor() {
    int j;
    char values[8];
    Sensor *s;
    s = new Sensor();
    int position = 20;

    int nChars = m_jcl->message[position++];
    char *nameSensor = (char *) malloc(sizeof(char) * nChars + 1);
    for (j = 0; j < nChars; j++) {
        nameSensor[j] = m_jcl->message[position++];
    }
    nameSensor[nChars] = '\0';
    s->set_sensorNickname(nameSensor);

    position++;
    nChars = m_jcl->message[position++];
    char *pin = (char *) malloc(sizeof(char) * nChars + 1);
    for (j = 0; j < nChars; j++) {
        pin[j] = m_jcl->message[position++];
    }
    pin[nChars] = '\0';
    s->set_pin(pin);

    if (!Sensor::validPin(atoi(s->get_pin()))) {
        s->deleteSensor();
        return false;
    }

    /* Sensor Size */
    position++;
    nChars = m_jcl->message[position++];
    char *sensorSize = (char *) malloc(sizeof(char) * nChars + 1);
    for (j = 0; j < nChars; j++)
        sensorSize[j] = m_jcl->message[position++];
    sensorSize[nChars] = '\0';
    s->set_sensorSize(sensorSize);

    position++;
    nChars = m_jcl->message[position++];
    char *delay = (char *) malloc(sizeof(char) * nChars + 1);
    for (j = 0; j < nChars; j++)
        delay[j] = m_jcl->message[position++];
    delay[nChars] = '\0';
    s->set_delay(delay);

    if (position != m_jcl->message[3]) {
        position++;
        nChars = m_jcl->message[position++];
        for (j = 0; j < nChars; j++)
            values[j] = m_jcl->message[position++];
        values[nChars] = '\0';

        s->set_typeIO(toUpperCase(values[0]));
        if (s->get_typeIO() != Constants::CHAR_OUTPUT && s->get_typeIO() != Constants::CHAR_INPUT)
            s->set_typeIO(Constants::CHAR_INPUT);
    } else
        s->set_typeIO(Constants::CHAR_INPUT);

    position++;
    nChars = m_jcl->message[position++];
    for (j = 0; j < nChars; j++)
        values[j] = m_jcl->message[position++];
    values[nChars] = '\0';
    s->set_type(atoi(values));

    s->set_lastExecuted(0);
    int pinInt = atoi(s->get_pin());
    if (m_jcl->get_sensors()[pinInt] != NULL) {
        m_jcl->get_sensors()[pinInt]->deleteSensor();
        m_jcl->get_sensors()[pinInt] = NULL;
    } else {
        char nSensors[4];
        sprintf(nSensors, "%d", atoi(m_jcl->get_metadata()->get_numConfiguredSensors()) + 1);
        m_jcl->get_metadata()->set_numConfiguredSensors(nSensors);
    }

    m_jcl->get_sensors()[atoi(s->get_pin())] = s;
    s->configurePinMode();
    m_jcl->listSensors();
//jcl->availableSensors[jcl->numSensors++] = atoi((s->get_pin()));
    return true;
}


void Message::sendContextActionMessage(Action *act) {
    int currentPosition = 0;
    if (act->is_acting()) {
        m_jcl->message[currentPosition++] = 8;
        m_jcl->message[currentPosition++] = 51;
        m_jcl->message[currentPosition++] = 18;
        m_jcl->message[currentPosition++] = 37;
        m_jcl->message[currentPosition++] = 122;
        m_jcl->message[currentPosition++] = 16;
        m_jcl->message[currentPosition++] = 106;
        m_jcl->message[currentPosition++] = 97;
        m_jcl->message[currentPosition++] = 118;
        m_jcl->message[currentPosition++] = 97;
        m_jcl->message[currentPosition++] = 46;
        m_jcl->message[currentPosition++] = 108;
        m_jcl->message[currentPosition++] = 97;
        m_jcl->message[currentPosition++] = 110;
        m_jcl->message[currentPosition++] = 103;
        m_jcl->message[currentPosition++] = 46;
        m_jcl->message[currentPosition++] = 79;
        m_jcl->message[currentPosition++] = 98;
        m_jcl->message[currentPosition++] = 106;
        m_jcl->message[currentPosition++] = 101;
        m_jcl->message[currentPosition++] = 99;
        m_jcl->message[currentPosition++] = 116;
        m_jcl->message[currentPosition++] = 24;
        m_jcl->message[currentPosition++] = 2;
        m_jcl->message[currentPosition++] = 16;
        m_jcl->message[currentPosition++] = 1;
        m_jcl->message[currentPosition++] = 10;

        for (uint8_t x = 0; x < act->get_paramSize(); x++)
            m_jcl->message[currentPosition++] = act->get_param()[x];

        m_jcl->message[3] = currentPosition - 6;

        int *ip = Utils::getIPAsArray(act->get_hostIP());
        if (strcmp(act->get_hostIP(), m_jcl->get_metadata()->get_hostIP()) == 0) {
            // printMessagePROGMEM(Constants::actingMessage);
            Serial.println("Acting");
            int pos = currentPosition - 1;
            int value = 0;
            int pin = 0;
            int k, j;
            char command[12];

            pos = 0;
            while (m_jcl->message[pos] != 74)
                pos++;
            k = m_jcl->message[++pos];

            if (k == 1)
                pin = m_jcl->message[pos + 1] - 48;
            else {
                pin = 10 * (m_jcl->message[pos + 1] - 48);
                pin += m_jcl->message[pos + 2] - 48;
            }
            while (m_jcl->message[pos] != 16)
                pos++;
            pos++;
            pos++;
            uint8_t numCmd = (int) m_jcl->message[pos];
            pos++;
            pos++;

            for (k = 0; k < numCmd; k++) {
                uint8_t nChars = m_jcl->message[pos++];
                for (j = 0; j < nChars && j < 12; j++)
                    command[j] = m_jcl->message[pos++];
                command[j] = '\0';
                pos++;

                float f = atof(command);
                value = (int) f;
                if (!m_jcl->get_sensors()[pin]->acting(value))
                    break;
            }
        } else {
            int msgSize = completeHeader(currentPosition, 9, true, act->get_hostMac(), atoi(act->get_superPeerPort()));
            EthernetClient host;
            host.connect(IPAddress(ip[0], ip[1], ip[2], ip[3]), atoi(act->get_hostPort()));
            host.write(m_jcl->message, msgSize);
            host.flush();
            while (!host.available());
            while (host.available())
                host.read();
            if (host.connected())
                host.stop();
        }
    } else {
        m_jcl->message[currentPosition++] = 8;
        m_jcl->message[currentPosition++] = 58;
        m_jcl->message[currentPosition++] = 18;
        m_jcl->message[currentPosition++] = 127;
        m_jcl->message[currentPosition++] = 122;
        m_jcl->message[currentPosition++] = 16;
        m_jcl->message[currentPosition++] = 106;
        m_jcl->message[currentPosition++] = 97;
        m_jcl->message[currentPosition++] = 118;
        m_jcl->message[currentPosition++] = 97;
        m_jcl->message[currentPosition++] = 46;
        m_jcl->message[currentPosition++] = 108;
        m_jcl->message[currentPosition++] = 97;
        m_jcl->message[currentPosition++] = 110;
        m_jcl->message[currentPosition++] = 103;
        m_jcl->message[currentPosition++] = 46;
        m_jcl->message[currentPosition++] = 79;
        m_jcl->message[currentPosition++] = 98;
        m_jcl->message[currentPosition++] = 106;
        m_jcl->message[currentPosition++] = 101;
        m_jcl->message[currentPosition++] = 99;
        m_jcl->message[currentPosition++] = 116;
        m_jcl->message[currentPosition++] = 24;
        m_jcl->message[currentPosition++] = 9;
        m_jcl->message[currentPosition++] = 16;
        m_jcl->message[currentPosition++] = 1;
        m_jcl->message[currentPosition++] = 10;

        m_jcl->message[currentPosition++] = 2;
        m_jcl->message[currentPosition++] = 8;
        if (act->is_useSensorValue())
            m_jcl->message[currentPosition++] = 1;
        else
            m_jcl->message[currentPosition++] = 0;

        m_jcl->message[currentPosition++] = 10;
        m_jcl->message[currentPosition++] = strlen(act->get_ticket()) + 2;
        m_jcl->message[currentPosition++] = 74;
        m_jcl->message[currentPosition++] = strlen(act->get_ticket());
        for (uint8_t x = 0; x < strlen(act->get_ticket()); x++)
            m_jcl->message[currentPosition++] = act->get_ticket()[x];

        m_jcl->message[currentPosition++] = 10;
        m_jcl->message[currentPosition++] = strlen(act->get_hostIP()) + 2;
        m_jcl->message[currentPosition++] = 74;
        m_jcl->message[currentPosition++] = strlen(act->get_hostIP());
        for (uint8_t x = 0; x < strlen(act->get_hostIP()); x++)
            m_jcl->message[currentPosition++] = act->get_hostIP()[x];

        m_jcl->message[currentPosition++] = 10;
        m_jcl->message[currentPosition++] = strlen(act->get_hostPort()) + 2;
        m_jcl->message[currentPosition++] = 74;
        m_jcl->message[currentPosition++] = strlen(act->get_hostPort());
        for (uint8_t x = 0; x < strlen(act->get_hostPort()); x++)
            m_jcl->message[currentPosition++] = act->get_hostPort()[x];

        m_jcl->message[currentPosition++] = 10;
        m_jcl->message[currentPosition++] = strlen(act->get_hostMac()) + 2;
        m_jcl->message[currentPosition++] = 74;
        m_jcl->message[currentPosition++] = strlen(act->get_hostMac());
        for (uint8_t x = 0; x < strlen(act->get_hostMac()); x++)
            m_jcl->message[currentPosition++] = act->get_hostMac()[x];

        if (act->get_superPeerPort() != 0) {
            m_jcl->message[currentPosition++] = 10;
            m_jcl->message[currentPosition++] = strlen(act->get_superPeerPort()) + 2;
            m_jcl->message[currentPosition++] = 74;
            m_jcl->message[currentPosition++] = strlen(act->get_superPeerPort());
            for (uint8_t x = 0; x < strlen(act->get_superPeerPort()); x++)
                m_jcl->message[currentPosition++] = act->get_superPeerPort()[x];
        }
        m_jcl->message[currentPosition++] = 10;
        m_jcl->message[currentPosition++] = act->get_classNameSize() + 2;
        m_jcl->message[currentPosition++] = 74;
        m_jcl->message[currentPosition++] = act->get_classNameSize();
        for (uint8_t x = 0; x < act->get_classNameSize(); x++)
            m_jcl->message[currentPosition++] = act->get_className()[x];

        m_jcl->message[currentPosition++] = 10;
        m_jcl->message[currentPosition++] = act->get_methodNameSize() + 2;
        m_jcl->message[currentPosition++] = 74;
        m_jcl->message[currentPosition++] = act->get_methodNameSize();
        for (uint8_t x = 0; x < act->get_methodNameSize(); x++)
            m_jcl->message[currentPosition++] = act->get_methodName()[x];

        m_jcl->message[currentPosition++] = 10;
        for (uint8_t x = 0; x < act->get_paramSize(); x++)
            m_jcl->message[currentPosition++] = act->get_param()[x];

        uint8_t arrayChar[10];
        int totalBytes = encode_unsigned_varint(arrayChar, currentPosition - 4 - 2);

        if (totalBytes > 1) {
            for (int aux = currentPosition; aux > 3; aux--)
                m_jcl->message[aux + 1] = m_jcl->message[aux];
        }
        for (int i = 0; i < totalBytes; i++)
            m_jcl->message[3 + i] = (int) arrayChar[i];
        currentPosition += totalBytes - 1;

        int msgSize = completeHeader(currentPosition, 9, true, act->get_hostMac(), atoi(act->get_superPeerPort()));

        int *ip = Utils::getIPAsArray(act->get_hostIP());
        EthernetClient host;
        host.connect(IPAddress(ip[0], ip[1], ip[2], ip[3]), atoi(act->get_hostPort()));
        host.connect(IPAddress(192, 168, 2, 109), 7070);
        host.write(m_jcl->message, msgSize);
        host.flush();
        long time = millis();
        while (!host.available() && millis() - time < 10000);
        while (host.available())
            host.read();
        if (host.connected())
            host.stop();
    }
}

void Message::unregister() {
    int size;
    int currentPosition = 0;

    m_jcl->message[currentPosition++] = 8; // Indicates first field

    /* The next fields indicates the value -2. It takes multiple bytes to indicate negative field because
    Protocol Buffer uses base 128 varints */
    m_jcl->message[currentPosition++] = -2;
    m_jcl->message[currentPosition++] = -1;
    m_jcl->message[currentPosition++] = -1;
    m_jcl->message[currentPosition++] = -1;
    m_jcl->message[currentPosition++] = -1;
    m_jcl->message[currentPosition++] = -1;
    m_jcl->message[currentPosition++] = -1;
    m_jcl->message[currentPosition++] = -1;
    m_jcl->message[currentPosition++] = -1;
    m_jcl->message[currentPosition++] = 1;

    m_jcl->message[currentPosition++] = 18;

    // Calculate size to append to the message
    int totalSize = 2 +
                    strlen(m_jcl->get_metadata()->get_hostIP()) + 2 +
                    strlen(m_jcl->get_metadata()->get_hostPort()) + 2 +
                    strlen(m_jcl->get_metadata()->get_mac()) + 2 +
                    1 + 2 +  // strlen(coreValue)
                    1 + 2;   // strlen(deviceType)

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

    m_jcl->message[currentPosition++] = 8;
    m_jcl->message[currentPosition++] = 5;

    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = strlen(m_jcl->get_metadata()->get_hostIP());
    for (unsigned int i = 0; i < strlen(m_jcl->get_metadata()->get_hostIP()); i++)
        m_jcl->message[currentPosition++] = m_jcl->get_metadata()->get_hostIP()[i];

    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = strlen(m_jcl->get_metadata()->get_hostPort());
    for (unsigned int i = 0; i < strlen(m_jcl->get_metadata()->get_hostPort()); i++)
        m_jcl->message[currentPosition++] = m_jcl->get_metadata()->get_hostPort()[i];

    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = strlen(m_jcl->get_metadata()->get_mac());
    for (unsigned int i = 0; i < strlen(m_jcl->get_metadata()->get_mac()); i++)
        m_jcl->message[currentPosition++] = m_jcl->get_metadata()->get_mac()[i];

    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = 1;  //strlen(corevalue)
    for (unsigned int i = 0; i < 1; i++)
        m_jcl->message[currentPosition++] = Constants::coreValue[i];

    m_jcl->message[currentPosition++] = 18;
    m_jcl->message[currentPosition++] = 1;
    for (unsigned int i = 0; i < 1; i++)
        m_jcl->message[currentPosition++] = Constants::deviceTypeValue[i];  // Device Type

    size = completeHeader(currentPosition, 2, true, m_jcl->get_metadata()->get_mac(), 0);

    m_jcl->get_client().write(m_jcl->message, size);
    m_jcl->get_client().flush();
    receiveServerAnswer();
}

bool Message::unregisterContext(bool isMQTTContext) {
    int pos = 14;
    while (m_jcl->message[pos] != 74)
        pos++;
    int nChars = m_jcl->message[++pos];

    char *nickname = (char *) malloc(sizeof(char) * nChars + 1);
    for (uint8_t x = 0; x < nChars; x++)
        nickname[x] = m_jcl->message[x + pos + 1];
    nickname[nChars] = '\0';

    for (int sensorPos = 0; sensorPos < m_jcl->get_totalSensors(); sensorPos++) {
        if (m_jcl->get_sensors()[sensorPos] != NULL) {
            Sensor *s = m_jcl->get_sensors()[sensorPos];
            for (int ctxPos = 0; ctxPos < s->get_numContexts(); ctxPos++) {
                Context *ctx = s->getEnabledContexts()[ctxPos];
                if (strcmp(ctx->get_nickname(), nickname) == 0 && isMQTTContext == ctx->is_mqttContext()) {
                    Serial.println("Exists");

                    if (s->get_numContexts() == 1 || ctxPos == s->get_numContexts() - 1)
                        s->getEnabledContexts()[ctxPos] = NULL;
                    else {
                        s->getEnabledContexts()[ctxPos] = s->getEnabledContexts()[s->get_numContexts() - 1];
                        s->getEnabledContexts()[s->get_numContexts() - 1] = NULL;
                    }
                    ctx->deleteContext();
                    s->set_numContexts(s->get_numContexts() - 1);
                    return true;
                }
            }
        }
    }

    return false;
}

bool Message::removeContextAction(bool isActing) {
    uint16_t pos = 17;
    while (m_jcl->message[pos] != 74)
        pos++;
    pos++;
    uint8_t nChars = m_jcl->message[pos++];
    char contextName[nChars + 1];
    for (uint8_t x = 0; x < nChars; x++)
        contextName[x] = m_jcl->message[pos + x];
    contextName[nChars] = '\0';

    /*uint8_t nChars=m_jcl->message[43];
    char contextName[nChars+1];
    for (uint8_t x=0; x < nChars; x++)
      contextName[x] = m_jcl->message[44 + x];
    contextName[nChars] = '\0';*/
    Context *ctx = NULL;
    for (uint16_t x = 0; x < TOTAL_SENSORS; x++)
        if (m_jcl->getSensors()[x]->getNumContexts() > 0) {
            for (uint8_t ctxNumber = 0; ctxNumber < m_jcl->getSensors()[x]->getNumContexts(); ctxNumber++)
                if (strcmp(m_jcl->getSensors()[x]->getEnabledContexts()[ctxNumber]->getNickname(), contextName) == 0) {
                    ctx = m_jcl->getSensors()[x]->getEnabledContexts()[ctxNumber];
                    break;
                }
        }
    Serial.print("1: ");
    Serial.println(ctx == NULL);
    if (ctx == NULL)
        return false;

    if (isActing) {
        uint16_t pos = 47 + nChars;
        nChars = m_jcl->message[pos++];
        char *hostIP = (char *) malloc(sizeof(char) * nChars + 1);
        for (uint8_t x = 0; x < nChars; x++)
            hostIP[x] = m_jcl->message[pos + x];
        hostIP[nChars] = '\0';

        pos += nChars;
        while (m_jcl->message[pos] != 74)
            pos++;
        nChars = m_jcl->message[++pos];
        pos++;
        char *hostPort = (char *) malloc(sizeof(char) * nChars + 1);
        for (uint8_t x = 0; x < nChars; x++)
            hostPort[x] = m_jcl->message[pos + x];
        hostPort[nChars] = '\0';

        pos += nChars;
        while (m_jcl->message[pos] != 74)
            pos++;
        nChars = m_jcl->message[++pos];
        pos++;
        char *portSuperPeer = (char *) malloc(sizeof(char) * nChars + 1);
        for (uint8_t x = 0; x < nChars; x++)
            portSuperPeer[x] = m_jcl->message[pos + x];
        portSuperPeer[nChars] = '\0';

        pos += nChars + 95;
        while (m_jcl->message[pos] != 3)
            pos++;
        /*pos++;
        while (m_jcl->message[pos] != 3)
          pos++;*/

        nChars = m_messageSize - pos;
        char *param = (char *) malloc(sizeof(char) * nChars + 1);
        for (uint16_t x = 0; x < nChars; x++)
            param[x] = m_jcl->message[pos + x];
        param[nChars] = '\0';

        for (int actPos = 0; actPos < ctx->get_numActions(); actPos++) {
            Action *act = ctx->getEnabledActions()[actPos];
            if (act->is_acting() && strcmp(act->get_hostIP(), hostIP) == 0 && strcmp(act->get_hostPort(), hostPort) == 0
                && strcmp(act->get_superPeerPort(), portSuperPeer) == 0 && strcmp(act->get_param(), param) == 0) {

                if (ctx->get_numActions() == 1 || actPos == ctx->get_numActions() - 1) {
                    ctx->getEnabledActions()[actPos] = NULL;
                } else {
                    ctx->getEnabledActions()[actPos] = ctx->getEnabledActions()[ctx->get_numActions() - 1];
                    ctx->getEnabledActions()[ctx->get_numActions() - 1] = NULL;
                }

                act->deleteAction();
                ctx->set_numActions(ctx->get_numActions() - 1);
                return true;
            }
        }
        return false;
    } else {
        bool useSensorValue;
        pos += nChars;
        while (m_jcl->message[pos] != 74)
            pos++;
        pos++;
        nChars = m_jcl->message[pos++];
        if (m_jcl->message[pos] == 't')
            useSensorValue = true;
        else
            useSensorValue = false;

        pos += nChars;
        while (m_jcl->message[pos] != 74)
            pos++;
        pos++;
        nChars = m_jcl->message[pos++];
        char *className = new char[nChars + 1];
        for (uint8_t x = 0; x < nChars; x++)
            className[x] = m_jcl->message[pos + x];
        className[nChars] = '\0';

        pos += nChars;
        while (m_jcl->message[pos] != 74)
            pos++;
        nChars = m_jcl->message[++pos];
        char *methodName = new char[nChars + 1];
        for (uint8_t x = 0; x < nChars; x++)
            methodName[x] = m_jcl->message[pos + x + 1];
        methodName[nChars] = '\0';

        while (m_jcl->message[pos] != 122)
            pos++;
        pos--;
        char *param = new char[m_messageSize - pos + 1];
        for (int x = pos; x < m_messageSize; x++)
            param[x - pos] = m_jcl->message[x];
        param[m_messageSize - pos] = '\0';

        for (int actPos = 0; actPos < ctx->get_numActions(); actPos++) {
            Action *act = ctx->getEnabledActions()[actPos];
            if (!act->is_acting() && act->is_useSensorValue() == useSensorValue
                && strcmp(act->get_className(), className) == 0 && strcmp(act->get_methodName(), methodName) == 0
                && strcmp(act->get_param(), param) == 0) {

                if (ctx->get_numActions() == 1 || actPos == ctx->get_numActions() - 1) {
                    ctx->getEnabledActions()[actPos] = NULL;
                } else {
                    ctx->getEnabledActions()[actPos] = ctx->getEnabledActions()[ctx->get_numActions() - 1];
                    ctx->getEnabledActions()[ctx->get_numActions() - 1] = NULL;
                }
                act->deleteAction();
                ctx->set_numActions(ctx->get_numActions() - 1);
                return true;
            }
        }
        return false;
    }
}