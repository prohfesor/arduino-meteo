// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

#include "DHT.h"

#include "Wire.h"
#include "Adafruit_BMP085.h"

#include "EtherCard.h"

#define DHTPIN 3     // what pin we're connected to

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor for normal 16mhz Arduino
//DHT dht(DHTPIN, DHTTYPE);
// NOTE: For working with a faster chip, like an Arduino Due or Teensy, you
// might need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// Example to initialize DHT sensor for Arduino Due:
DHT dht(DHTPIN, DHTTYPE, 7);

/***************************************************
This is an example for the BMP085 Barometric Pressure & Temp Sensor

Designed specifically to work with the Adafruit BMP085 Breakout
----> https://www.adafruit.com/products/391

These displays use I2C to communicate, 2 pins are required to
interface
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, all text above must be included in any redistribution
****************************************************/
Adafruit_BMP085 bmp;


static byte mac[] = { 0x74, 0x69, 0x68, 0x67, 0x66, 0x01 };
static String sMac = "746968676601";
byte Ethernet::buffer[700];
Stash stash;

char website[] PROGMEM = "narodmon.ru";
static byte hisip[] = { 91, 122, 49, 168 };
uint16_t nSourcePort = 8888;
uint16_t nDestinationPort = 8283;


//sensor vars
float t, t2, h, a, p;


void setup() {
	Serial.begin(9600);
	//delay(1000);
	
	Serial.println("DHTxx starting");
	dht.begin();
	
	Serial.println("BMPxxx starting");
	bmp.begin();

	Serial.println("Ethernet starting");
	//ethStart();

	Serial.println();
}


void loop() {
	// Wait a few seconds between measurements.
	delay(5000);

	//DHT sensor - takes about 250 milliseconds!
	readDht();

	//BMP sensor
	readBmp();

	//LAN narodmon
	//sendUdp();
	//sendTcp();
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
	if (ether.begin(sizeof Ethernet::buffer, mac) == 0)
		Serial.println("Failed to access Ethernet controller");
	if (!ether.dhcpSetup())
		Serial.println("DHCP failed");

	/*
	//overwrite DNS with google's if there are problems with DNS setup
	static byte dnsip[] = { 8, 8, 8, 8 };
	ether.copyIp(ether.dnsip, dnsip);
	*/

	ether.printIp("My IP: ", ether.myip);
	ether.printIp("GW IP: ", ether.gwip);
	ether.printIp("DNS IP: ", ether.dnsip);

	/*
	Serial.println("Looking for Gateway");
	while (ether.clientWaitingGw())
	ether.packetLoop(ether.packetReceive());
	Serial.println("Gateway found");
	*/

	Serial.print("Looking for narodmon.ru server: ");
	if (!ether.dnsLookup(website)){
		Serial.println("DNS failed");
		ether.parseIp(ether.hisip, "91.122.49.168"); //92.39.235.156
	}

	//ether.copyIp(ether.hisip, hisip);

	ether.printIp("Server: ", ether.hisip);
}



void sendUdp() {
	Serial.println("Push to narodmon");
	String message = "#74-69-68-67-66-01#Meteo\n#74696867660101#%T1%#Temp1\n";
	message += "#74696867660102#%T2%#Temp2\n#74696867660103#%H%#Humidity\n";
	message += "#74696867660104#%P%#Pressure\n##";

	/*
	char buf[7];
	dtostrf(t, 7, 2, buf);
	message.replace("T1", buf);
	dtostrf(t2, 7, 2, buf);
	message.replace("T2", buf);
	dtostrf(h, 7, 2, buf);
	message.replace("H", buf);
	*/

	int len = message.length() +1;
	char payload[200];
	message.toCharArray(payload, len);

	ether.sendUdp(payload, sizeof(payload), nSourcePort, ether.hisip, nDestinationPort);
}



void sendTcp() {
	Serial.println("Send POST to narodmon");
	byte sd = stash.create();
	stash.print("ID=");
	stash.print(sMac);
	stash.print("&");
	stash.print(sMac);
	stash.print("01");
	stash.print("=");
	stash.print(t);
	//stash.print("&name=Meteo");

	stash.save();
	// generate the header with payload - note that the stash size is used,
	// and that a "stash descriptor" is passed in as argument using "$H"
	Stash::prepare(PSTR("POST http://narodmon.ru/post.php HTTP/1.0" "\r\n"
		"Host: narodmon.ru" "\r\n"
		"Content-Length: $D" "\r\n"
		"\r\n"
		"$H"),
		stash.size(), sd);
	// send the packet - this also releases all stash buffers once done
	ether.tcpSend();
	Serial.println("Done");
}