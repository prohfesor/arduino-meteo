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
EthernetClient client;
boolean lastConnected = false;
boolean webservice = false;

//post settings
char openWeatherAuth[] = "xxxxxxxxxxxxxxxxxxx==";	//string "login:pass" with base64 
char narodmonAuth[] = "xxxxxxxxxxxxxxxxxxxxxx";
char stationName[] = "sneg-test";


//timings
#define PUSH_INTERVAL 60000;
#define ETH_INTERVAL  600000;

//sensor vars
float t, t2, h, a, p;



void setup() {
	Serial.begin(9600);

	Serial.println("Ethernet starting");
	ethStart();

	Serial.println("DHTxx starting");
	dht.begin();
	
	Serial.println("BMPxxx starting");
	bmp.begin();

	Serial.println();
}


void loop() {
	//if correct answer is not received then re-initialize ethernet module
	long chke = lastEth + ETH_INTERVAL;
	if (millis() > chke){
		lastEth = millis();
		ethRenew();
	}

	long chk = lastPush + PUSH_INTERVAL;
	if (millis() > chk || !lastPush) {
		lastPush = millis();

		//DHT sensor - takes about 250 milliseconds!
		readDht();

		//BMP sensor
		readBmp();

		//LAN send 
		sendTcp();
	}

	readTcp();
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
	Serial.println("Receive DHCP... ");

	if (Ethernet.begin(mac) == 0) {
		Serial.println("Failed to configure Ethernet using DHCP");
	}
	else {
		Serial.println("OK");
	}

	delay(1000);
	Serial.print("My IP address: ");
	Serial.println(Ethernet.localIP());
}



void ethRenew() {
	Serial.println("Need ethernet renew...");
	Ethernet.maintain();
	Serial.println("done");
	ethStart();
}



void sendTcp() {
	client.stop();
	String server;
	String dataString;
	String headerString;
	// prepare post to service 1
	if (webservice){
		server = "openweathermap.org";
		dataString = "name=";
		dataString += stationName;
		dataString += "&temp=";
		dataString += t;
		dataString += "&humidity=";
		dataString += h;
		dataString += "&pressure=";
		dataString += p / 100;
		headerString = "POST /data/post HTTP/1.1\n";
		headerString += "Host: ";
		headerString += server;
		headerString += "\n";
		headerString += "Authorization: Basic ";
		headerString += openWeatherAuth;
		headerString += "\n";
		headerString += "Content-Type: application/x-www-form-urlencoded\n";
		headerString += "Content-Length: ";
		headerString += dataString.length();
		headerString += "\n";
		headerString += "Connection: close";
	}
	// prepare post to service 2
	else {
		server = "narodmon.ru";
		dataString = "ID=";
		dataString += narodmonAuth;
		dataString += "&name=";
		dataString += stationName;
		dataString += "&";
		dataString += narodmonAuth;
		dataString += "01=";
		dataString += t;
		dataString += "&";
		dataString += narodmonAuth;
		dataString += "02=";
		dataString += h;
		dataString += "&";
		dataString += narodmonAuth;
		dataString += "03=";
		dataString += p;
		headerString = "POST /post.php HTTP/1.1\n";
		headerString += "Host: ";
		headerString += server;
		headerString += "\n";
		headerString += "Content-Type: application/x-www-form-urlencoded\n";
		headerString += "Content-Length: ";
		headerString += dataString.length();
		headerString += "\n";
		headerString += "Connection: close";
	}

	char charBuf[server.length() + 1];
	server.toCharArray(charBuf, server.length()+1);
	if (client.connect(charBuf, 80)) {
		Serial.println("connected");
		// Make a HTTP request:
		client.println(headerString);
		client.println();
		client.println(dataString);

		lastConnected = client.connected();
	}
	else {
		// if you didn't get a connection to the server:
		Serial.println("connection failed");
	}

	webservice = !webservice;
}


void readTcp() {
	if (client.available()) {
		char c = client.read();
		Serial.print(c);
		lastEth = millis();
	}
	lastConnected = client.connected();
	if(!client.connected() && lastConnected) {
		Serial.println();
		Serial.println("disconnecting.");
		client.stop();
	}
}