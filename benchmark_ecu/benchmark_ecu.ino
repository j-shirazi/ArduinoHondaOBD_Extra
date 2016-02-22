/*
Forked and additions made by @mr-sneezy
Editing and suggestions by @micooke

Benchmark the timing of the OBD2 response from the honda S2000
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
const uint8_t ledPin = 13;       // the number of the LED pin

bool ledState = LOW;          // the current state of the output pin
const uint16_t ledDelay = 500;

enum state { DATA_ERROR, DATA, NO_DATA, RESET, OK };

//Variables for temperature sensor readings and calculations.
SoftwareSerialWithHalfDuplex ecuSerial(12, 12, false, false);

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

void ecuInit()
{
	ecuSerial.write(0x68);
	ecuSerial.write(0x6a);
	ecuSerial.write(0xf5);
	ecuSerial.write(0xaf);
	ecuSerial.write(0xbf);
	ecuSerial.write(0xb3);
	ecuSerial.write(0xb2);
	ecuSerial.write(0xc1);
	ecuSerial.write(0xdb);
	ecuSerial.write(0xb3);
	ecuSerial.write(0xe9);
}

state ecuCommand(byte cmd, byte num, byte loc, byte len, byte(&data)[OBD2_BUFFER_LENGTH], const uint16_t timeOut_ = 300)
{
	byte crc = (0xFF - (cmd + num + loc + len - 0x01)); // checksum FF - (cmd + num + loc + len - 0x01)

	uint32_t timeOut = millis() + timeOut_;
	// Not needed, all arrays passed to this function have been initialised as all zeros
	// memset(data, 0, sizeof(data) * OBD2_BUFFER_LENGTH);

	ecuSerial.listen();

	ecuSerial.write(cmd);  // header/cmd read memory ??
	ecuSerial.write(num);  // num of bytes to send
	ecuSerial.write(loc);  // address
	ecuSerial.write(len);  // num of bytes to read
	ecuSerial.write(crc);  // checksum

	int i = 0;
	while (i < (len + 3) && millis() < timeOut)
	{
		if (ecuSerial.available())
		{
			data[i] = ecuSerial.read();
			i++;
		}
	}

	// or use checksum?
	if (data[0] != 0x00 && data[1] != (len + 3))
	{
		return DATA_ERROR; // invalid data error
	}
	if (i < (len + 3))
	{ // timeout
		return NO_DATA; // timeout error
	}
	return DATA; // success
}

uint8_t timeEcuResponse(const uint16_t timeOut_ = 300)
{
	uint8_t numValid = 0;
	byte ecudata[OBD2_BUFFER_LENGTH] = { 0 };  // ecu data buffer

	state response = NO_DATA; // Default response = NO DATA

	response = ecuCommand(0x20, 0x05, 0x0B, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; } // 1
	response = ecuCommand(0x20, 0x05, 0x9a, 0x02, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x9c, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x10, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x20, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; } // 5
	response = ecuCommand(0x20, 0x05, 0x22, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x12, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x00, 0x02, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x02, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x26, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; } // 10
	response = ecuCommand(0x20, 0x05, 0x11, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x14, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x15, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x13, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x17, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; } // 15
	response = ecuCommand(0x20, 0x05, 0x28, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x08, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x09, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x0A, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x0B, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; } // 20
	response = ecuCommand(0x20, 0x05, 0x0C, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x0D, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x0E, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; }
	response = ecuCommand(0x20, 0x05, 0x0F, 0x01, ecudata, timeOut_); if (response == DATA) { numValid++; } // 24

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

	// setup the ECU serial
	ecuSerial.begin(9600);

	// wait for the button to be pressed to start the benchmark
	while (digitalRead(buttonPin) == LOW);

	delay(100);
	ecuInit();
	delay(300);
	
	// We assume that a delay time of 300ms will always respond in valid data (if it exists)
	MAX_VALID = timeEcuResponse(timeOut); timeOut -= 10;

	lcd.setCursor(0, 0);
	lcd.print(F("STATUS:Calibrate"));

	if (MAX_VALID < 24)
	{
		DebugPrint(F("WARNING : For 24 ecu commands sent to your vehicle, a maximum of "));
		DebugPrint(MAX_VALID);
		DebugPrintln(F(" resulted in a valid data response"));
	}
	else
	{
		DebugPrintln(F("SUCCESS : 24 out of 24 ecu commands resulted in valid data responses returned!"));
	}

	lcd.print(F("VALID:   MAX: 24"));
	lcd.setCursor(6, 1);
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
			const uint8_t num_valid = timeEcuResponse(timeOut);
			if (num_valid < MAX_VALID)
			{
				timeOut = timeOut + 10;

				DebugPrint(F("COMPLETE : The minimum response time for an Ecu command (without error) is : "));
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
