#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	tft.init();
	tft.setRotation(1);
	tft.fillScreen(TFT_BLACK);
	tft.setCursor(0, 0, 2);
	tft.setTextColor(TFT_WHITE);
	tft.println("Hello World!");
}

void loop()
{
	Serial.println("Hello World!");
	String message = "MISO is " + String(TFT_MISO) + " MOSI is " + String(TFT_MOSI) + " SCLK is " + String(TFT_SCLK) + " CS is " + String(TFT_CS) + " DC is " + String(TFT_DC) + " RST is " + String(TFT_RST) + " BL is " + String(TFT_BL);
	Serial.println(message);
	delay(1000);
}