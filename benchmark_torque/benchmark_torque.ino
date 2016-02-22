/*
Forked and additions made by @mr-sneezy
Editing and suggestions by @micooke

Benchmark the timing of the torque response using a bluetooth module: HC-05
*/
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <SoftwareSerialWithHalfDuplex.h>

#define OBD2_BUFFER_LENGTH 20

/*
* LCD RS pin 9
* LCD Enable pin 8
* LCD D4 pin 7
* LCD D5 pin 6
* LCD D6 pin 5
* LCD D7 pin 4
* LCD R/W pin to ground
* 10K potentiometer:
* ends to +5V and ground
* wiper to LCD VO pin (pin 3)
*/
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);

const uint8_t buttonPin = 3;      // the number of the pushbutton pin
const uint8_t ledPin =  13;       // the number of the LED pin

bool ledState = LOW;          // the current state of the output pin
const uint16_t ledDelay = 500;

enum state {DATA_ERROR, DATA, NO_DATA, RESET, OK};

//Variables for temperature sensor readings and calculations.

SoftwareSerialWithHalfDuplex btSerial(10,11); // RX, TX

#define debugSerial Serial

#define _DEBUG 0

#if (_DEBUG > 0)
#define DebugPrint(x) debugSerial.print(x)
#define DebugPrintln(x) debugSerial.println(x)
#define DebugPrintHex(x) debugSerial.print(x, HEX)
#else
#define DebugPrint(x)
#define DebugPrintln(x)
#define DebugPrintHex(x)
#endif

uint8_t MAX_VALID = 0;
uint16_t timeOut = 300;

char first_message[OBD2_BUFFER_LENGTH] = {0};

// process bt command for the ecu
// processBluetoothCommand("0100", 4); // OBD2 Mode 1 command
void processBluetoothCommand(const char * btdata1, const uint8_t &command_length);

void bt_write(char *str)
{
	while (*str != '\0')
	btSerial.write(*str++);
}


int16_t procbtSerialMin(void)
{
	// initialise all arrays as zeros
	char btdata1[OBD2_BUFFER_LENGTH] = {0};  // bt data in buffer
	
	uint8_t command_length = 0;
	uint8_t delays_called = 0;
	
	btSerial.listen();
	
	// Wait until 
	while (btSerial.available() == false)
	{
		delay(10);
		delays_called++;
	}

	// get the command string
	while (command_length < 20)
	{
		if (btSerial.available())
		{
			btdata1[command_length] = toupper(btSerial.read());
			if (btdata1[command_length] == '\r')
			{ // terminate at \r
				btdata1[command_length] = '\0';
				break;
			}
			else if (btdata1[command_length] != ' ')
			{ // ignore space
				++command_length;
			}
		}
	}
		
	if (strcmp(btdata1, first_message) == 0)
	{
		return -1;
	}

	DebugPrint(F("BT Command :>"));
	DebugPrint(btdata1);
	DebugPrint(F("< Length = "));
	DebugPrintln(command_length);
		
	processBluetoothCommand(btdata1, command_length);
	return 10*delays_called;
}

bool procbtSerial(const uint16_t timeOut_ = 300)
{
	// initialise all arrays as zeros
	char btdata1[OBD2_BUFFER_LENGTH] = {0};  // bt data in buffer
	
	uint8_t command_length = 0;
	
	btSerial.listen();
	delay(timeOut_);
	if (btSerial.available())
	{

		// get the command string
		while (command_length < 20)
		{
			if (btSerial.available())
			{
				btdata1[command_length] = toupper(btSerial.read());
				if (btdata1[command_length] == '\r')
				{ // terminate at \r
					btdata1[command_length] = '\0';
					break;
				}
				else if (btdata1[command_length] != ' ')
				{ // ignore space
					++command_length;
				}
			}
		}
		
		if (strcmp(btdata1, first_message) == 0)
		{
			return false;
		}

		DebugPrint(F("BT Command :>"));
		DebugPrint(btdata1);
		DebugPrint(F("< Length = "));
		DebugPrintln(command_length);
		
		processBluetoothCommand(btdata1, command_length);
		return true;
	}
	return false;
}

void processBluetoothCommand(const char * btdata1, const uint8_t &command_length)
{
	char btdata2[OBD2_BUFFER_LENGTH] = {0};  // bt data out buffer
	
	state response = NO_DATA; // Default response = NO DATA
	
	/// ELM327 (Hayes modem style) AT commands
	if (!strcmp(btdata1, "ATD"))
	{
		response = OK;
	}//               print id / general | reset all / general
	else if (!strcmp(btdata1, "ATI"))
	{
		response = RESET;
	}
	else if (!strcmp(btdata1, "ATZ"))
	{
		response = RESET;
	}
	// echo on/off / general
	else if (command_length == 4 && strstr(btdata1, "ATE"))
	{
		//elm_echo = (btdata1[3] == '1' ? true : false);
		response = OK;
	}
	// line feed on/off / general
	else if (command_length == 4 && strstr(btdata1, "ATL"))
	{
		//elm_linefeed = (btdata1[3] == '1' ? true : false);
		response = OK;
	}
	// memory on/off / general
	else if (command_length == 4 && strstr(btdata1, "ATM"))
	{
		//elm_memory = (btdata1[3] == '1' ? true : false);
		response = OK;
	}
	// space on/off / obd
	else if (command_length == 4 && strstr(btdata1, "ATS"))
	{
		//elm_space = (btdata1[3] == '1' ? true : false);
		response = OK;
	}
	// headers on/off / obd
	else if (command_length == 4 && strstr(btdata1, "ATH"))
	{
		//elm_header = (btdata1[3] == '1' ? true : false);
		response = OK;
	}
	// set protocol to ? and save it / obd
	else if (command_length == 5 && strstr(btdata1, "ATSP"))
	{
		//elm_protocol = atoi(data[4]);
		response = OK;
	}
	// set hobd protocol
	else if (command_length == 6 && strstr(btdata1, "ATSHP"))
	{
		response = OK;
	}
	// get hobd protocol
	else if (!strcmp(btdata1, "ATDHP"))
	{
		sprintf_P(btdata2, PSTR("HOBD%d\r\n>"), 2);
		response = DATA;
	}
	// pin 13 test
	else if (command_length == 5 && strstr(btdata1, "AT13"))
	{
		response = OK;
	}
	// door lock signal @ D17 / A3
	else if (!strcmp(btdata1, "AT17"))
	{
		response = OK;
	}
	// door unlock signal @ D18 / A4
	else if (!strcmp(btdata1, "AT18"))
	{
		response = OK;
	}
	else
	{
		response = NO_DATA;
	}

	switch(response)
	{
		case RESET:
		sprintf_P(btdata2, PSTR("Honda OBD v1.0\r\n>")); break;
		case OK:
		sprintf_P(btdata2, PSTR("OK\r\n>")); break;
		case DATA_ERROR:
		sprintf_P(btdata2, PSTR("DATA ERROR\r\n>")); break;
		case NO_DATA:
		sprintf_P(btdata2, PSTR("NO DATA\r\n>")); break;
		default: // Default case is DATA - btdata2 is already set (or NULL)
		break;
	}

	DebugPrint(F("BT Response >"));
	DebugPrint(btdata2);
	DebugPrintln(F("<"));

	bt_write(btdata2); // send response data
}

uint8_t timeTorqueResponse(const uint16_t timeOut_ = 300)
{
	uint32_t t0 = millis();

	uint8_t numValid = 0;
	
	while (procbtSerial(timeOut_)) { numValid++; }

	DebugPrint(num_valid);
	DebugPrintln(F(" valid"));
	
	DebugPrint(F("Time taken : "));
	DebugPrint(millis() - t0);
	DebugPrint(F("ms"));
	
	return numValid;
}

uint8_t timeMinTorqueResponse(void)
{
	uint32_t t0 = millis();

	uint8_t numValid = 0;
	
	while (procbtSerialMin() >= 0) { numValid++; }

	DebugPrint(num_valid);
	DebugPrintln(F(" valid"));
	
	DebugPrint(F("Time taken : "));
	DebugPrint(millis() - t0);
	DebugPrint(F("ms"));
	
	return numValid;
}

void setup()
{
	// initialize the LED pin (and others) as an output
	pinMode(ledPin, OUTPUT);
	digitalWrite(ledPin, LOW);

	// initialize the pushbutton pin as an input
	pinMode(buttonPin, INPUT);

	// Setup the lcd
	lcd.begin(16, 2);
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print(F("WAITING 4 BUTTON"));

	#if ( _DEBUG > 0 )
	debugSerial.begin(9600);
	#endif
	
	// Setup the bluetooth and start it listening
	btSerial.begin(9600);
	
	// wait for the button to be pressed to start the benchmark
	while (digitalRead(buttonPin) == LOW);
	
	lcd.setCursor(0, 0);
	lcd.print(F("STATUS:Calibrate"));

	btSerial.listen();
	delay(timeOut);

	while (btSerial.available() == 0) { delay(10);}
	
	// Get the first bluetooth command & save it
	uint8_t command_length = 0;
	while (command_length < 20)
	{
		if (btSerial.available())
		{
			first_message[command_length] = toupper(btSerial.read());
			if (first_message[command_length] == '\r')
			{ // terminate at \r
				first_message[command_length] = '\0';
				break;
			}
			else if (first_message[command_length] != ' ')
			{ // ignore space
				++command_length;
			}
		}
	}
	DebugPrint(F("BT Command :>"));
	DebugPrint(btdata1);
	DebugPrint(F("< Length = "));
	DebugPrintln(command_length);
	
	processBluetoothCommand(first_message, command_length);

	MAX_VALID = timeTorqueResponse(timeOut);

	lcd.print(F("MAXNUM VALID:   "));
	lcd.setCursor(13, 1);
	lcd.print(MAX_VALID);
	lcd.setCursor(0, 0);
}

bool testing = true;

void loop()
{
	// 3 iterations for each test
	while (testing & (timeOut > 0))
	{
		for (uint8_t i = 0; i < 3; ++i)
		{
			const uint8_t num_valid = timeTorqueResponse(timeOut);
			if (num_valid < MAX_VALID)
			{
				timeOut = timeOut + 10;

				DebugPrint(F("COMPLETE : The minimum response time for an Torque command (without error) is : "));
				DebugPrint(timeOut);
				DebugPrintln(F("ms"));

				lcd.print(F("MIN DELAY:    ms"));
				uint8_t _inc = 11 + (timeOut < 10) ? 2 : (timeOut < 100) ? 1 : 0;
				lcd.setCursor(_inc, 0);
				lcd.print(timeOut);
				testing = false;
				break;
			}
		}
		timeOut -= 10;
	}

	// were done - blinky time!
	digitalWrite(ledPin, HIGH);
	delay(ledDelay);
	digitalWrite(ledPin, LOW);
	delay(ledDelay);
}