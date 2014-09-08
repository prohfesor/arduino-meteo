// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

#include "DHT.h"

#include "Wire.h"
#include "Adafruit_BMP085.h"

#include "EtherCard.h"

#define DHTPIN 3
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE, 7);

Adafruit_BMP085 bmp;

//LAN vars
static byte mac[] = { 0x74, 0x69, 0x68, 0x67, 0x66, 0x01 };
static byte webip[] = { 91, 122, 49, 168 };
char website[] PROGMEM = "narodmon.ru";
byte Ethernet::buffer[700];
Stash stash;
byte session;
bool part = 0;
long lastPush, lastEth;
#define PUSH_INTERVAL 5000;
#define ETH_INTERVAL  600000;


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
	long chke = lastEth + ETH_INTERVAL;
	if (millis() > chke){
		lastEth = millis();
		ethStart();
	}
	res = res+1;

	//listen
	ether.packetLoop(ether.packetReceive());

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
			Serial.println("DNS failed. Fallback to hardcoded IP.");
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
	if (part){
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
	}
	else {
		stash.print("&");
		stash.print(ssm);
		stash.print("03");
		stash.print("=");
		stash.print(h);
		stash.print("&");
		stash.print(ssm);
		stash.print("05");
		stash.print("=");
		stash.print(a);
	}
	stash.print("&name=Meteo");
	part++;

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