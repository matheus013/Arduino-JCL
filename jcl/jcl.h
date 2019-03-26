#ifndef JCL_h
#define JCL_h

#include "metadata.h"
#include <Ethernet.h>
//include <UIPEthernet.h>
#include <SPI.h>
#include "sensor.h"
#include "constants.h"
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <PubSubClient.h>
#include "helpers.h"

static const int TOTAL_DIGITAL_SENSORS = 54;
static const int TOTAL_SENSORS = 70;

class JCL {
public:
    JCL(int port, char *mac);

    void changeBoardNickname(char *boardName);

    void configureJCLServer(char *serverIP, int serverPort);

    void setBrokerData(char* brokerIP, int brokerPort);

    void startHost();

    void writeEprom();

    void readEprom();

    void listSensors();

    void run();

    void connectToBroker();

    static int get_totalSensors();

    static int get_totalDigitalSensors();

    void useEEPROM(bool useEEPROM);

    int freeRam();

    Sensor **get_sensors();

    int numSensors;
    char message[850];

private:
    void beginEthernet();

    void connectToServer();

    void sendBroadcastMessage();

    void makeSensing();

    void checkContext(int pin);

    bool checkCondition(int sensorValue, char *operation, char *threshold, float lastValue);

ATTRIBUTE_OBJECT(char *, boardName)
ATTRIBUTE_OBJECT(Metadata *, metadata)
ATTRIBUTE_OBJECT(EthernetClient, client)
ATTRIBUTE_OBJECT(EthernetClient, requestListener)
ATTRIBUTE_OBJECT(EthernetClient, mqttEthClient)
ATTRIBUTE_OBJECT(char *, key)
ATTRIBUTE_OBJECT(PubSubClient *, mqtt)
ATTRIBUTE_BOOL(encryption)
ATTRIBUTE_BOOL(eeprom)
    Sensor *sensors[TOTAL_SENSORS];
};

#endif
