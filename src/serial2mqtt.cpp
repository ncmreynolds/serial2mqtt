#ifndef serial2mqtt_cpp
#define serial2mqtt_cpp
#include <Arduino.h>
#include "serial2mqtt.h"
void serial2mqtt::begin(Stream &serialStream) //Pass a reference to the stream used for the serial port. Often this will be &Serial
{
	_serialStream = &serialStream;				//Get the serial stream reference
}

void serial2mqtt::loopbackTopic(String loopbackTopic)
{
	_loopbackTopic = String(loopbackTopic);		//Store the loopback topic
	_loopbackConfigured = true;					//Flag that the loopback topic is set
}

bool serial2mqtt::online()						//Is the MQTT server reachable? Tested with the loopback topic
{
	if(_loopbackConfigured)
	{
		return(_online);
	}
	else
	{
		return(true);
	}
}
bool serial2mqtt::messageWaiting()				//Is there a message waiting
{
	return(_messageWaiting);
}

String serial2mqtt::topic()						//Return the most recent topic as a String
{
	return(_mostRecentTopic);
}
    
String serial2mqtt::message()					//Return the most recent message as a String
{
	return(_mostRecentMessage);
}
bool serial2mqtt::messageIsNumber()				//Is the most recent message a number
{
	return(_isNumber(_mostRecentMessage));
}
uint32_t serial2mqtt::messageInt()				//Return the most recent message an int32_t
{
	return(_mostRecentMessage.toInt());
}
void serial2mqtt::useJSONobjects()
{
	_useJSONobjects = true;
}
void serial2mqtt::useJSONarrays()
{
	_useJSONobjects = false;
}
void serial2mqtt::markMessageRead()				//Mark the most recent message read
{
	_mostRecentTopic = "";
	_mostRecentMessage = "";
	_messageWaiting = false;
}

void serial2mqtt::subscribe(String topic)
{
	if(_debugEnabled)
	{
		_serialStream->print(F("Arduino:Subscribing to "));
		_serialStream->println(topic);
	}
	if(_useJSONobjects)
	{
		_serialStream->print(F("{ \"cmd\":\"MQTT-SUB\",\"topic\":\""));
		_serialStream->print(topic);
		_serialStream->println(F("\" }"));
	}
	else
	{
		_serialStream->print(F("[0,\""));
		_serialStream->print(topic);
		_serialStream->print(F("\"]\r\n"));
	}
}
void serial2mqtt::housekeeping()
{
	//Send loopbacks, if configured
	if(millis() - _lastSentLoopback > _loopbackInterval)
	{
		//Send subscribes to try and get back online
		if(_loopbackConfigured && _online == false)
		{
			subscribe(_loopbackTopic);
		}
		_lastSentLoopback = millis();
		if(_loopbackConfigured)
		{
			publish(_loopbackTopic,_loopbackSequenceNumber);	//Send a message to the loopback topic
		}
		else
		{
			//_serialStream->print(char(0));	//Send a null to keep serial2mqtt on Linux happy
			//_serialStream->println();	//Send a newline to keep serial2mqtt on Linux happy
			if(_useJSONobjects)
			{
				//_serialStream->println("{ \"cmd\":\"MQTT-CONN\",\"topic\":\"\" }");
				_serialStream->println("{ }");
			}
			else
			{
				_serialStream->println("{ 2, \"\" }");	//Send empty JSON serial2mqtt on Linux happy
			}
		}
	}
	//Check for loopback expiry
	if(_online == true && millis() - _lastReceivedLoopback > _loopbackInterval * 3)
	{
		_online = false;
		_flashFrequency = 250;
		if(_debugEnabled)
		{
			_serialStream->println(F("Arduino:Gone offline"));
		}
	}
	if(_statusLedEnabled)
	{
		if(millis() - _statusLedTimer > _flashFrequency)
		{
			_statusLedTimer = millis();
			if(_statusLedState)
			{
				digitalWrite(_statusLed,LOW);
				_statusLedState = false;
			}
			else
			{
				digitalWrite(_statusLed,HIGH);
				_statusLedState = true;
			}
		}
	}
	if(_serialStream->available())
	{
		String receivedString = _serialStream->readString();
		String receivedCommand;
		String receivedTopic;
		String message;
		uint32_t value;
		bool _validMessageReceived = false;
		if(_loopbackConfigured == false)
		{
			_lastReceivedLoopback = millis();	//Take incoming serial as proof of life
		}
		if(_validMessage(receivedString))
		{
			uint16_t commandStart = receivedString.indexOf("\"cmd\":\"") + 6;
			uint16_t commandEnd = receivedString.indexOf('"', commandStart + 1);
			receivedCommand = receivedString.substring(commandStart + 1, commandEnd);
			if(receivedCommand.equals("MQTT-PUB"))
			{
				_validMessageReceived = true;
				uint16_t topicStart = receivedString.indexOf("\"topic\":\"") + 8;
				uint16_t topicEnd = receivedString.indexOf('"', topicStart + 1);
				uint16_t messageStart = receivedString.indexOf("\"message\":\"", topicEnd + 1) + 10;
				uint16_t messageEnd = receivedString.indexOf('"', messageStart + 1);
				receivedTopic = receivedString.substring(topicStart + 1, topicEnd);
				message = receivedString.substring(messageStart + 1, messageEnd);
				if(_debugEnabled)
				{
					_serialStream->print(F("Arduino:MQTT-PUB received on topic "));
					_serialStream->print(receivedTopic);
				}
				if(_isNumber(message))
				{ 
					value = message.toInt();
					if(_debugEnabled)
					{
						_serialStream->print(F(" value "));
						_serialStream->println(value);
					}
				}
				else if(_debugEnabled)
				{
					_serialStream->print(F(" message "));
					_serialStream->println(message);
				}
			}
			else if(receivedCommand.equals("MQTT-CONN") && _debugEnabled)
			{
				_serialStream->println(F("Arduino:MQTT-CONN connected!"));
			}
			else if(_debugEnabled)
			{
				_serialStream->print(F("Arduino:Unknown command \""));
				_serialStream->print(receivedCommand);
				_serialStream->print(F("\" "));
				_serialStream->print(commandStart);
				_serialStream->print('-');
				_serialStream->print(commandEnd);
				_serialStream->print(F(" in "));
				_serialStream->println(receivedString);
			}
		}
		else if(_debugEnabled)
		{
			_serialStream->print(F("Arduino:invalid string "));
			_serialStream->println(receivedString);
		}
		//Handle online/offline
		if(_validMessageReceived)
		{
			if(_loopbackConfigured && receivedTopic.equals(_loopbackTopic))
			{
				_messageWaiting = false;             //Loopback messages never make it to the main sketch
				//if(value == _loopbackSequenceNumber) //The loopback sequence number MUST be the expected one
				{
					if(_debugEnabled)
					{
						_serialStream->print(F("Arduino:Expected loopback message "));
						_serialStream->print(value);
						_serialStream->println(F(" received"));
					}
					_lastReceivedLoopback = millis();  //Refresh the timeout
					_loopbackSequenceNumber++;         //Increment the sequence number
					if(_online == false)
					{
						_online = true;
						_flashFrequency = 3000;
						if(_debugEnabled)
						{
							_serialStream->println(F("Arduino:Gone online"));
						}
					}
				}
				/*else if(_debugEnabled)
				{
					_serialStream->println("Arduino:Wrong loopback message " + String(value) + " received, expecting " + String(_loopbackSequenceNumber));
					_loopbackSequenceNumber = value;
				}*/
			}
			else
			{
				_messageWaiting = true;            //Inform the main loop a message is waiting
				_mostRecentTopic = receivedTopic;          //Overwrite the waiting message if there is one, tough luck too slow
				_mostRecentMessage = message;
			}
		}
	}
}

void serial2mqtt::debug(bool newValue)
{
	_debugEnabled = newValue;
}

void serial2mqtt::statusLed(uint8_t pin)
{
	_statusLedEnabled = true;
	_statusLed = pin;
	pinMode(_statusLed,OUTPUT);
	digitalWrite(_statusLed,LOW);
}

bool serial2mqtt::_isNumber(String message)                      //Check if a message is an integer
{
	for(uint16_t i = 0; i<message.length()-1;i++)
	{
		if(not isDigit(message.charAt(i)))
		{
			return(false);
		}
	}
	return(true);
}

bool serial2mqtt::_validMessage(String stringToCheck)          //Check if the message is valid looking, it is not an exhaustive test
{
	return(true);
	if(stringToCheck.charAt(0) == '{' && stringToCheck.charAt(stringToCheck.length()-1) == '}')
	{
		return(true);
	}
	else
	{
		return(false);
	}
}
#endif