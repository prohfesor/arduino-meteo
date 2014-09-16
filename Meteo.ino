// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

#include "DHT.h"

#include "Wire.h"
#include "Adafruit_BMP085.h"
#include "SPI.h"
#include "Ethernet.h"

//sensor vars
#define DHTPIN 3
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE, 7);
Adafruit_BMP085 bmp;

//LAN vars
byte mac[] = { 0x74, 0x69, 0x68, 0x67, 0x66, 0x01 };
long lastPush, lastEth;
int EthernetResetPin = 2;
EthernetClient client;

//timings
#define PUSH_INTERVAL 5000;
#define ETH_INTERVAL  600000;

//sensor vars
float t, t2, h, a, p;



void setup() {
	Serial.begin(9600);
	
	Serial.println("DHTxx starting");
	dht.begin();
	
	Serial.println("BMPxxx starting");
	bmp.begin();

	Serial.println("Ethernet starting");
	ethStart();

	Serial.println();
}


void loop() {
	//if correct answer is not received then re-initialize ethernet module
	long chke = lastEth + ETH_INTERVAL;
	if (millis() > chke){
		lastEth = millis();
		ethStart();
	}

	long chk = lastPush + PUSH_INTERVAL;
	if (millis() > chk) {
		lastPush = millis();

		//DHT sensor - takes about 250 milliseconds!
		readDht();

		//BMP sensor
		readBmp();

		//LAN narodmon
		sendTcp();
	}

}



void readDht() {
	// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
	h = dht.readHumidity();
	// Read temperature as Celsius
	t = dht.readTemperature();

	// Check if any reads failed and exit early (to try again).
	if (isnan(h) || isnan(t)) {
		Serial.println("Failed to read from DHT sensor!");
		return;
	}

	//Output
	Serial.print("Humidity: ");
	Serial.print(h);
	Serial.print("%\t");
	Serial.print("Temperature: ");
	Serial.print(t);
	Serial.print("*C\t");
	Serial.println();
}



void readBmp() {
	a = bmp.readAltitude();
	p = bmp.readPressure();
	t2 = bmp.readTemperature();

	// Check if any reads failed and exit early (to try again).
	if (isnan(a) || isnan(p) || isnan(t2)) {
		Serial.println("Failed to read from BMP sensor!");
		return;
	}

	//Output
	Serial.print("Pressure: ");
	Serial.print(p);
	Serial.print("Pa\tAltitude: ");
	Serial.print(a);
	Serial.print("m\t");
	Serial.print("Temperature: ");
	Serial.print(t2);
	Serial.print("*C");
	Serial.println();
}



void ethStart() {
	/*
	Serial.print("Reseting Ethernet... ");
	pinMode(EthernetResetPin, OUTPUT);
	digitalWrite(EthernetResetPin, LOW);
	delay(100);
	digitalWrite(EthernetResetPin, HIGH);
	Serial.println("done");
	delay(100);
	*/

	Serial.print("Receive DHCP... ");
	if (Ethernet.begin(mac)) {
		Serial.println("ok");
		// try to congifure using IP address instead of DHCP:
		//Ethernet.begin(mac, ip);
		// print your local IP address:
		Serial.print("My IP address: ");
		IPAddress ip = Ethernet.localIP();
		for (byte thisByte = 0; thisByte < 4; thisByte++) {
			// print the value of each byte of the IP address:
			Serial.print(ip[thisByte], DEC);
			Serial.print(".");
		}
	}
	else {
		Serial.println("FAILED!");
	}
}




void sendTcp() {

}