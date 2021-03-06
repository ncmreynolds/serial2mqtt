#ifndef serial2mqtt_h
#define serial2mqtt_h
#include <Arduino.h>
class serial2mqtt
{

  public: 
    
    serial2mqtt()											//Constructor function, not actively used
	{}
    ~serial2mqtt()											//Deconstructor, not actively used
	{}

    void begin(Stream &);									//Pass a reference to the stream used for the serial port. Often this will be &Serial
	void useJSONobjects();									//Use JSON objects to talk to serial2mqtt on Linux
	void useJSONarrays();									//Use JSON objects to talk to serial2mqtt on Linux, this is the default
	void loopbackTopic(String);								//Set the topic used for loopback tests, should begin with /src/
	
	bool online();											//Is the MQTT server reachable? Tested with the loopback topic

	bool messageWaiting();									//Is there a message waiting
    String topic();											//Return the most recent topic as a String
    String message();										//Return the most recent message as a String	
	bool messageIsNumber();									//Is the most recent message a number?
    uint32_t messageInt();									//Return the most recent message an int32_t
    void markMessageRead();									//Mark the most recent message read

	//Publish method - done with templates (hence inside this file) but this should be transparent to a user of the library
	template <class templateTopic, class templateMessage>
	void publish(templateTopic topic, templateMessage message)
	{
		if(_debugEnabled)
		{
			_serialStream->print(F("Arduino:Publishing to "));
			_serialStream->print(topic);
			_serialStream->print(F(" data "));
			_serialStream->println(message);
		}
		if(_useJSONobjects)
		{
			_serialStream->print(F("{ \"cmd\":\"MQTT-PUB\",\"topic\":\""));
			_serialStream->print(topic);
			_serialStream->print(F("\",\"message\":\""));
			_serialStream->print(message);
			_serialStream->println(F("\",\"qos\":0,\"retained\":false }"));
		}
		else
		{
			_serialStream->print(F("[1,\""));
			_serialStream->print(topic);
			_serialStream->print(F("\",\""));
			_serialStream->print(message);
			_serialStream->print(F("\",0,0]\r\n"));
		}
	}
    
    void subscribe(String topic);							//Subscribe to a topic
    void housekeeping();									//Process incoming/outgoing serial data
	
	//Printing methods - done with templates (hence inside this file) but this should be transparent to a user of the library
	template <class variableContent>
	void print(variableContent content)
	{
		_serialStream->print(content);
	}
	
	template <class variableContent>
	void println(variableContent content)
	{
		_serialStream->println(content);
	}
	
	void debug(bool);										//Enable/disable debug logging
	void statusLed(uint8_t);								//Enable the status LED
    
  private:

    Stream *_serialStream = nullptr; 				       //The stream used for the communication with serial2mqtt
    String _loopbackTopic;
	bool _loopbackConfigured = false;
    uint32_t _lastSentLoopback = 0;
    uint32_t _lastReceivedLoopback = 0;
    uint8_t _loopbackSequenceNumber = 0;
    uint32_t _loopbackInterval = 45e2;  					//4.5s loopback to stop the Pi timing out
    bool _online = false;
    String _mostRecentTopic = "";
    String _mostRecentMessage = "";
    bool _messageWaiting = false;
	bool _debugEnabled = false;
	bool _useJSONobjects = false;			//Use JSON objects to communicate with serial2mqtt
	
	bool _statusLedEnabled = false;			//Is the status LED enabled?
	uint8_t _statusLed = 0;					//Which pin is the status LED on?
	uint32_t _statusLedTimer = 0;			//Timer for slow/fast blink
	bool _statusLedState = false;			//Current state of status LED
	uint32_t _flashFrequency = 500;			//Flash frequencyt of the status LED


    bool _isNumber(String);							//Check if a message is an integer
    bool _validMessage(String);				//Check if the message is valid looking, it is not an exhaustive test
  
};
#endif