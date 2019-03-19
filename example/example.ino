#include <jcl.h>

void setup() {
  Serial.begin(9600);
  char  hostMAC[] = "BA-DB-CD-DD-EE-FF",
        serverIP[] = "192.168.1.19";
  char * nameBoard = "arduino";
  int hostPort = 5151,
      serverPort = 6969;

  JCL jcl(hostPort, hostMAC);
  jcl.configureJCLServer(serverIP, serverPort);
  jcl.set_encryption(false); // optional (Default = false)
  jcl.changeBoardNickname(nameBoard); // optional (Board Nickname)
  jcl.useEEPROM(false);  // optional (default = true) if false Arduino won't restore the configuration of all sensors and contexts when a reboot occurs
  jcl.set_brokerData(serverIP, 1883);  // mqtt broker configuration
  jcl.startHost();
}

// no code is necessary in this method
void loop() {

}