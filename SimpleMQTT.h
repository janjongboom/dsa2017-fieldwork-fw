/**
 * Small wrapper library around EasyConnect and the MQTT lib for DSA 2017
 */

#ifndef _SIMPLE_MQTT_H_
#define _SIMPLE_MQTT_H_

#include <string.h>
#include "easy-connect.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

using namespace std;

MQTT::Client<MQTTNetwork, Countdown>* connect_mqtt(string host, uint16_t port, string* mac_address) {
    // So this leaks... But it does not matter because we kill the full program when disconnect happens
    NetworkInterface* network;
    MQTTNetwork* mqttNetwork;

    while ((network = easy_connect(true)) == NULL) {
        printf("[SimpleMQTT] Initial connection to network failed...\r\n");
        wait_ms(1000);
    }
    printf("[SimpleMQTT] Initial connection to network OK\r\n");

    string* mc = new string(network->get_mac_address());

    *mac_address = *mc;

    mqttNetwork = new MQTTNetwork(network);

    MQTT::Client<MQTTNetwork, Countdown>* client = new MQTT::Client<MQTTNetwork, Countdown>(*mqttNetwork);

    const char* hostname = host.c_str();
    printf("[SimpleMQTT] Connecting to %s:%d\r\n", hostname, port);
    int rc = -1;

    while ((rc = mqttNetwork->connect(hostname, port)) != 0) {
        printf("[SimpleMQTT] rc from TCP connect is %d\r\n", rc);
        wait_ms(1000);
    }

    printf("[SimpleMQTT] TCP connect OK\r\n");

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "mbed-dsa";
    // data.username.cstring = "testuser";
    // data.password.cstring = "testpassword";
    while ((rc = client->connect(data)) != 0) {
        printf("[SimpleMQTT] rc from MQTT connect is %d\r\n", rc);
        wait_ms(1000);
    }

    printf("[SimpleMQTT] MQTT connect OK\r\n");

    return client;
}

#endif // _SIMPLE_MQTT_H_
