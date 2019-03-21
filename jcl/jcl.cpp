#include "jcl.h"
#include "metadata.h"
#include "utils.h"
#include "sensor.h"
#include "message.h"
#include "constants.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <Ethernet.h>
#include <SPI.h>

void writeEpromHelp(int *j, char *str);

char *readEpromHelp(int *j);

JCL::JCL(int hostPort, char *mac) {
    delay(7500);
    numSensors = 0;
    this->m_metadata = new Metadata();
    this->m_metadata->set_mac(mac);
    char port[8];
    itoa(hostPort, port, 10);
    this->m_metadata->set_hostPort(port);
    this->set_encryption(false);
    this->useEEPROM(true);
    char defaultBoardName[] = "arduino";
    this->set_boardName(defaultBoardName);

    for (int i = 0; i < TOTAL_SENSORS; i++)
        this->get_sensors()[i] = NULL;
}

void JCL::configureJCLServer(char *serverIP, int serverPort) {
    m_metadata->set_serverIP(serverIP);
    char port[8];
    itoa(serverPort, port, 10);
    m_metadata->set_serverPort(port);
}

void JCL::startHost() {
    if (this->is_eeprom()) {
        readEprom();
    }
    beginEthernet();
    connectToServer();
    listSensors();
    run();
}

void JCL::readEprom() {
    int addr = 0;
    int valueA = EEPROM.read(addr++);
    int numBytes;
    int numSensors = 0;
    if (valueA != 1)
        return;

    char c1, c2;
    c1 = EEPROM.read(addr++);
    c2 = EEPROM.read(addr++);
    numBytes = ((c1 & 0xff) << 8) | ((c2 & 0xff));

    char *nameBoard = readEpromHelp(&addr);
    this->get_metadata()->set_boardName(nameBoard);

    char *hostIP = readEpromHelp(&addr);
    this->get_metadata()->set_hostIP(hostIP);

    char *hostPort = readEpromHelp(&addr);
    this->get_metadata()->set_hostPort(hostPort);

    char *serverIP = readEpromHelp(&addr);
    this->get_metadata()->set_serverIP(serverIP);

    char *serverPort = readEpromHelp(&addr);
    this->get_metadata()->set_serverPort(serverPort);

    while (addr < numBytes) {
        int pin = EEPROM.read(addr++);
        Sensor *s = new Sensor;

        this->get_sensors()[pin] = s;

        char *pinStr = new char[4];
        sprintf(pinStr, "%d", pin);
        s->set_pin(pinStr);

        numSensors++;

        char *nickname = readEpromHelp(&addr);
        s->set_sensorNickname(nickname);

        s->set_typeIO(EEPROM.read(addr++));
        s->set_type(EEPROM.read(addr++));

        char *delay = readEpromHelp(&addr);
        s->set_delay(delay);

        char *sensorSize = readEpromHelp(&addr);
        s->set_sensorSize(sensorSize);

        s->set_numContexts(EEPROM.read(addr++));
        s->configurePinMode();

        for (int numCtx = 0; numCtx < s->get_numContexts(); numCtx++) {
            Context *ctx = new Context();

            s->getEnabledContexts()[numCtx] = ctx;

            char *ctxName = readEpromHelp(&addr);
            ctx->set_nickname(ctxName);

            char *expression = readEpromHelp(&addr);
            ctx->set_expression(expression);

            ctx->set_numExpressions(EEPROM.read(addr++));

            for (int numExp = 0; numExp < ctx->get_numExpressions(); numExp++) {
                char *op = readEpromHelp(&addr);
                ctx->get_operators()[numExp] = op;

                char *threshold = readEpromHelp(&addr);
                ctx->get_threshold()[numExp] = threshold;
            }

            ctx->set_numActions(EEPROM.read(addr++));
            for (int numActions = 0; numActions < ctx->get_numActions(); numActions++) {
                Action *act = new Action();
                ctx->getEnabledActions()[numActions] = act;

                act->set_acting(EEPROM.read(addr++));

                char *hostIPAct = readEpromHelp(&addr);
                act->set_hostIP(hostIPAct);

                char *hostPortAct = readEpromHelp(&addr);
                act->set_hostPort(hostPortAct);

                char nChars;
                if (!act->is_acting()) {
                    act->set_useSensorValue(EEPROM.read(addr++));

                    char *hostMACAct = readEpromHelp(&addr);
                    act->set_hostMac(hostMACAct);

                    char *ticket = readEpromHelp(&addr);
                    act->set_ticket(ticket);

                    nChars = EEPROM.read(addr);
                    act->set_classNameSize(nChars);

                    char *className = readEpromHelp(&addr);
                    act->set_className(className);

                    nChars = EEPROM.read(addr);
                    act->set_methodNameSize(nChars);

                    char *methodName = readEpromHelp(&addr);
                    act->set_methodName(methodName);
                }
                nChars = EEPROM.read(addr);
                act->set_paramSize(nChars);
                char *param = readEpromHelp(&addr);
                act->set_param(param);
            }
        }
    }
    char *nSensors = new char[4];
    sprintf(nSensors, "%d", numSensors);
    this->get_metadata()->set_numConfiguredSensors(nSensors);
    Serial.println(numSensors);
}

void JCL::writeEprom() {
    uint16_t addr = 0;
    unsigned int i;
    EEPROM.write(addr++, 1); // Indicates that there's information on EEPROM

    // At the end these two bytes indicates the size of the info stored on EEPROM
    EEPROM.write(addr++, Constants::SEPARATOR);
    EEPROM.write(addr++, Constants::SEPARATOR);

    writeEpromHelp(&addr, get_metadata()->get_boardName());
    writeEpromHelp(&addr, get_metadata()->get_hostIP());
    writeEpromHelp(&addr, get_metadata()->get_hostPort());
    writeEpromHelp(&addr, get_metadata()->get_serverIP());
    writeEpromHelp(&addr, get_metadata()->get_serverPort());
    // writeEpromHelp(&addr, getMetadata()->getHostIP());

    for (int k = 0; k < TOTAL_SENSORS; k++)
        if (sensors[k] != NULL) {
            Sensor *s = get_sensors()[k];
            EEPROM.write(addr++, k);
            writeEpromHelp(&addr, s->get_sensorNickname());

            EEPROM.write(addr++, s->get_typeIO());
            EEPROM.write(addr++, (char) s->get_type());

            writeEpromHelp(&addr, s->get_delay());
            writeEpromHelp(&addr, s->get_sensorSize());

            EEPROM.write(addr++, s->get_numContexts());

            for (int numCtx = 0; numCtx < s->get_numContexts(); numCtx++) {
                Context *ctx = s->getEnabledContexts()[numCtx];
                writeEpromHelp(&addr, ctx->get_nickname());
                writeEpromHelp(&addr, ctx->get_expression());

                EEPROM.write(addr++, ctx->get_numExpressions());

                for (int numExp = 0; numExp < ctx->get_numExpressions(); numExp++) {
                    writeEpromHelp(&addr, ctx->get_operators()[numExp]);
                    writeEpromHelp(&addr, ctx->get_threshold()[numExp]);
                }

                EEPROM.write(addr++, ctx->get_numActions());

                for (int numActions = 0; numActions < ctx->get_numActions(); numActions++) {
                    Action *act = ctx->getEnabledActions()[numActions];

                    EEPROM.write(addr++, act->is_acting());
                    writeEpromHelp(&addr, act->get_hostIP());
                    writeEpromHelp(&addr, act->get_hostPort());

                    if (!act->is_acting()) {
                        EEPROM.write(addr++, act->is_useSensorValue());
                        writeEpromHelp(&addr, act->get_hostMac());
                        writeEpromHelp(&addr, act->get_ticket());
                        writeEpromHelp(&addr, act->get_classNameSize());
                        writeEpromHelp(&addr, act->get_methodNameSize());
                    }
                    writeEpromHelp(&addr, act->get_paramSize());
                }
            }
        }

    EEPROM.write(1, (addr >> 8) & 0xFF);
    EEPROM.write(2, addr & 0xFF);
}

void JCL::changeBoardNickname(char *boardName) {
    m_metadata->set_boardName(boardName);
}

void JCL::beginEthernet() {
    Serial.println("DHCP");
    if (Ethernet.begin(Utils::macAsByteArray(m_metadata->get_mac())) != 0) {
        IPAddress adr = Ethernet.localIP();
        sprintf(m_metadata->get_hostIP(), "%d.%d.%d.%d", (int) adr[0], (int) adr[1], (int) adr[2], (int) adr[3]);
        Serial.println((int) adr[0]);
        Serial.println(m_metadata->get_hostIP());
    } else {
        Serial.println("Could not get IP address through DHCP");
        for (;;);
    }
}

void JCL::connectToServer() {
    int *ip = Utils::getIPAsArray(m_metadata->get_serverIP());
    while (!m_client.connected()) {
        Serial.print("Server: ");
        Serial.print(m_metadata->get_serverIP());
        Serial.print("  ");
        Serial.println(m_metadata->get_serverPort());
        client.connect(IPAddress(ip[0], ip[1], ip[2], ip[3]), atoi(m_metadata->get_serverPort()));
        if (millis() >= 12000 && !m_client.connected()) {
            Serial.println("Not Connected");
            break;
            // Message::printMessagePROGMEM(Constants::connectionErrorMessage);
        } else if (m_client.connected()) {
            Serial.println("Connected");
            // Message::printMessagePROGMEM(Constants::connectedMessage);
            break;
        }
    }
    while (!m_client.connected()) {
        sendBroadcastMessage();
        Serial.println(m_metadata->get_serverIP());
        Serial.println(m_metadata->get_serverPort());
        ip = Utils::getIPAsArray(m_metadata->get_serverIP());
        m_client.connect(IPAddress(ip[0], ip[1], ip[2], ip[3]), atoi(m_metadata->get_serverPort()));
        delay(100);
    }
    Message msg(this);
    msg.sendMetadata(-1);
}

void JCL::listSensors() {
    // Message::printMessagePROGMEM(Constants::configuredSensorsMessage);
    Serial.print("free: ");
    Serial.println(freeRam());
    for (int i = 0; i < TOTAL_SENSORS; i++) {
        if (sensors[i] != NULL) {
            Serial.println(sensors[i].toString())
            for (int k = 0; k < sensors[i]->getNumContexts(); k++) {
                Serial.println(sensors[i]->get_enabledContexts()[k]->toString());
                for (int y = 0; y < sensors[i]->get_enabledContexts()[k]->getNumExpressions(); y++) {
                    Serial.print("   ||");
                    Serial.print(sensors[i]->get_enabledContexts()[k]->get_operators()[y]);
                    Serial.println(sensors[i]->get_enabledContexts()[k]->get_threshold()[y]);
                }
            }
            Serial.println();
        }
    }
}

void JCL::run() {
    EthernetServer s(atoi(get_metadata()->get_hostPort()));
    s.begin();
    while (true) {
        if (!this->get_metadata()->is_standBy()) {
            this->makeSensing();
        }
        int currentPosition = 0;
        this->m_requestListener = s.available();
        if (m_requestListener) {
            while (this->m_requestListener.available()) {
                message[currentPosition++] = (char) this->m_requestListener.read();
            }
            Message msg(this, currentPosition);
            msg.treatMessage();
        }
    }
}

void JCL::conectToBroker() {
    m_mqtt = new PubSubClient(mqttEthClient);

    m_mqtt->setServer(get_metadata()->get_brokerIP(), m_metadata->get_brokerPort());
    Serial.println(get_metadata()->get_brokerIP());
    Serial.println(get_metadata()->get_brokerPort());

    for (int i = 0; i < 4; i++) {
        if (m_mqtt->connect(get_metadata()->get_boardName())) {
            Serial.println("connected");
            break;
        }
        delay(1000);
    }
}

void JCL::sendBroadcastMessage() {
    Serial.println("Server Discovery");
    int UDP_PORT = 9696;
    char packetBuffer[18];

    EthernetUDP udp;
    udp.begin(UDP_PORT);
    IPAddress broadcastIp(255, 255, 255, 255);
    while (true) {
        udp.beginPacket(broadcastIp, UDP_PORT);
        udp.write("SERVERMAINPORT\n");
        udp.endPacket();
        int pos = 0;
        int packetSize = udp.parsePacket();
        if (packetSize) {
            IPAddress remote = udp.remoteIP();
            char ip[18], port[8];
            for (int i = 0; i < 4; i++) {
                char part[4];
                sprintf(part, "%d", remote[i]);
                for (int k = 0; k < strlen(part); k++)
                    ip[pos++] = part[k];
                if (i < 3)
                    ip[pos++] = '.';
                else
                    ip[pos] = '\0';
            }
            udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
            pos = 0;
            for (int k = 0; k < packetSize; k++)
                port[pos++] = packetBuffer[k];
            port[pos] = '\0';
            m_metadata->set_serverIP(ip);
            m_metadata->set_serverPort(port);
            Serial.println(ip);
            Serial.println(port);
            break;
        }
        delay(5000);
    }
    udp.stop();
}

void JCL::makeSensing() {
    for (int i = 0; i < TOTAL_SENSORS; i++) {
        Sensor *s = this->get_sensors()[i];

        if (s != NULL)
            checkContext(i);

        if (s != NULL && millis() - s->get_lastExecuted() >= atoi(s->get_delay())) {
            unsigned long currentMillis = millis();

            Message m(this);
            m.sensing(atoi(s->get_pin()), false);
        }
    }
}

void JCL::checkContext(int pin) {
    if (sensors[pin] != NULL) {
        for (uint16_t x = 0; x < sensors[pin]->get_numContexts(); x++) {
            int sensorValue;
            if (pin >= TOTAL_DIGITAL_SENSORS) {
                sensorValue = analogRead(pin - TOTAL_DIGITAL_SENSORS);
            } else
                sensorValue = digitalRead(pin);
            Context *c = sensors[pin]->getEnabledContexts()[x];
            if (checkCondition(sensorValue, c->get_operators()[0], c->get_threshold()[0], sensors[pin]->get_value())) {
                if (c->is_mqttContext()) {
                    if (m_mqtt->connected()) {
                        char mqttMessage[6];
                        sprintf(mqttMessage, "%d", sensorValue);
                        m_mqtt->publish(c->get_nickname(), mqttMessage);
                    }
                } else if (!c->is_triggered()) {
                    Serial.println("** Context reached **");
                    for (uint8_t l = 0; l < c->get_numActions(); l++) {
                        Message m(this);
                        m.sendContextActionMessage(c->getEnabledActions()[l]);
                    }
                }
                c->set_triggered(true);
            } else {
                if (c->is_mqttContext() && c->is_triggered()) {
                    char m[] = "done";
                    m_mqtt->publish(c->get_nickname(), m);
                }
                c->set_triggered(false);
            }
        }
    }
}

bool JCL::checkCondition(int sensorValue, char *operation, char *threshold, float lastValue) {

    int theresholdINT = atoi(threshold);
    if (strcmp(operation, ">") == 0) {
        if (sensorValue > theresholdINT)
            return true;
    } else if (strcmp(operation, "<") == 0) {
        if (sensorValue < theresholdINT)
            return true;
    } else if (strcmp(operation, "=") == 0) {
        if (sensorValue == theresholdINT)
            return true;
    } else if (strcmp(operation, "<=") == 0) {
        if (sensorValue <= theresholdINT)
            return true;
    } else if (strcmp(operation, ">=") == 0) {
        if (sensorValue >= theresholdINT)
            return true;
    } else if (strcmp(operation, "~") == 0) {
        if (abs(sensorValue - lastValue) >= theresholdINT)
            return true;
    }
    return false;
}

char *readEpromHelp(int *j) {
    int nChars = EEPROM.read((*j)++);
    char *result = new char[nChars + 1];
    for (int i = 0; i < nChars; i++) {
        result[i] = EEPROM.read((*j)++);
    }
    result[nChars] = '\0';
    return result;
}

void writeEpromHelp(int *j, char *str) {
    EEPROM.write((*j)++, strlen(str));
    for (int i = 0; i < strlen(str); i++) {
        EEPROM.write((*j)++, str[i]);
    }
}

int JCL::get_totalSensors() {
    return TOTAL_SENSORS;
}

int JCL::get_totalDigitalSensors() {
    return TOTAL_DIGITAL_SENSORS;
}

void JCL::useEEPROM(bool useEEPROM) {
    set_eeprom(useEEPROM);
}

int JCL::freeRam() {
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

Sensor **JCL::get_sensors() {
    return this->sensors;
}