#include "mbed.h"
#include <string.h>
#include <sstream>
#include "SimpleMQTT.h"

using namespace std;

MQTT::Client<MQTTNetwork, Countdown>* client;
string mac_address; // here we'll store our MAC address, used to construct the MQTT topic

int arrivedcount = 0;

void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    // printf("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
}

void gatherTemperatureData() {
    // Declare the sensor
    AnalogIn temperatureSensor(A0);

    MQTT::Message message;
    string topic = mac_address + "/temperature";

    while (1) {

        unsigned int a, beta = 3975;
        float temperature, resistance;

        a = temperatureSensor.read_u16(); /* Read analog value */

        /* Calculate the resistance of the thermistor from analog votage read. */
        resistance = (float) 10000.0 * ((65536.0 / a) - 1.0);

        /* Convert the resistance to temperature using Steinhart's Hart equation */
        temperature = (1 / ((log(resistance / 10000.0) / beta) + (1.0 / 298.15))) - 273.15;

        printf("Publishing %f on topic %s\r\n", temperature, topic.c_str());

        stringstream ss;
        ss << temperature;

        const char* buffer = ss.str().c_str();

        message.qos = MQTT::QOS0;
        message.retained = false;
        message.dup = false;
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

        wait_ms(3000); // 3 seconds between gathering
    }
}

int main(int argc, char* argv[])
{
    Serial pc(USBTX, USBRX);
    pc.baud(115200);

    printf("DSA2017 Data Gathering\r\n");

    // Connects to the network, and connects to MQTT server
    client = connect_mqtt("192.168.8.101", 1883, &mac_address);

    // List all topic that you'll use in here...
    string topics[] = {
        mac_address + "/temperature"
    };

    for (size_t ix = 0; ix < sizeof(topics) / sizeof(topics[0]); ix++) {
        printf("Subscribing to topic %s\r\n", topics[ix].c_str());

        // Subscribe to the topic, so we know when our message arrived at the server (useful to detect disconnects)
        while ((client->subscribe(topics[ix].c_str(), MQTT::QOS2, messageArrived)) != 0) {
            printf("Could not subscribe to topic...\r\n");
            wait_ms(1000);
        }
    }

    // Schedule a data-gathering thread
    Thread dataThread;
    dataThread.start(&gatherTemperatureData);

    wait(osWaitForever);
}
