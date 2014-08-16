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

//LAN vars
static byte mac[] = { 0x74, 0x69, 0x68, 0x67, 0x66, 0x01 };
byte Ethernet::buffer[700];
Stash stash;
byte session;
char website[] PROGMEM = "narodmon.ru";
static byte webip[] = { 91, 122, 49, 168 };


//sensor vars
float t, t2, h, a, p;

//timing variable
int res = 0;


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
	//3000 res = 5mins
	if (res > 3000){
		ethStart();
	}
	res = res+1;

	//listen
	ether.packetLoop(ether.packetReceive());

	//200 res = 10 seconds (50ms each res)
	if (res == 200) {
		//DHT sensor - takes about 250 milliseconds!
		readDht();

		//BMP sensor
		readBmp();

		//LAN narodmon
		//sendUdp();
		sendTcp();
	}

	const char* reply = ether.tcpReply(session);
	if (reply != 0) {
		res = 0;
		Serial.println(reply);
	}

	delay(50);
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
	for (;;){ // keep trying until you succeed 
		Serial.println("Power off Ethernet");
		//Reinitialize ethernet module
		pinMode(5, OUTPUT);
		digitalWrite(5, LOW);
		delay(1000);
		digitalWrite(5, HIGH);
		delay(500);
		Serial.println("Reseting Ethernet...");

		if (ether.begin(sizeof Ethernet::buffer, mac) == 0){
			Serial.println("Failed to access Ethernet controller");
			continue;
		}

		if (!ether.dhcpSetup()){
			Serial.println("DHCP failed");
			continue;
		}

		ether.printIp("IP:  ", ether.myip);
		ether.printIp("GW:  ", ether.gwip);
		ether.printIp("DNS: ", ether.dnsip);

		if (!ether.dnsLookup(website)) {
			Serial.println("DNS failed");
			ether.copyIp(ether.hisip, webip);
		}

		ether.printIp("SRV: ", ether.hisip);

		//reset init value
		res = 0;
		break;
	}
}




void sendTcp() {
	char ssm[] = "746968676601";

	Serial.println("Send POST to narodmon");
	byte sd = stash.create();
	stash.print("ID=");
	stash.print(ssm);
	stash.print("&");
	stash.print(ssm);
	stash.print("01");
	stash.print("=");
	stash.print(t);
	stash.print("&");
	stash.print(ssm);
	stash.print("02");
	stash.print("=");
	stash.print(t2);
	stash.print("&");
	stash.print(ssm);
	stash.print("04");
	stash.print("=");
	stash.print(p);
	stash.print("&name=Meteo");

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