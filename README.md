# DSA-2017 Data Gathering Firmware

This is the firmware that is used to gather data during Data Science Africa 2017 fieldwork at 22 July.

## How to run

1. Take a NUCLEO-F401RE or NUCLEO-F411RE.
1. Take an ESP8266 module and connect to the board according to [this diagram](https://github.com/ARMmbed/dsa-2017/blob/1e797dfd75ebafd136f4849019b7b6c3105d2c3a/instructions.md#wiring).
1. Attach an accelerometer and a temperature sensor to the board.
1. Open ``mbed_app.json`` and configure your WiFi credentials.
1. Open ``main.cpp`` and configure the location of your MQTT server.
1. Build and flash the application to the board.

## What the application does

It gathers data and publishes it over MQTT to [a server](https://github.com/janjongboom/dsa2017-fieldwork-fw). The topic is `macaddress/sensor` (e.g. `60:01:94:1f:cf:f9/temperature`).

**Why MQTT?** During the workshops we used mbed Device Connector, a hosted service. To be able to run without an internet connection we switch to MQTT, which you can host yourself. This should increase the chance of successfully gathering data.
