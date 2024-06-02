#include <Arduino.h>

void setup()
{
	// put your setup code here, to run once:
	Serial.begin(9600);
	pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
	// put your main code here, to run repeatedly:
	Serial.println("Hello World");
	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
	delay(1000);
}