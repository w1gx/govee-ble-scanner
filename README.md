# govee-ble-scanner
![GitHub last commit](https://img.shields.io/github/last-commit/w1gx/govee-ble-scanner?style=for-the-badge) ![Top language](https://img.shields.io/github/languages/top/w1gx/govee-ble-scanner?style=for-the-badge) ![Language count](https://img.shields.io/github/languages/count/w1gx/govee-ble-scanner?style=for-the-badge) ![Issues](https://img.shields.io/github/issues/w1gx/govee-ble-scanner?style=for-the-badge) ![GitHub release](https://img.shields.io/github/v/release/w1gx/govee-ble-scanner?style=for-the-badge)
>A BLE scanner for Govee H5074 temperature sensors.

![terminalscreen](./img/terminal-output.png)


## General info
The Govee BLE scanner uses a HCI connection to scan for packets of type HCI_EVENT_PKT (04) with event types of EVT_LE_META_EVENT and extracts all advertising data from them.
It then looks for AD type identifiers of 0xFF (manufacturer data), and within those for the company identifier 0x88EC, which contain the encoded temperature and humidity.
For more detail please refer to the wiki and the Bluetooth Core Specification at https://www.bluetooth.com/specifications/bluetooth-core-specification/.

## Technology

The Govee BLE scanner is written in C++ and tested on Raspberry Pis. Besides the standard libraries, it requires libbluetooth.

It has two main classes: a BLEPacket, which stores all packet information and BLEScan, the scanner that looks for HCI_EVENT packets.

## Setup
To run the scanner, do the following:

	sudo apt-get install bluetooth bluez libbluetooth-dev
	make release
	sudo ./build/release/goveeBLE

Optionally the doxygen documentation can be created with

	make doc		


## Status
This project served as a proof of concept and is probably finished in its current stage. The next steps are to send the data to InfluxDB and an MQTT broker, but that will be a separate project.
