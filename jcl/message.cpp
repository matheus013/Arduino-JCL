//
// Created by Matheus Inácio on 2019-03-19.
//

#include "jcl.h"
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