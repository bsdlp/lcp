#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

#define CALIBRATION_FILE "/calibrationData"

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

void setup()
{
	uint16_t calibrationData[5];
	uint8_t calDataOK = 0;

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	tft.init();
	tft.setRotation(1);
	tft.fillScreen((0xFFFF));

	tft.setCursor(20, 0, 2);
	tft.setTextColor(TFT_BLACK, TFT_WHITE);
	tft.setTextSize(1);
	tft.println("calibration run");

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
	tft.fillScreen((0xFFFF));
}

void loop()
{
	uint16_t x, y;
	static uint16_t color;

	if (tft.getTouch(&x, &y))
	{
		set_brightness(x * 0.3125);

		tft.setCursor(5, 5, 2);
		tft.printf("x: %i     ", x);
		tft.setCursor(5, 20, 2);
		tft.printf("y: %i    ", y);

		tft.drawPixel(x, y, color);
		color += 155;
	}
}