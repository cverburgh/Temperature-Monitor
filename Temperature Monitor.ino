// Demo the quad alphanumeric display LED backpack kit
// scrolls through every character, then scrolls Serial
// input onto the display

//#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

// WiFi Network setup
const char* ssid = "NothingToSeeHere";
const char* password = "Teamassb5m!";
//const char* host = "wifitest.adafruit.com";
String regBaseUrl = "http://192.168.1.133/api/config/register/"; // mac and sensor type added during registration
String submitServer = "";
String configUrl = "";

int sensorId = -1;
String sensorTypes[] = { "Temperature", "Humidity" };
bool active = false;

// temporary values for testing
int targetTemp = -10;
int tempVariance = 3;

void setup() {
	Serial.begin(9600);
	pinMode(0, OUTPUT);

	// pass in the I2C address for the display
	alpha4.begin(0x70);

	// connect to the WiFi
	// after 30 seconds of not connecting, throw an error
	// hold that for 5 seconds then try connecting again
	// 30 seconds is about 60 loops (500ms per loop)
	WiFi.begin(ssid, password);
	int i = 0;
	while (WiFi.status() != WL_CONNECTED) {
		writeText("WiFi CONNECTING    ", true);
		//Serial.print(".");
		i++;
		if (i >= 10) {
			//Serial.println("WiFi COnnection fail...");
			flashText("Er01", 500, 5);
			i = 0;				// reset and try connecting again
		}
	}

	writeText("    WiFi UP", true);
	delay(1000);

	//Serial.print("MAC: ");
	//Serial.println(WiFi.macAddress());
	Serial.print("IP: ");
	Serial.println(WiFi.localIP());

	// keep trying to register
	while (!registerByMacAddress()) {

	}
}

char displaybuffer[4] = { ' ', ' ', ' ', ' ' };
int temp = -10;

void loop() {
	while (!active) {


		String msg = "    SENSOR NOT ACTIVATED    ";
		Serial.println(msg);
		writeText(msg, true);
		int i = 100;

		while (i >= 0) {
			writeText(String(i), false);
			i--;
			delay(100);
		}
		writeText("TRYING AGAIN    ", true);

		//registerMacAddress();

		if (active) {
			writeText("ACTIVATED!", true);
			delay(3000);
		}
	}

	bool flash = false;

	// check if the temp is within variance limits
	if (abs(temp - targetTemp) > tempVariance) {
		flash = true;
	}

	String displayText = String(temp) + 'c';
	submitValue(temp);

	if (flash) {
		flashText(displayText, 500, 5000);
		/*writeText(displayText, false);
		delay(500);
		writeText("    ", false);
		delay(500);*/
	}
	else {
		writeText(displayText, false);
		delay(5000);
	}

	// change the temp by a random amount
	if (temp > -5) temp = -5;
	if (temp < -15) temp = -15;

	int tempChange = random(-1, 2);
	temp = temp + tempChange;
}

void writeText(String someText, bool scroll) {
	// make sure text length is at least 4 chars long
	while (someText.length() < 4) {
		someText = " " + someText;
	}
	int len = someText.length();

	char ca[len + 1];
	someText.toCharArray(ca, len + 1);

	if (scroll == false) {
		// just write it on the display
		// but jus the first 4 chars

		alpha4.clear();
		alpha4.writeDigitAscii(0, ca[0]);
		alpha4.writeDigitAscii(1, ca[1]);
		alpha4.writeDigitAscii(2, ca[2]);
		alpha4.writeDigitAscii(3, ca[3]);

		alpha4.writeDisplay();
	}
	else {
		for (uint8_t i = 0; i <= len; i++) {
			writeChar(ca[i], scroll);
		}
	}
}

void flashText(String someText, int delayTime, int numOfFlashes) {
	Serial.println("flashing..");
	int i = 0;
	while (i < numOfFlashes) {
		Serial.println("Flash " + i);
		writeText(someText, false);
		delay(delayTime);
		writeText("    ", false);
		delay(delayTime);
		i++;
	}
}

void writeChar(char c, bool scroll) {
	if (!isprint(c)) return; // only printable!

	delay(200);
	// scroll down display
	displaybuffer[0] = displaybuffer[1];
	displaybuffer[1] = displaybuffer[2];
	displaybuffer[2] = displaybuffer[3];
	displaybuffer[3] = c;

	// set every digit to the buffer
	alpha4.writeDigitAscii(0, displaybuffer[0]);
	alpha4.writeDigitAscii(1, displaybuffer[1]);
	alpha4.writeDigitAscii(2, displaybuffer[2]);
	alpha4.writeDigitAscii(3, displaybuffer[3]);

	// write it out!
	alpha4.writeDisplay();
}

void clearIt() {
	writeChar(' ', false);
	writeChar(' ', false);
	writeChar(' ', false);
	writeChar(' ', false);
}

void submitValue(int value) {
	HTTPClient http;  //Object of class HTTPClient

	String url = "http://" + submitServer + "/api/sensordata/submit/" + sensorId + "/" + String(value);
	Serial.println("URL: " + url);

	http.begin(url);

	int httpCode = http.GET();
	if (httpCode > 0) {
		Serial.println("connected, submitting data...");

		/*
		JSON Parsing:	http://www.instructables.com/id/ESP8266-Parsing-JSON/
		and:			https://arduinojson.org/assistant/
		*/
		String json = http.getString();
		Serial.println(json);

	}
	else {
		writeText("    CONNECTION ERROR", true);
		delay(500);
	}
}

bool registerByMacAddress() {
	Serial.println("Registering...");
	 
	HTTPClient http;  //Object of class HTTPClient

	for (int i = 0; i < sensorTypes.length(); i++) {
		String url = "http://" + regBaseUrl + WiFi.macAddress() + "/" + sensorTypes[i];
		Serial.println(url);

		http.begin(url);
		int httpCode = http.GET();

		if (httpCode < 0) {
			flashText("Er02", 500, 10);
			http.end();
			return false;
		}

		if (httpCode > 0) {
			// Get the request response payload
			String json = http.getString();
			Serial.println(json);
			/*
			JSON Parsing:	http://www.instructables.com/id/ESP8266-Parsing-JSON/
			and:			https://arduinojson.org/assistant/
			*/

			const size_t bufferSize = JSON_OBJECT_SIZE(12) + 320;

			DynamicJsonBuffer jsonBuffer(bufferSize);
			JsonObject& root = jsonBuffer.parseObject(json);

			Serial.println(json);

			//sensorId = root["id"];
			//sensorType = String(root["sensorType"]);
			//active = root["active"];
		}
	}

	http.end();   //Close connection
	return true;
}