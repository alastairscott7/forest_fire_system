# forest_fire_system

This is code from a hackathon hosted by HackTheBase.
The purpose of this project was to create a forest fire warning system.
The .ino file was run on a Feather M0 development board with LoRa radio to send data read by a temperature sensor through a gateway, up to a network server hosted by The Things Network.
The .py file is an application that utilizes an MQTT client library to communicate with The Things Network and supports uplink and downlink messages.
