/*
 * serial2mqttIOexpander
 * 
 * This sketch is a basic functional example of using serial2mqtt (https://github.com/vortex314/serial2mqtt) to control the pins on an AVR Arduino with MQTT to act as an I/O expander.
 * It uses the microcontroller EEPROM to store pin cofiguration between restarts so you need not rely on retained MQTT messages for this.
 * 
 * It can also tie the state of an input pin to an output pin, toggling the state when the input pin changes.
 * This is to replicate basic 'light switch' functionality that does not rely on a working MQTT server.
 * 
 * This was done as a proof-of-concept for a home automation project based around a Raspberry Pi and the serial2mqtt linux application.
 * 
 * The developer of serial2mqtt does publish a microcontroller library but it is generic and dependent on PlatfomIO and other dependencies so I wrote my own that can be easily installed in Arduino IDE.
 * This library does not do proper serialisation/deserialisation of JSON as it is a very specific use case where simple text searches suffice.
 * 
 * serial2mqtt assumes a very specific structure for MQTT topics and it is best to stick to that as it subscribes to the MQTT server accordingly. Do not use a leading /
 * 
 * src/<HOSTNAME>.<serial device less the 'tty'>/XXX for topics the Arduino publishes to
 * dst/<HOSTNAME>.<serial device less the 'tty'>/XXX for topics the Arduino subscribes to
 * 
 * serial2mqtt automatically subscribes to dst/<HOSTNAME>.<serial device less the 'tty'>/# on the MQTT server.
 * 
 * Beware, should you use topics that do not start in 'src/' or 'dst/' it can cause a segmentation fault in the serial2mqtt program on the linux host.
 * 
 * On a raspberry Pi running serial2mqtt these topics might look like...
 * 
 * src/raspberrypi.ACM0/XXX
 * dst/raspberrypi.ACM0/XXX
 * 
 * This sketch uses six topics, three in each 'direction'.
 * 
 * src/<HOSTNAME>.<USB device>/pinMode  - On startup, once online the microcontroller informs MQTT of the 'mode' of each pin in the format <pin number>,<pin mode> where pin mode is INPUT/INPUT_PULLUP/OUTPUT
 * dst/<HOSTNAME>.<USB device>/pinMode  - Publish messages here to set the 'mode' of each pin in the format <pin number>,<pin mode> where pin mode is INPUT/INPUT_PULLUP/OUTPUT
 * src/<HOSTNAME>.<USB device>/pinState - Once online, on state change the microcontroller informs MQTT of the 'state' of each input pin in the format <pin number>,<pin mode> where pin state is HIGH/LOW
 * dst/<HOSTNAME>.<USB device>/pinState - Publish messages here to set the 'state' of each ouput pin in the format <pin number>,<pin mode> where pin state is HIGH/LOW or 1-254 for PWM output. It does not check a pin can do PWM.
 * src/<HOSTNAME>.<USB device>/pinSwitch - On startup, once online the microcontroller informs MQTT of the 'switch' relationship of each pin in the format <input pin number>,<output pin number> where such a relationship exists
 * dst/<HOSTNAME>.<USB device>/pinSwitch - Publish messages here to set the 'switch' relationship of a pin in the format <input pin number>,<output pin number>. To delete the relationship set the output pin to 'none'.
 * 
 * In addition, the serial2mqtt library uses this topic for sending loopback messages to ensure it able to communicate with the MQTT server
 * 
 * src/<HOSTNAME>.<USB device>/loopback
 * 
 * To set appropriate topics, edit the two lines immediately below this header. Strings are used to keep the code simple to read, at some cost to efficiency.
 * 
 * Nick Reynolds 15/12/2020
 * 
 */

const String serial2mqttHostname = "raspberrypi";  //The hostname of the linux host where serial2mqtt is running
const String serial2mqttUsbDevice = "ACM0";        //The serial device (eg. /dev/TTYUSB0 or /dev/ttyACM0) that the microcontroller serial port appears as, less the 'tty'

/*
 * Topics for managing pins are also built out of these elements
 */

const String serial2mqttPubTopicPrefix = "src/";     //The topic suffix for published messages
const String serial2mqttSubTopicPrefix = "dst/";     //The topic suffix for subscriptions
const String serial2mqttModeTopic = "pinMode";   //The topic suffic used for configuring pins
const String serial2mqttStateSuffix = "pinState";  //The topic suffic used for changing/reporting pin state
const String serial2mqttSwitchSuffix = "pinSwitch"; //The topic suffic used for configuring pins as switches
const String serial2mqttLoopbackSuffix = "loopback";//The topic suffic used for MQTT loopback

/*
 * This next section sets some defaults for common AVR Arduino boards.
 * 
 * This ensures the right number of pins are set and also prevents you accidentially changing the mode/state of the serial port, which will break communication with serial2mqtt.
 * Should you use a non-default serial port on a board with more than one port you will need to edit this.
 */

#if defined (ARDUINO_AVR_UNO)
const uint8_t numberOfPins = 20;
const uint8_t rxPin = 0;
const uint8_t txPin = 1;
#elif defined(ARDUINO_AVR_MINI)
const uint8_t numberOfPins = 20;
const uint8_t rxPin = 0;
const uint8_t txPin = 1;
#elif defined(ARDUINO_AVR_NANO)
const uint8_t numberOfPins = 22;
const uint8_t rxPin = 0;
const uint8_t txPin = 1;
#elif defined(ARDUINO_AVR_MICRO)
const uint8_t numberOfPins = 20;
const uint8_t rxPin = 0;
const uint8_t txPin = 1;
#elif defined(ARDUINO_AVR_PRO)
const uint8_t numberOfPins = 20;
const uint8_t rxPin = 0;
const uint8_t txPin = 1;
#elif defined(ARDUINO_AVR_MEGA2560)
const uint8_t numberOfPins = 70;
const uint8_t rxPin = 0;
const uint8_t txPin = 1;
#elif defined(ARDUINO_AVR_MEGA2560)
const uint8_t numberOfPins = 70;
const uint8_t rxPin = 0;
const uint8_t txPin = 1;
#elif defined(ARDUINO_AVR_MEGA)
const uint8_t numberOfPins = 70;
const uint8_t rxPin = 0;
const uint8_t txPin = 1;
#elif defined(ARDUINO_AVR_ADK)
const uint8_t numberOfPins = 70;
const uint8_t rxPin = 0;
const uint8_t txPin = 1;
#elif defined(ARDUINO_AVR_LEONARDO)
const uint8_t numberOfPins = 64;
const uint8_t rxPin = 0;
const uint8_t txPin = 1;
#else
const uint8_t numberOfPins = 8;
const uint8_t rxPin = 0;
const uint8_t txPin = 1;
#endif

uint8_t pinStatus[numberOfPins];  //An array to track how the pins are configured and their current state, used for operation and saving to EEPROM
uint8_t pinSwitch[numberOfPins];  //An array to track any 'switch' pins that are configured, used for operation and saving to EEPROM
const uint8_t defaultPinConfiguration = INPUT;  //By default, all pins start as an input
const uint8_t defaultPinSwitch = 0xff;  //By default no input pins work as 'switches'

const uint8_t pinIsInput = 0;
const uint8_t pinIsOutput = 1;
const uint8_t pinStateHigh = 2;

#include <EEPROM.h>       //EEPROM is used to store pin configuration between restarts

#include <serial2mqtt.h>  //Include the serial2mqtt library
serial2mqtt mqtt;         //Create the serial2mqtt object

/*
 * Create the various topics by appending Strings
 */

String loopbackPubTopic =  serial2mqttPubTopicPrefix + serial2mqttHostname + "." + serial2mqttUsbDevice + "/" + serial2mqttLoopbackSuffix;

String pinModePubTopic =   serial2mqttPubTopicPrefix + serial2mqttHostname + "." + serial2mqttUsbDevice + "/" + serial2mqttModeTopic;
String pinStatePubTopic =  serial2mqttPubTopicPrefix + serial2mqttHostname + "." + serial2mqttUsbDevice + "/" + serial2mqttStateSuffix;
String pinSwitchPubTopic = serial2mqttPubTopicPrefix + serial2mqttHostname + "." + serial2mqttUsbDevice + "/" + serial2mqttSwitchSuffix;

String pinModeSubTopic =   serial2mqttSubTopicPrefix + serial2mqttHostname + "." + serial2mqttUsbDevice + "/" + serial2mqttModeTopic;
String pinStateSubTopic =  serial2mqttSubTopicPrefix + serial2mqttHostname + "." + serial2mqttUsbDevice + "/" + serial2mqttStateSuffix;
String pinSwitchSubTopic = serial2mqttSubTopicPrefix + serial2mqttHostname + "." + serial2mqttUsbDevice + "/" + serial2mqttSwitchSuffix;

String microcontrollerSubTopic = serial2mqttSubTopicPrefix + serial2mqttHostname + "." + serial2mqttUsbDevice + "/microcontroller";

bool mqttOnline = false;  //The microcontroller starts 'offline' and subscribes to the three topics above when it goes online
bool configurationChanged = false;
uint32_t lastConfigurationCheck = 0;

void setup()
{
  Serial.begin(115200);                  //Set to match serial2mqtt on the Linux machine
  Serial.setTimeout(100);                //Can be short as it's a machine sending the serial strings
  mqtt.useJSONobjects();                 //Communicate with the Linux machine using JSON objects
  //mqtt.useJSONarrays();                 //Communicate with the Linux machine using JSON arrays
  mqtt.begin(Serial);                    //Tell serial2mqtt which serial device to use
  //mqtt.debug(true);                      //Send more info to the logging facility of serial2mqtt
  mqtt.loopbackTopic(loopbackPubTopic);  //You SHOULD supply a loopback topic that matches how this device appears to serial2mqtt, otherwise it will not know if it is online
  
  #ifdef LED_BUILTIN
  mqtt.statusLed(LED_BUILTIN);           //Define a pin used to show online status, ONLY if the board includes an onboard LED
  #endif

  if(configurationValid())               //Checks the CRC on the configuration in EEPROM
  {
    readConfiguration();                 //Copies the configuration into pinStatus[] and pinSwitch[]
    //writeConfiguration();                //Writes pinStatus[] and pinSwitch[] to EEPROM
  }
  else
  {
    mqtt.println(F("Configuration from EEPROM is invalid, wiping!"));
    wipeConfiguration();                 //Wipes pinStatus[] and pinSwitch[], setting everything to just an INPUT
    writeConfiguration();                //Writes pinStatus[] and pinSwitch[] to EEPROM
  }
  configurePins();                       //Configures the physical pins based off the contents of pinStatus[]
}

void  loop()
{
  mqtt.housekeeping();      //This reads incoming MQTT data and checks for online status it MUST run regularly, do not use delay in sketches
  if(mqttOnline != mqtt.online()) //Online state has changed
  {
    if(mqttOnline)  //MQTT has just gone offline, log it in a vain hope something sees it
    {
      mqtt.println(F("Gone onffine"));
      mqttOnline = false;
    }
    else  //MQTT has just gone online, subscribe to the relevant topics
    {
      mqtt.println(F("Gone online, subscribing to topics"));
      mqtt.subscribe(pinModeSubTopic);          //This topic handles the pin mode
      mqtt.subscribe(pinStateSubTopic);         //This topic handles the pin state
      mqtt.subscribe(pinSwitchSubTopic);        //This topic handles switch configuration
      mqtt.subscribe(microcontrollerSubTopic);  //This topic handles commands for the whole microcontroller, reset, wipe, save config etc.
      mqttOnline = true;
    }
  }
  if(mqtt.messageWaiting()) //Check for incoming messages
  {
    if(mqtt.topic().equals(microcontrollerSubTopic))
    {
      if(mqtt.message().equals("RESTART"))  //Restart the microcontroller
      {
        mqtt.println(F("Restarting"));
        delay(1000);
        asm volatile ("  jmp 0");
      }
      else if(mqtt.message().equals("WIPE"))  //Wipe the configuration and restart
      {
        wipeConfiguration();                 //Wipes pinStatus[] and pinSwitch[], setting everything to just an INPUT
        writeConfiguration();                //Writes pinStatus[] and pinSwitch[] to EEPROM
        mqtt.println(F("Restarting"));
        delay(1000);
        asm volatile ("  jmp 0");
      }
      else if(mqtt.message().equals("WRITE"))  //Save the configuration
      {
        writeConfiguration();                //Writes pinStatus[] and pinSwitch[] to EEPROM
      }
    }
    else
    {
      uint8_t pin = extractIntegerBeforeDelimiter(mqtt.message(),',');
      if(usablePin(pin))
      {
        String pinCommand = extractStringAfterDelimiter(mqtt.message(),',');
        if(mqtt.topic().equals(pinModeSubTopic))        //MQTT has asked the microcontroller to change the mode of a pin
        {
          if(pinCommand.equals("INPUT"))
          {
            pinMode(pin,INPUT);
            pinStatus[pin] = pinIsInput;
            configurationChanged = true;
          }
          else if(pinCommand.equals("INPUT_PULLUP"))
          {
            pinMode(pin,INPUT_PULLUP);
            pinStatus[pin] = pinIsInput & pinStateHigh;
            configurationChanged = true;
          }
          else if(pinCommand.equals("OUTPUT"))
          {
            pinMode(pin,OUTPUT);
            pinStatus[pin] = pinIsOutput;
            changeState(pin,false);
            configurationChanged = true;
          }
          if(configurationChanged)
          {
            mqtt.print(F("Changed mode of pin "));
            mqtt.print(pin);
            mqtt.print(F(" to "));
            mqtt.println(pinCommand);
          }
          else
          {
            mqtt.print(F("Cannot change mode of pin "));
            mqtt.print(pin);
            mqtt.print(F(" to "));
            mqtt.println(pinCommand);
          }
        }
        else if(mqtt.topic().equals(pinStateSubTopic))  //MQTT has asked the microcontroller to change the state of a pin
        {
          if(outputPin(pin))
          {
            if(pinCommand.equals("HIGH"))
            {
              changeState(pin,true);
            }
            else if(pinCommand.equals("LOW"))
            {
              changeState(pin,false);
            }
            else if(pinCommand.equals("TOGGLE"))
            {
              toggleState(pin);
            }
            else
            {
              uint8_t pwmLevel = extractIntegerAfterDelimiter(mqtt.message(),',');
              if(pwmLevel >0)
              {
                changeState(pin,pwmLevel);
              }
            }
          }
          else
          {
            mqtt.print(F("Can't change state of input pin "));
            mqtt.println(pin);
          }
        }
        else if(mqtt.topic().equals(pinSwitchSubTopic))  //MQTT has asked the microcontroller to configure an input as a switch to another pin
        {
          uint8_t switchedPin = extractIntegerAfterDelimiter(mqtt.message(),',');
          if(outputPin(switchedPin))
          {
            mqtt.print(F("Configuring switch on pin "));
            mqtt.print(pin);
            mqtt.print(F(" for pin "));
            mqtt.println(switchedPin);
            pinSwitch[pin] = switchedPin;
            configurationChanged = true;
          }
          else
          {
            mqtt.print(F("Cannot configure switch on pin "));
            mqtt.print(pin);
            mqtt.print(F(" for pin "));
            mqtt.println(switchedPin);
          }
        }
        else                                            //MQTT has asked for something else
        {
          mqtt.print(F("Unknown command "));
          mqtt.print(mqtt.topic());
          mqtt.print(' ');
          mqtt.println(mqtt.message());
        }
      }
      else
      {
        mqtt.print(F("Attempt to configure unusable pin "));
        mqtt.print(mqtt.topic());
        mqtt.print(' ');
        mqtt.println(mqtt.message());
      }
    }
    mqtt.markMessageRead();
  }
  for(uint8_t pin = 0;pin < numberOfPins;pin++)    //Check all the pins for a change of state
  {
    if(inputPin(pin)) //Only check input pins
    {
      bool pinState = (digitalRead(pin) == HIGH);
      bool oldPinState = ((pinStatus[pin] & pinStateHigh) == pinStateHigh);
      if(pinState != oldPinState) //Pin state has changed
      {
        if(pinState)
        {
          pinStatus[pin] = pinStateHigh;         //Save the current status
          mqtt.publish(pinStatePubTopic,String(pin) + ",HIGH");   //Publish the change to MQTT
        }
        else
        {
          pinStatus[pin] = 0;                //Save the current status
          mqtt.publish(pinStatePubTopic,String(pin) + ",LOW");   //Publish the change to MQTT
        }
        if(pinSwitch[pin] != 0xff)                 //Check if the pin is working as a switch
        {
          if(pinStatus[pinSwitch[pin]] & pinStateHigh)
          {
            changeState(pinSwitch[pin],false);
          }
          else
          {
            changeState(pinSwitch[pin],true);
          }
        }
      }
    }
  }
  if(millis() - lastConfigurationCheck > 6e4) //Only save the config every 60s maximum
  {
    lastConfigurationCheck = millis();
    if(configurationChanged)
    {
      writeConfiguration();
      configurationChanged = false;
    }
  }
}

void changeState(uint8_t pin,bool state)
{
  if(state == true && ((pinStatus[pin] & pinStateHigh)==0x00))
  {
    digitalWrite(pin,HIGH);
    pinStatus[pin] = pinIsOutput | pinStateHigh;
    mqtt.publish(pinStatePubTopic,String(pin) + ",HIGH");
    mqtt.print(F("Changing state of pin "));
    mqtt.print(pin);
    mqtt.println(F(" to HIGH"));
  }
  else if(state == false && (pinStatus[pin] & pinStateHigh))
  {
    digitalWrite(pin,LOW);
    pinStatus[pin] = pinIsOutput;
    mqtt.publish(pinStatePubTopic,String(pin) + ",LOW");
    mqtt.print(F("Changing state of pin "));
    mqtt.print(pin);
    mqtt.println(F(" to LOW"));
  }
}

void changeState(uint8_t pin,uint8_t pwmLevel)
{
  analogWrite(pin,pwmLevel);
  mqtt.publish(pinStatePubTopic,String(pin) + "," + String(pwmLevel));
  mqtt.print(F("Changing state of pin "));
  mqtt.print(pin);
  mqtt.print(F(" to PWM level "));
  mqtt.print(pwmLevel);
  mqtt.println(F("/255"));
}

void toggleState(uint8_t pin)
{
  if((pinStatus[pin] & pinStateHigh)==0x00)
  {
    digitalWrite(pin,HIGH);
    pinStatus[pin] = pinIsOutput | pinStateHigh;
    mqtt.publish(pinStatePubTopic,String(pin) + ",HIGH");
    mqtt.print(F("Changing state of pin "));
    mqtt.print(pin);
    mqtt.println(F(" to HIGH"));
  }
  else if(pinStatus[pin] & pinStateHigh)
  {
    digitalWrite(pin,LOW);
    pinStatus[pin] = pinIsOutput;
    mqtt.publish(pinStatePubTopic,String(pin) + ",LOW");
    mqtt.print(F("Changing state of pin "));
    mqtt.print(pin);
    mqtt.println(F(" to LOW"));
  }
}


uint8_t extractIntegerBeforeDelimiter(String text,char delimiter)
{
  uint16_t delimiterPosition = mqtt.message().indexOf(delimiter);
  return(text.substring(0,delimiter).toInt());
}
String extractStringAfterDelimiter(String text,char delimiter)
{
  uint16_t delimiterPosition = mqtt.message().indexOf(delimiter);
  return(text.substring(delimiterPosition+1,text.length()));
}
uint8_t extractIntegerAfterDelimiter(String text,char delimiter)
{
  uint16_t delimiterPosition = mqtt.message().indexOf(delimiter);
  return(text.substring(delimiterPosition+1,text.length()).toInt());
}
bool configurationValid()
{
  uint32_t calculatedCrc = calculateStoredCrc(0,numberOfPins*2);
  uint32_t storedCrc;
  EEPROM.get(numberOfPins*2,storedCrc); //EEPROM.get handles packing ANY basic data type into the EEPROM for us
  if(calculatedCrc == storedCrc)
  {
    return(true);
  }
  else
  {
    return(false);
  }
}
void readConfiguration()
{
  mqtt.println(F("Reading configuration from EEPROM"));
  for(uint8_t pin = 0;pin < numberOfPins;pin++)
  {
    pinStatus[pin] = EEPROM[pin]; //Straight read of the EEPROM
    pinSwitch[pin] = EEPROM[pin + numberOfPins]; //Straight read of the EEPROM
  }
}
void wipeConfiguration()
{
  mqtt.println(F("Wiping pin configuration"));
  for(uint8_t pin = 0;pin < numberOfPins;pin++)
  {
    if(usablePin(pin))
    {
      pinMode(pin,INPUT_PULLUP);             //INPUT_PULLUP is most stable for unconnected pins
      pinStatus[pin] = pinStateHigh;         //Save the current status
      pinSwitch[pin] = 0xff;                 //Set as no pinSwitch
    }
  }  
}
void writeConfiguration()
{
  mqtt.print(F("Writing configuration to EEPROM CRC is "));
  for(uint8_t pin = 0;pin < numberOfPins;pin++)
  {
    EEPROM.update(pin, pinStatus[pin]);  //EEPROM.update should only change the stored value if it has changed
    EEPROM.update(pin + numberOfPins, pinSwitch[pin]);
  }
  uint32_t calculatedCrc = calculateStoredCrc(0,numberOfPins*2);
  uint32_t storedCrc;
  EEPROM.get(numberOfPins*2,storedCrc); //EEPROM.get handles unpacking ANY basic data type into the EEPROM for us
  if(storedCrc != calculatedCrc)      //Data in EEPROM has changed, therefore the CRC must be updated
  {
    EEPROM.put(numberOfPins*2,calculatedCrc); //EEPROM.put handles packing and storing any basic data type into the EEPROM for us
  }
  mqtt.println(calculatedCrc);
}

bool inputPin(uint8_t pin)  //Returns true if the pin is configured as an input, false otherwise
{
  if(usablePin(pin) && not(pinStatus[pin] & pinIsOutput))
  {
    return(true);
  }
  return(false);
}

bool outputPin(uint8_t pin) //Returns true if the pin is configured as an output, false otherwise
{
  if(usablePin(pin) && (pinStatus[pin] & pinIsOutput))
  {
    return(true);
  }
  return(false);
}

bool usablePin(uint8_t pin) //Returns true if the pin is configurable by MQTT, false otherwise
{
  #ifdef LED_BUILTIN
  if(pin != rxPin && pin != txPin && pin != LED_BUILTIN)
  #endif
  if(pin != rxPin && pin != txPin)
  {
    return(true);
  }
  return(false);
}
void configurePins()
{
  mqtt.println(F("Configuring pins..."));
  for(uint8_t pin = 0;pin < numberOfPins;pin++)
  {
    if(usablePin(pin))
    {
      if(pinStatus[pin] & pinIsOutput)
      {
        pinMode(pin,OUTPUT);
        mqtt.publish(pinModePubTopic,String(pin) + ",OUTPUT");
        if(pinStatus[pin] & pinStateHigh)
        {
          digitalWrite(pin,HIGH);
          mqtt.publish(pinStatePubTopic,String(pin) + ",HIGH");
          mqtt.print(F("Pin "));
          mqtt.print(pin);
          mqtt.print(F(" OUTPUT HIGH"));
        }
        else
        {
          digitalWrite(pin,LOW);
          mqtt.publish(pinStatePubTopic,String(pin) + ",LOW");
          mqtt.print(F("Pin "));
          mqtt.print(pin);
          mqtt.print(F(" OUTPUT LOW"));
        }
      }
      else
      {
        if(pinStatus[pin] & pinStateHigh)
        {
          pinMode(pin,INPUT_PULLUP);
          mqtt.publish(pinModePubTopic,String(pin) + ",INPUT_PULLUP");
          mqtt.print(F("Pin "));
          mqtt.print(pin);
          mqtt.print(F(" INPUT_PULLUP"));
        }
        else
        {
          pinMode(pin,INPUT);
          mqtt.publish(pinModePubTopic,String(pin) + ",INPUT");
          mqtt.print(F("Pin "));
          mqtt.print(pin);
          mqtt.print(F(" INPUT"));
        }
      }
      if(pinSwitch[pin] != 0xff && not(pinStatus[pin] & pinIsOutput))
      {
        mqtt.print(F(" switches pin "));
        mqtt.println(pinSwitch[pin]);
        mqtt.publish(pinSwitchPubTopic,String(pin) + "," + pinSwitch[pin]);
      }
      else
      {
        mqtt.println(' ');
      }
    }
    else
    {
      mqtt.print(F("Pin "));
      mqtt.print(pin);
      mqtt.println(F(" unusable"));
    }
  }
}
/*
 * CRC calculation adapted from the CRC method published by Christopher Andrews on https://www.arduino.cc/en/Tutorial/LibraryExamples/EEPROMCrc
 *
 *   "CRC algorithm generated by pycrc, MIT licence ( https://github.com/tpircher/pycrc ).
 *
 *   A CRC is a simple way of checking whether data has changed or become corrupted.
 *
 *   This example calculates a CRC value directly on the EEPROM values.
 *
 *   The purpose of this example is to highlight how the EEPROM object can be used just like an array."
 * 
 */
uint32_t calculateStoredCrc(uint16_t start,uint16_t length)
{
  uint32_t crc = ~0L;
  uint32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };
  for (uint16_t index = start ; index < start + length ; index++)
  {
    crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return (crc);
}
