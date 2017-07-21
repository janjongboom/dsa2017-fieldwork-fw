/**
 * Small wrapper library around EasyConnect and the MQTT lib for DSA 2017
 */

#ifndef _SIMPLE_MQTT_H_
#define _SIMPLE_MQTT_H_

#include <string.h>
#include <sstream>
#include "easy-connect.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

using namespace std;

extern int arrivedcount;

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

void mqtt_publish_internal(MQTT::Client<MQTTNetwork, Countdown>* client, string topic, string value) {
    MQTT::Message message;

    const char* buffer = value.c_str();

    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    printf("Payloadlen is %d\n", strlen(buffer));
    message.payload = (void*)buffer;
    message.payloadlen = strlen(buffer) + 1;
    client->publish(topic.c_str(), message);

    uint32_t wait_time = 5000;

    while (arrivedcount < 1) {
        wait_time -= 100;

        if (wait_time > 0) {
            client->yield(100);
        }
        else {
            printf("Client disconnected\r\n");

            // If the client disconnects, reset the board...
            // The ESP8266 module (or the driver) seem buggy and I cannot update the firmware on the module used at DSA2017.
            NVIC_SystemReset();
        }
    }

    arrivedcount = 0;
}

void mqtt_publish_float(MQTT::Client<MQTTNetwork, Countdown>* client, string topic, float value) {
    printf("Publishing %f on topic %s\r\n", value, topic.c_str());

    // Add the values that you want to publish here
    stringstream ss;
    ss << value;

    mqtt_publish_internal(client, topic, ss.str());
}

void mqtt_publish_string(MQTT::Client<MQTTNetwork, Countdown>* client, string topic, string value) {
    printf("Publishing on topic %s:\r\n%s\r\n", topic.c_str(), value.c_str());

    mqtt_publish_internal(client, topic, value);
}

#endif // _SIMPLE_MQTT_H_
