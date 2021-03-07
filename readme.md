# Simple serial2mqtt client library/application

This is a client Arduino application/library for [Lieven](https://vortex314.blogspot.com/)'s [serial2mqtt](https://github.com/vortex314/serial2mqtt) application which connects microcontrollers to an MQTT broker over serial links.

## Table of Contents

1. [About the Project](#about-the-project)
1. [Project Status](#project-status)
1. [Getting Started](#getting-started)
	1. [Dependencies](#dependencies)
	1. [Building](#building)
	1. [Installation](#installation)
	1. [Usage](#usage)
1. [Release Process](#release-process)
	1. [Versioning](#versioning)
	1. [Payload](#payload)
1. [How to Get Help](#how-to-get-help)
1. [Further Reading](#further-reading)
1. [Contributing](#contributing)
1. [License](#license)
1. [Authors](#authors)
1. [Acknowledgements](#acknowledgements)

# About the Project

This is a client Arduino application/library for [Lieven](https://vortex314.blogspot.com/)'s [serial2mqtt](https://github.com/vortex314/serial2mqtt) application which connects microcontrollers to an MQTT broker over serial links.

Lieven has written a library for this, but it is for use in PlatformIO, much more comprehensive and part of a larger project.

This is a single purpose application/library to allow an Arduino to function as a basic I/O expander expressely for the purpose of home automation, connected directly to a Raspberry Pi over USB. It will compile and run easily on basic boards such as Arduino Uno and other AVR boards. As it is designed for home automation, you can configure some independence from the MQTT broker directly, mostly to make light switches work in the event of a broker/connectivity issue.

**[Back to top](#table-of-contents)**

# Project Status

This project was written specifically to help with a custom home automation project and is therefore sparsely documented. However should you wish to make use of it, please feel free.

**[Back to top](#table-of-contents)**

# Getting Started

## Dependencies

There are purposefully no dependencies to make it trivial to install and build, future versions may move to using ArduinoJSON.

## Getting the Source

This project is [hosted on GitHub](https://github.com/ncmreynolds/serial2mqtt). You can clone this project directly using this command:

## Building

There is one example included with the Arduino library, ioExpander.ino and this provides control of the Arduino pins over MQTT for home automation purposes.

## Installation

From the project page, select 'Code' -> 'Download ZIP' and save the file somewhere appropriate.

You can then install in the Arduino IDE by choosing 'Sketch' -> 'Include Library' -> 'Add .ZIP Library...'.

## Usage

After installation, the example will be available under "File" -> "Examples" -> "serial2mqtt" -> "ioExpander".

To make effective use of the example you will need to edit two lines in the ioExpander sketch.

```
48 const String serial2mqttHostname = "raspberrypi";  //The hostname of the linux host where serial2mqtt is running
49 const String serial2mqttUsbDevice = "ACM0";        //The serial device (eg. /dev/TTYUSB0 or /dev/ttyACM0) that the microcontroller serial port appears as, less the 'tty'
```

In line 48 you *must* set the hostname of the Linux system the Arduino is connected to. The linux daemon uses the hostname as part of the MQTT topics it services.

In line 49 you *must* set serial device the Arduino "connects as", which is usually something like /dev/TTYUSB0 or /dev/ttyACM0 but stripped of the leading /dev/tty. So for example /dev/ttyACM0 becomes just ACM0. This will vary depending on the board/system but is part of the MQTT topics the linux daemon services and must be set to match.

Change these to reflect your system and upload the sketch to the Arduino. The library in principle supports different serial ports than 'Serial', see lines 151 and 155 should you wish to change it to 'Serial2' etc., but as the expected use case is for USB connected Arduinos this is unlikely to be necessary.

On the Linux system you connect the Arduino board to you will need to configure serial2mqtt.json for 'JSON object' format. A working configuration might look like...

```
{
    "mqtt": {
        "connection": "tcp://localhost:1883"
    },
    "serial": {
        "baudrate": 115200,
        "ports": [
        "/dev/ttyACM0"
        ],
    "protocol":"jsonObject"
    },
    "log" : {
        "protocol":false,
        "debug":true,
        "useColors":false,
        "mqtt":false,
        "program":true
    }
}
```

Please see the [serial2mqtt](https://github.com/vortex314/serial2mqtt) documentation for how to install and configure the linux daemon. On the Raspberry Pi, the prebuilt ARM binary from

Once the Arduino is flashed and connected to the linux system running the daemon, you can control pins on the Arduino by sending messages to topics on the MQTT server to which the linux daemon is a subscriber. Broadly these are...

```
dst/<HOSTNAME>.<USB device>/pinMode  - Publish messages here to set the 'mode' of each pin in the format <pin number>,<pin mode> where pin mode is INPUT/INPUT_PULLUP/OUTPUT
dst/<HOSTNAME>.<USB device>/pinState - Publish messages here to set the 'state' of each ouput pin in the format <pin number>,<pin mode> where pin state is HIGH/LOW/TOGGLE or 1-254 for PWM output. It does not check a pin can do PWM.
dst/<HOSTNAME>.<USB device>/pinSwitch - Publish messages here to set the 'switch' relationship of a pin in the format <input pin number>,<output pin number>. To delete the relationship set the output pin to 'none'.
```

...note that the pins used for the Arduino hardware serial port, usually 0 and 1 are not available, for obvious reasons. A real world example of configuring pin 6 as an output and then switching it high could be...

```
mosquitto_pub -t dst/raspberrypi.ACM0/pinMode -m 6,OUTPUT
mosquitto_pub -t dst/raspberrypi.ACM0/pinState -m 6,HIGH
```

The example can also associate an input pin with an output pin and work independently of the MQTT server. This is designed for use in home automation and specifically designed so light switches etc. continue to function should the MQTT server or linux daemon fail.

A real world example of configuring pin 6 as an output and then configuring pin 7 that it toggles pin 6 could be...

```
mosquitto_pub -t dst/raspberrypi.ACM0/pinMode -m 6,OUTPUT
mosquitto_pub -t dst/raspberrypi.ACM0/pinState -m 6,LOW
mosquitto_pub -t dst/raspberrypi.ACM0/pinMode -m 7,INPUT_PULLUP
mosquitto_pub -t dst/raspberrypi.ACM0/pinSwitch -m 7,6
```

All this configuration is stored in the EEPROM of the Arduino, again for resilience in the case of a restart or power loss. The *state* of pins is not stored, to save wear on the EEPROM, only their configuration as input or output and whether one pin toggles anopther.

The Arduino also publishes to MQTT in a different topic.

```
src/<HOSTNAME>.<USB device>/pinMode  - On startup, once online the microcontroller informs MQTT of the 'mode' of each pin in the format <pin number>,<pin mode> where pin mode is INPUT/INPUT_PULLUP/OUTPUT
src/<HOSTNAME>.<USB device>/pinState - Once online, on state change the microcontroller informs MQTT of the 'state' of each input pin in the format <pin number>,<pin mode> where pin state is HIGH/LOW
src/<HOSTNAME>.<USB device>/pinSwitch - On startup, once online the microcontroller informs MQTT of the 'switch' relationship of each pin in the format <input pin number>,<output pin number> where such a relationship exists
```

So if you have set pin 7 as an input with pullup, then when its state changes you will receive messages on a topic. A real world example of this might be...

```
mosquitto_sub -t src/raspberrypi.ACM0/pinState
7,LOW
7,HIGH
```

If a pin is set to toggle another pin, you will see state changes for both pins.

**[Back to top](#table-of-contents)**

# Release Process

## Versioning

This project uses [Semantic Versioning](http://semver.org/) for Arduino library compatibility.

## Payload

**[Back to top](#table-of-contents)**

# How to Get Help

For help, please contact [Nick Reynolds](https://github.com/ncmreynolds) on serial2mqtt@arcanium.london.

# Contributing

Should you wish to contribute, please contact [Nick Reynolds](https://github.com/ncmreynolds) on serial2mqtt@arcanium.london.

**[Back to top](#table-of-contents)**

# Further Reading

This project entirely relies on [serial2mqtt](https://github.com/vortex314/serial2mqtt) by [Lieven](https://vortex314.blogspot.com/) and the assumptions it makes about MQTT topics. See this project for further unformation.

**[Back to top](#table-of-contents)**

# License

Copyright (c) 2020 Nick Reynolds.

This project is licensed under the GNU GPL License - see [LICENSE.md](LICENSE.md) file for details.

**[Back to top](#table-of-contents)**

# Authors

* **[Nick Reynolds](https://github.com/ncmreynolds)**

**[Back to top](#table-of-contents)**

# Acknowledgments

This project entirely relies on [serial2mqtt](https://github.com/vortex314/serial2mqtt) by [Lieven](https://vortex314.blogspot.com/) and was written to assist [Daniel](https://github.com/banier1) with a home automation project.

**[Back to top](#table-of-contents)**