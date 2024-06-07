#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include "secrets.h"
#include <PubSubClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

#define CALIBRATION_FILE "/calibrationData2"

// Keypad start position, key sizes and spacing
#define KEY_X 40 // Centre of key
#define KEY_Y 96
#define KEY_W 62 // Width and height
#define KEY_H 30
#define KEY_SPACING_X 18 // X and Y gap
#define KEY_SPACING_Y 20
#define KEY_TEXTSIZE 1 // Font size multiplier

// Using two fonts since numbers are nice when bold
#define LABEL1_FONT &FreeSansOblique12pt7b // Key label font 1
#define LABEL2_FONT &FreeSansBold12pt7b		 // Key label font 2

// Numeric display box size and location
#define DISP_X 1
#define DISP_Y 10
#define DISP_W 238
#define DISP_H 50
#define DISP_TSIZE 3
#define DISP_TCOLOR TFT_CYAN

// Number length, buffer for storing it and character index
#define NUM_LEN 12
char numberBuffer[NUM_LEN + 1] = "";
uint8_t numberIndex = 0;

// We have a status line for messages
#define STATUS_X 120 // Centred on this
#define STATUS_Y 65

// Create 15 keys for the keypad
char keyLabel[15][5] = {"New", "Del", "Send", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "#"};
uint16_t keyColor[15] = {TFT_RED, TFT_DARKGREY, TFT_DARKGREEN,
												 TFT_BLUE, TFT_BLUE, TFT_BLUE,
												 TFT_BLUE, TFT_BLUE, TFT_BLUE,
												 TFT_BLUE, TFT_BLUE, TFT_BLUE,
												 TFT_BLUE, TFT_BLUE, TFT_BLUE};

// Invoke the TFT_eSPI button class and create all the button objects
TFT_eSPI_Button key[15];

WiFiClient wifiClient;
int wifi_status = WL_IDLE_STATUS;

PubSubClient mqttClient(wifiClient);

// uses PWM to set brightness between 0 and 100%
void set_brightness(uint8_t Value)
{
	if (Value < 0 || Value > 100)
	{
		printf("TFT_SET_BL Error \r\n");
	}
	else
	{
		analogWrite(TFT_BL, Value * 2.55);
	}
}

void touch_calibrate()
{
	uint16_t calibrationData[5];
	uint8_t calDataOK = 0;

	// check file system
	if (!SPIFFS.begin())
	{
		Serial.println("formatting file system");

		SPIFFS.format();
		SPIFFS.begin();
	}

	// check if calibration file exists
	if (SPIFFS.exists(CALIBRATION_FILE))
	{
		File f = SPIFFS.open(CALIBRATION_FILE, "r");
		if (f)
		{
			if (f.readBytes((char *)calibrationData, 14) == 14)
				calDataOK = 1;
			f.close();
		}
	}
	if (calDataOK)
	{
		// calibration data valid
		tft.setTouch(calibrationData);
	}
	else
	{
		// data not valid. recalibrate
		tft.calibrateTouch(calibrationData, TFT_WHITE, TFT_RED, 15);
		// store data
		File f = SPIFFS.open(CALIBRATION_FILE, "w");
		if (f)
		{
			f.write((const unsigned char *)calibrationData, 14);
			f.close();
		}
	}
}

void draw_keypad()
{
	// Draw the keys
	for (uint8_t row = 0; row < 5; row++)
	{
		for (uint8_t col = 0; col < 3; col++)
		{
			uint8_t b = col + row * 3;

			if (b < 3)
				tft.setFreeFont(LABEL1_FONT);
			else
				tft.setFreeFont(LABEL2_FONT);

			key[b].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
												KEY_Y + row * (KEY_H + KEY_SPACING_Y), // x, y, w, h, outline, fill, text
												KEY_W, KEY_H, TFT_WHITE, keyColor[b], TFT_WHITE,
												keyLabel[b], KEY_TEXTSIZE);
			key[b].drawButton();
		}
	}
}

void printCurrentNet()
{

	// print hostname
	Serial.print("Hostname: ");
	Serial.println(WiFi.getHostname());

	// print the SSID of the network you're attached to:

	Serial.print("SSID: ");

	Serial.println(WiFi.SSID());

	// print the MAC address of the router you're attached to:

	byte bssid[6];

	WiFi.BSSID(bssid);

	Serial.print("BSSID: ");

	Serial.print(bssid[5], HEX);

	Serial.print(":");

	Serial.print(bssid[4], HEX);

	Serial.print(":");

	Serial.print(bssid[3], HEX);

	Serial.print(":");

	Serial.print(bssid[2], HEX);

	Serial.print(":");

	Serial.print(bssid[1], HEX);

	Serial.print(":");

	Serial.println(bssid[0], HEX);

	// print the received signal strength:

	long rssi = WiFi.RSSI();

	Serial.print("signal strength (RSSI):");

	Serial.println(rssi);

	// print the encryption type:

	byte encryption = WiFi.encryptionType();

	Serial.print("Encryption Type:");

	Serial.println(encryption, HEX);

	Serial.println();
}

void ensureConnectedToWifi()
{
	while (wifi_status != WL_CONNECTED)
	{
		Serial.print("Attempting to connect to WPA SSID: ");

		Serial.println(WIFI_SSID);

		// disable low power mode since we're just plugged in
		WiFi.noLowPowerMode();

// set hostname if HOSTNAME is defined
#ifdef HOSTNAME
		WiFi.setHostname(HOSTNAME);
#endif

		// Connect to WPA/WPA2 network:
		wifi_status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

		// wait 5 seconds for connection:

		delay(5000);
		Serial.println("wifi status: " + String(wifi_status));
	}
}

void setup()
{
	// turn on pin mode to signal we have power, turn off when setup is complete
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	// connect to wifi
	ensureConnectedToWifi();
	printCurrentNet();

	// OTA setup
	ArduinoOTA.onStart([]()
										 {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type); });
	ArduinoOTA.onEnd([]()
									 { Serial.println("\nEnd"); });
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
												{ Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
	ArduinoOTA.onError([](ota_error_t error)
										 {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    } });
	ArduinoOTA.begin();

	// configure mqtt
	mqttClient.setServer(BROKER_ADDRESS, 1883);

	// turn on onboard LED to signal that we've finished setting up
	digitalWrite(LED_BUILTIN, LOW);

	// initialize TFT
	tft.init();
	set_brightness(20);
	tft.setRotation(2); // vertical with pins on top
	touch_calibrate();

	// Clear the screen
	tft.fillScreen(TFT_BLACK);

	// Draw keypad background
	tft.fillRect(0, 0, 240, 320, TFT_DARKGREY);

	// Draw number display area and frame
	tft.fillRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_BLACK);
	tft.drawRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_WHITE);

	// Draw keypad
	draw_keypad();
}

// Print something in the mini status bar
void status(const char *msg)
{
	tft.setTextPadding(240);
	// tft.setCursor(STATUS_X, STATUS_Y);
	tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
	tft.setTextFont(0);
	tft.setTextDatum(TC_DATUM);
	tft.setTextSize(1);
	tft.drawString(msg, STATUS_X, STATUS_Y);
}

void reconnect()
{
	// Loop until we're reconnected
	while (!mqttClient.connected())
	{
		Serial.print("Attempting MQTT connection...");
		// Create a random client ID
		String clientId = HOSTNAME;
		clientId += String(random(0xffff), HEX);
		// Attempt to connect
		if (mqttClient.connect(clientId.c_str()))
		{
			Serial.println("connected");
		}
		else
		{
			Serial.print("failed, rc=");
			Serial.print(mqttClient.state());
			Serial.println(" try again in 5 seconds");
			// Wait 1 seconds before retrying
			delay(1000);
		}
	}
}

void loop()
{
	// ensure we're connected to the wifi
	ensureConnectedToWifi();

	// arduino OTA
	ArduinoOTA.handle();

	// ensure we're connected to the mqtt broker
	if (!mqttClient.connected())
	{
		reconnect();
	}
	mqttClient.loop();

	// start handling keypad
	uint16_t t_x, t_y;
	static uint16_t color;

	// Pressed will be set true is there is a valid touch on the screen
	bool pressed = tft.getTouch(&t_x, &t_y);

	// / Check if any key coordinate boxes contain the touch coordinates
	for (uint8_t b = 0; b < 15; b++)
	{
		if (pressed && key[b].contains(t_x, t_y))
		{
			key[b].press(true); // tell the button it is pressed
		}
		else
		{
			key[b].press(false); // tell the button it is NOT pressed
		}
	}

	// Check if any key has changed state
	for (uint8_t b = 0; b < 15; b++)
	{

		if (b < 3)
			tft.setFreeFont(LABEL1_FONT);
		else
			tft.setFreeFont(LABEL2_FONT);

		if (key[b].justReleased())
			key[b].drawButton(); // draw normal

		if (key[b].justPressed())
		{
			key[b].drawButton(true); // draw invert

			// if a numberpad button, append the relevant # to the numberBuffer
			if (b >= 3)
			{
				if (numberIndex < NUM_LEN)
				{
					numberBuffer[numberIndex] = keyLabel[b][0];
					numberIndex++;
					numberBuffer[numberIndex] = 0; // zero terminate
				}
				status(""); // Clear the old status
			}

			// Del button, so delete last char
			if (b == 1)
			{
				numberBuffer[numberIndex] = 0;
				if (numberIndex > 0)
				{
					numberIndex--;
					numberBuffer[numberIndex] = 0; //' ';
				}
				status(""); // Clear the old status
			}

			if (b == 2)
			{
				status("Sent value to serial port");
				Serial.println(numberBuffer);
				String payload = "{\"state\":\"ON\", \"brightness\":" + String(numberBuffer) + "}";
				mqttClient.publish("underbed-proximity-lights/light/light/command", payload.c_str());
			}
			// we dont really check that the text field makes sense
			// just try to call
			if (b == 0)
			{
				status("Value cleared");
				numberIndex = 0;							 // Reset index to 0
				numberBuffer[numberIndex] = 0; // Place null in buffer
			}

			// Update the number display field
			tft.setTextDatum(TL_DATUM);				// Use top left corner as text coord datum
			tft.setFreeFont(&FreeSans18pt7b); // Choose a nice font that fits box
			tft.setTextColor(DISP_TCOLOR);		// Set the font colour

			// Draw the string, the value returned is the width in pixels
			int xwidth = tft.drawString(numberBuffer, DISP_X + 4, DISP_Y + 12);

			// Now cover up the rest of the line up by drawing a black rectangle.  No flicker this way
			// but it will not work with italic or oblique fonts due to character overlap.
			tft.fillRect(DISP_X + 4 + xwidth, DISP_Y + 1, DISP_W - xwidth - 5, DISP_H - 2, TFT_BLACK);

			delay(10); // UI debouncing
		}
	}
}