/*
Geekstips.com
IoT project - Communication between two ESP8266 - Talk with Each Other
ESP8266 Arduino code example
*/

#include "common.h"
#include "data_fields.h"
#include <vector>
#include "ESPComm.h"


//ESP8266WebServer server(80);
WiFiClient client;
MySQL_Connection SQL_Connection((Client *)&client);


//const char password[] = "tightwad22.", ssid[] = "Intellivision";
ESPComm ESPDevice;

const char INDEX_HTML[] PROGMEM =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
"<title>ESP8266 Web Form Demo</title>"
"<style>"
"\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
"</style>"
"</head>"
"<body>"
"<h1>ESP8266 Web Form Demo</h1>"
"<FORM action=\"/\" method=\"post\">"
"<P>"
"LED<br>"
"<INPUT type=\"radio\" name=\"LED\" value=\"1\">On<BR>"
"<INPUT type=\"radio\" name=\"LED\" value=\"0\">Off<BR>"
"<INPUT type=\"submit\" value=\"Send\"> <INPUT type=\"reset\">"
"</P>"
"</FORM>"
"</body>"
"</html>";

//MAIN Functions
void setup() 
{
	
  Serial.begin(250000); //Set up the serial communication.
  WiFi.mode(WIFI_AP_STA); //Access point and station. 
  //setupAccessPoint();
  ESPDevice.setup();
}

void loop()
{
	ESPDevice.Process();
}
//

void ESPComm::setup()
{
	//Init our objects here.
	p_server = new ESP8266WebServer(80); //Open on port 80 (http)
	//p_Client = new WiFiClient;
	//p_SQL_Connection = new MySQL_Connection((Client *)&p_Client);
	//setupServer(); //Set up the web hosting stuff.
	WiFi.enableAP(false); //Disable AP mode by default.
}

void ESPComm::Process()
{
	parseSerialData(); //parse all incoming serial data.
	wl_status_t status = WiFi.status();
	
	if ( status == WL_CONNECTED ) //Only o this stuff if we're connected to a network.
	{
		if ( b_attemptingConnection )
		{
			Serial.println( "Connected with local IP: " + String(WiFi.localIP()) );
			b_attemptingConnection = false;
		}
		p_server->handleClient(); //Process stuff for clients that have connected.
	}
		
	/*else if ( status == WL_CONNECT_FAILED && status != currentStatus )
	{
		Serial.println("Failed to connect to network.");
		currentStatus = status;
		
	}
	else if ( status == WL_CONNECTION_LOST && status != currentStatus );
	{
		Serial.println("Network connection lost.");
		currentStatus = status;
	}*/
}

// Handling the / root web page from my server
void ESPComm::HandleIndex()
{
	
    p_server->send(200, "text/html", INDEX_HTML );
}

void ESPComm::HandleConfig()
{

	p_server->send(200, "text/html", " " );
}

void ESPComm::HandleLogin()
{
	p_server->send( 200, "text/html", " ");
}


//THis function creates the access point if a network is unavailable for us to connect to. 
bool ESPComm::setupAccessPoint( String ssid, String password )
{
	if ( WiFi.status() == WL_CONNECTED )
	{
		Serial.println( "Wifi is already connected, please disconnect before attempting to scan for available networks,");
		return false;
	}
	WiFi.disconnect(); //Just to be sure.
	WiFi.mode(WIFI_AP_STA);
	WiFi.enableAP(true);
	Serial.println("- start ap with SID: "+ String(ssid));
	WiFi.softAP(ssid.c_str(), password.c_str()); //Set up the access point with our password and SSID name.
	IPAddress myIP = WiFi.softAPIP();
	Serial.print("- AP IP address is :");
	Serial.println(myIP);
	setupServer(); //Host our pages
	return true;
}

void ESPComm::closeConnection()
{
	WiFi.enableAP(false);
	p_server->close();
	WiFi.disconnect();
}

void ESPComm::setupServer()
{
	Serial.println("Starting server :");
	//These set up our page triggers, linking them to specific functions.
	p_server->on("/config", std::bind(&ESPComm::HandleConfig, this) );
	p_server->on("/", std::bind(&ESPComm::HandleIndex, this) );
	p_server->on("/login", std::bind(&ESPComm::HandleLogin, this) );
	//
	p_server->begin();
};


void ESPComm::scanNetworks()
{
	if ( WiFi.status() == WL_CONNECTED )
	{
		Serial.println( "Wifi is already connected, please disconnect before attempting to scan for available networks,");
		return;
	}
		
	WiFi.mode(WIFI_STA);
	WiFi.disconnect(); //Just in case
	delay(100);
	int n = WiFi.scanNetworks(); //More than 255 networks in an area? Possible I suppose.
	if (!n)
		Serial.println("no networks found");
	else
	{
		Serial.print(n);
		Serial.println(" networks found");
		for (int i = 0; i < n; ++i)
		{
			// Print SSID and RSSI for each network found
			Serial.print(i + 1);
			Serial.print(": ");
			Serial.print(WiFi.SSID(i));
			Serial.print(" (");
			Serial.print(WiFi.RSSI(i));
			Serial.print(")");
			Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
			delay(10);
		}
	}
}

void ESPComm::beginConnection( const char* ssid, const char* password )
{
	Serial.println("Attempting connection to: " + String(ssid) );
	WiFi.setAutoReconnect(true); //Set up to automatically reconnect to network if disconnected.
	WiFi.begin( ssid, password );
}