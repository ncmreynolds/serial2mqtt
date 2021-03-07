# Simple serial2mqtt client library/application

This is a client Arduino application/library for [Lieven](https://vortex314.blogspot.com/)'s [serial2mqtt](https://github.com/vortex314/serial2mqtt) application which connected microcontrollers to an MQTT broker over serial links.

Lieven has written a library for this, but it is for use in PlatformIO, much more comprehensive and part of a larger project.

This is a single purpose application/library to allow an Arduino to function as a basic IO expander. It will compile and run easily on basic boards such as Arduino Uno and other AVR boards.

* What your project does
* Why people should consider using your project
* Link to project home page

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

Here you can provide more details about the project
* What features does your project provide?
* Short motivation for the project? (Don't be too long winded)
* Links to the project site

```
Show some example code to describe what your project does
Show some of your APIs
```

**[Back to top](#table-of-contents)**

# Project Status

Show the build status if you have a CI server:

[![Build Status](http://your-server:12345/job/badge/icon)](http://your-server:12345/job/http://your-server:12345/job/badge/icon/)

Describe the current release and any notes about the current state of the project. Examples: currently compiles on your host machine, but is not cross-compiling for ARM, APIs are not set, feature not implemented, etc.

**[Back to top](#table-of-contents)**

# Getting Started

This section should provide instructions for other developers to

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

## Dependencies

There are purposefully no dependencies, future versions may move to using ArduinoJSON.

## Getting the Source

This project is [hosted on GitHub](https://github.com/ncmreynolds/serial2mqtt). You can clone this project directly using this command:

## Building

There is one example included with the Arduino library, ioExpander.ino and this provides control of the Arduino pins over MQTT.

## Installation

From the project page, select 'Code' -> 'Download ZIP' and save the file somewhere appropriate.

You can then install in the Arduino IDE by choosing 'Sketch' -> 'Include Library' -> 'Add .ZIP Library...'.

## Usage

After installation, the example will be available under "File" -> "Examples" -> "serial2mqtt" -> "ioExpander".

To make effective use fo the example you will need to edit two lines in the ioExpander sektch.

```
48 const String serial2mqttHostname = "raspberrypi";  //The hostname of the linux host where serial2mqtt is running
49 const String serial2mqttUsbDevice = "ACM0";        //The serial device (eg. /dev/TTYUSB0 or /dev/ttyACM0) that the microcontroller serial port appears as, less the 'tty'
```

In line 48 you *must* set the hostname of the Linux system the Arduino is connected to. The linux daemon uses the hostname as part of the MQTT topics it services.

In line 49 you *must* set serial device the Arduino "connects as", which is usually something like /dev/TTYUSB0 or /dev/ttyACM0 but stripped of the leading /dev/tty. This will vary depending on the board/system but is part of the MQTT topics the linux daemon services and must be set to match.

Once configured, you can control pins on the Arduino by sending messages to topics on the MQTT server to which the linux daemon is a subscriber. Broadly these are...

* dst/<HOSTNAME>.<USB device>/pinMode  - Publish messages here to set the 'mode' of each pin in the format <pin number>,<pin mode> where pin mode is INPUT/INPUT_PULLUP/OUTPUT
* dst/<HOSTNAME>.<USB device>/pinState - Publish messages here to set the 'state' of each ouput pin in the format <pin number>,<pin mode> where pin state is HIGH/LOW or 1-254 for PWM output. It does not check a pin can do PWM.
* dst/<HOSTNAME>.<USB device>/pinSwitch - Publish messages here to set the 'switch' relationship of a pin in the format <input pin number>,<output pin number>. To delete the relationship set the output pin to 'none'.

* src/<HOSTNAME>.<USB device>/pinMode  - On startup, once online the microcontroller informs MQTT of the 'mode' of each pin in the format <pin number>,<pin mode> where pin mode is INPUT/INPUT_PULLUP/OUTPUT
* src/<HOSTNAME>.<USB device>/pinState - Once online, on state change the microcontroller informs MQTT of the 'state' of each input pin in the format <pin number>,<pin mode> where pin state is HIGH/LOW
* src/<HOSTNAME>.<USB device>/pinSwitch - On startup, once online the microcontroller informs MQTT of the 'switch' relationship of each pin in the format <input pin number>,<output pin number> where such a relationship exists


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