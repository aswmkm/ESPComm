/*
Geekstips.com
IoT project - Communication between two ESP8266 - Talk with Each Other
ESP8266 Arduino code example
*/

#include "common.h"
#include "data_fields.h"
#include <vector>
#include "ESPComm.h"

WiFiClient client;
MySQL_Connection SQL_Connection((Client *)&client);

ESPComm ESPDevice; //Device object init

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
  WiFi.persistent(false);
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
	setupServer(); //Set up the web hosting directories.
	WiFi.softAPdisconnect( false ); //AP mode disabled by default.
	b_verboseMode = true; //Do this by default for now, will probably have an eeprom setting for this later.
	i_timeoutLimit = 10; //20 second default, will probably be an eeprom config later
}

void ESPComm::Process()
{
	parseSerialData(); //parse all incoming serial data.
	wl_status_t status = WiFi.status();
	
	if ( status == WL_CONNECTED ) //Only o this stuff if we're connected to a network.
	{
		p_server->handleClient(); //Process stuff for clients that have connected.
	}
}

// Handling the / root web page from my server
void ESPComm::HandleIndex()
{
	
    p_server->send(200, "text/html", INDEX_HTML ); //Testing
}

void ESPComm::HandleConfig()
{

	p_server->send(200, "text/html", "This is the config page." );
}

void ESPComm::HandleLogin()
{
	p_server->send( 200, "text/html", "This is the login page.");
}

void ESPComm::HandleAdmin()
{
	p_server->send( 200, "text/html", "This is the admin page.");
}

//THis function creates the access point if a network is unavailable for us to connect to. 
bool ESPComm::setupAccessPoint( String ssid, String password )
{
	if ( WiFi.status() == WL_CONNECTED )
	{
		sendMessage( "Wifi is already connected, please disconnect before attempting to scan for available networks.", true );
		return false;
	}
	
	WiFi.disconnect(); //Just to be sure.
	WiFi.mode(WIFI_AP_STA);
	WiFi.enableAP(true);
	sendMessage( "Opening access point with SID: "+ String(ssid) );
	WiFi.softAP(ssid.c_str(), password.c_str()); //Set up the access point with our password and SSID name.
	IPAddress myIP = WiFi.softAPIP();
	sendMessage("IP address is :" + String(myIP) );
	setupServer(); //Host our pages
	return true;
}

void ESPComm::closeConnection( bool msg )
{
	if ( msg )
		sendMessage("Disconnecting/Closing Wifi connection.");
	
	WiFi.setAutoReconnect( false ); 
	WiFi.softAPdisconnect( false ); //Close the AP, if open.
	p_server->close(); //Stop the web server.
	WiFi.disconnect(); //Disconnect the wifi
}

void ESPComm::setupServer()
{
	//These set up our page triggers, linking them to specific functions.
	p_server->on("/config", std::bind(&ESPComm::HandleConfig, this) );
	p_server->on("/", std::bind(&ESPComm::HandleIndex, this) );
	p_server->on("/login", std::bind(&ESPComm::HandleLogin, this) );
	p_server->on("/admin", std::bind(&ESPComm::HandleAdmin, this) );
	//
	//p_server->begin();
};


void ESPComm::scanNetworks()
{
	if ( WiFi.status() == WL_CONNECTED )
	{
		sendMessage( "Wifi is already connected, please disconnect before attempting to scan for available networks.", true );
		return;
	}
		
	WiFi.mode(WIFI_STA);
	closeConnection( false ); //Just in case
	delay(100);
	int n = WiFi.scanNetworks(); //More than 255 networks in an area? Possible I suppose.
	if (!n)
		sendMessage("No networks found");
	else
	{
		sendMessage( String(n) + " networks found.");
		for (int i = 0; i < n; ++i)
		{
			// Print SSID and RSSI for each network found
			sendMessage( String( i ) + ": " + WiFi.SSID(i) + " (" +  WiFi.RSSI(i) + ")" + ( ( WiFi.encryptionType(i) == ENC_TYPE_NONE )?" ":"*" ), true );
			delay(10);
		}
	}
}

void ESPComm::beginConnection( String ssid, String password )
{
	uint8_t i_retries = 0;
	sendMessage("Attempting connection to: " + ssid + " using password " + password );
	WiFi.mode(WIFI_AP_STA);
	WiFi.begin( ssid.c_str(), /*password.c_str()*/"testing123" );
	while ( WiFi.status() != WL_CONNECTED && i_retries < i_timeoutLimit ) //We'll give it 10 seconds to try to connect?
	{
		sendMessage(".");
		delay(1000); 
		i_retries++;
	}
	if ( i_retries < i_timeoutLimit )
	{
		sendMessage("Connected to " + ssid + " with local IP: " + WiFi.localIP().toString() );
		p_server->begin(); //start the server
	}
	else 
	{
		sendMessage("Connection to " + ssid + " timed out.", true ); 
		closeConnection( false );
	}
}

void ESPComm::sendMessage( String str, bool force )
{
	if ( !b_verboseMode && !force ) //verbose mode disabled, and we're not forcing the message to send.
		return;
		
	Serial.println( str );
}
