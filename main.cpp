#include "mbed.h"
#include <string.h>
#include <sstream>
#include "SimpleMQTT.h"
#include "ADXL345_I2C.h"

using namespace std;

DigitalOut led(LED1); // Blinks when we're connecting...
Ticker connectivityTicker;

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
    AnalogIn temperatureSensor(A1);

    while (1) {

        unsigned int a, beta = 3975;
        float temperature, resistance;

        a = temperatureSensor.read_u16(); /* Read analog value */

        /* Calculate the resistance of the thermistor from analog votage read. */
        resistance = (float) 10000.0 * ((65536.0 / a) - 1.0);

        /* Convert the resistance to temperature using Steinhart's Hart equation */
        temperature = (1 / ((log(resistance / 10000.0) / beta) + (1.0 / 298.15))) - 273.15;

        mqtt_publish_float(client, mac_address + "/temperature", temperature);

        wait_ms(3000); // 3 seconds between gathering
    }
}

void gatherMoistureData() {
    // Declare the sensor
    AnalogIn moistureSensor(A0);

    while (1) {
        mqtt_publish_float(client, mac_address + "/moisture", moistureSensor.read());

        wait_ms(3000); // 3 seconds between gathering
    }
}

void gatherAccelerometerData() {
    ADXL345_I2C accelerometer(D14, D15);

    // These are here to test whether any of the initialization fails. It will print the failure
    if (accelerometer.setPowerControl(0x00)) {
        printf("Accelerometer init failed... Didn't intitialize power control\n");
        return;
    }
    // Full resolution, +/-16g, 4mg/LSB.
    wait(.001);

    if (accelerometer.setDataFormatControl(0x0B)) {
        printf("Accelerometer init failed... Didn't set data format\n");
        return;
    }
    wait(.001);

    // 3.2kHz data rate.
    if (accelerometer.setDataRate(ADXL345_3200HZ)) {
        printf("Accelerometer init failed... Didn't set data rate\n");
        return;
    }
    wait(.001);

    // Measurement mode.
    if (accelerometer.setPowerControl(MeasurementMode)) {
        printf("Accelerometer init failed... Didn't set the power control to measurement\n");
        return;
    }

    printf("Accelerometer is initialized!\r\n");

    while (1) {
        printf("Start reading data\r\n");

        std::stringstream x;
        std::stringstream y;
        std::stringstream z;

        int readings[3] = { 0, 0, 0 };

        int intervals = 10 * 30; // 10 seconds * 30 measurements
        int timeout = 33;       // ms. timeout between measuring

        for (size_t ix = 0; ix < intervals; ix++) {
            accelerometer.getOutput(readings);

            x << readings[0];
            x << ",";

            y << readings[1];
            y << ",";

            z << readings[2];
            z << ",";

            wait_ms(timeout);
        }

        printf("Done reading data\r\n");

        // @todo, should happen on different thread so we don't block here...
        mqtt_publish_string(client, mac_address + "/accelerometer/x", x.str());
        mqtt_publish_string(client, mac_address + "/accelerometer/y", y.str());
        mqtt_publish_string(client, mac_address + "/accelerometer/z", z.str());
    }
}

// Blink the LED when
void blink_led() {
    led = !led;
}

int main(int argc, char* argv[])
{
    Serial pc(USBTX, USBRX);
    pc.baud(115200);

    printf("DSA2017 Data Gathering\r\n");

    connectivityTicker.attach(&blink_led, 0.5f);

    // Connects to the network, and connects to MQTT server
    client = connect_mqtt("192.168.8.101", 1883, &mac_address);

    // List all topic that you'll use in here...
    string topics[] = {
        mac_address + "/temperature",
        mac_address + "/moisture",
        mac_address + "/accelerometer/x",
        mac_address + "/accelerometer/y",
        mac_address + "/accelerometer/z"
    };

    for (size_t ix = 0; ix < sizeof(topics) / sizeof(topics[0]); ix++) {
        printf("Subscribing to topic %s\r\n", topics[ix].c_str());

        // Subscribe to the topic, so we know when our message arrived at the server (useful to detect disconnects)
        while ((client->subscribe(topics[ix].c_str(), MQTT::QOS2, messageArrived)) != 0) {
            printf("Could not subscribe to topic...\r\n");
            wait_ms(1000);
        }
    }

    connectivityTicker.detach(); // stop blinking when connected
    led = 1; // and put the LED on indefinitely

    // Schedule a data-gathering thread (un-comment the sensor that you want to run)
    Thread dataThread(osPriorityNormal, 16 * 1024);

    dataThread.start(&gatherTemperatureData);
    // dataThread.start(&gatherMoistureData);
    // dataThread.start(&gatherAccelerometerData);

    wait(osWaitForever);
}
