#include "select_project.h"
#include "mbed.h"
#include <string.h>
#include <sstream>

#if SELECT_PROJECT == ACCELEROMETER

#include "ADXL345_I2C.h"
#include "easy-connect.h"

using namespace std;

DigitalOut led(LED1); // Blinks when we're connecting...
Ticker connectivityTicker;

NetworkInterface* network;
string mac_address;
UDPSocket socket;

// Structure to store the accelerometer readings
typedef struct {
    char mac[17];
    int16_t x[330];
    int16_t y[330];
    int16_t z[330];
} AccelerometerData_t;

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

        AccelerometerData_t accel_data;
        memcpy(accel_data.mac, mac_address.c_str(), 17);

        int readings[3] = { 0, 0, 0 };

        int seconds = 10;                     // seconds
        int timeout = 33;                     // ms. timeout between measuring
        int intervals = seconds * timeout;    // number of intervals (NEEDS TO MATCH THE AccelerometerData_t type)

        for (size_t ix = 0; ix < intervals; ix++) {
            accelerometer.getOutput(readings);

            accel_data.x[ix] = readings[0];
            accel_data.y[ix] = readings[1];
            accel_data.z[ix] = readings[2];

            wait_ms(timeout);
        }

        printf("X: ");
        for (size_t ix = 0; ix < intervals; ix++) {
            printf("%d ", accel_data.x[ix]);
        }
        printf("\n");

        printf("Done reading data\r\n");

        printf("Sending data\r\n");
        socket.sendto("192.168.8.101", 1884, &accel_data, sizeof(AccelerometerData_t));
        printf("Done sending data\r\n");
    }
}

// Receive data from the server
void receiveUDP() {
    // Allocate 2K of data
    void* data = malloc(2048);
    while (1) {
        // recvfrom blocks until there is data
        // nsapi_size_or_error_t size = socket.recvfrom(NULL, data, 2048);
        // if (size < 0) {
        //     // printf("Error while receiving data from UDP socket (%d)\r\n", size);
        //     continue; // should I restart here?
        // }

        // printf("Received %d bytes from UDP socket\r\n", size);
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

    printf("DSA2017 Data Gathering (accelerometer)\r\n");

    // Blink the LED until connectivity is achieved
    connectivityTicker.attach(&blink_led, 0.5f);

    // Connect to the network
    while ((network = easy_connect(true)) == NULL) {
        printf("Could not connect to the network...\r\n");
        wait_ms(1000);
    }

    mac_address = string(network->get_mac_address());

    // Open a UDP socket
    nsapi_error_t rt = socket.open(network);
    if (rt != NSAPI_ERROR_OK) {
        printf("Could not open UDP Socket (%d)\r\n", rt);
        NVIC_SystemReset(); // Restart system
        return 1;
    }

    socket.set_blocking(false);

    connectivityTicker.detach(); // stop blinking when connected
    led = 1; // and put the LED on indefinitely

    // Schedule a data-gathering thread
    Thread dataThread(osPriorityNormal, 16 * 1024);
    dataThread.start(&gatherAccelerometerData);

    // Schedule a UDP receive thread
    Thread recvThread;
    recvThread.start(&receiveUDP);

    wait(osWaitForever);
}

#endif
