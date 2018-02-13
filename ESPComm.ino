/*
Geekstips.com
IoT project - Communication between two ESP8266 - Talk with Each Other
ESP8266 Arduino code example
*/

#include "common.h"
#include "data_fields.h"
#include <vector>
#include "ESPComm.h"
#include "user_interface.h"

WiFiClient client;
MySQL_Connection SQL_Connection((Client *)&client);

ESPComm ESPDevice; //Device object init

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
	i_timeoutLimit = 15; //20 second default, will probably be an eeprom config later
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
	if ( WiFi.isConnected() )
	{
		sendMessage( "Connected to '" + WiFi.SSID() + "'. Disconnect before establishing an access point.", true );
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
	if ( msg ) //Sometimes this function will be called from within the code and not explicitly by the user.
	{
		if ( !WiFi.isConnected() )
		{
			sendMessage("Wifi is not currently connected.");
			return; //end here
		}
		else
		{
			sendMessage( "Closing connection to " + WiFi.SSID() );
		}
	}
	
	WiFi.setAutoReconnect( false ); 
	WiFi.softAPdisconnect( false ); //Close the AP, if open.
	p_server->close(); //Stop the web server.
	WiFi.disconnect(); //Disconnect the wifi
	WiFi.mode(WIFI_OFF);
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
	if ( WiFi.isConnected() )
	{
		sendMessage( "Connected to '" + WiFi.SSID() + "'. Disconnect before scanning for networks.", true );
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
	if ( WiFi.isConnected() )
	{
		sendMessage( "Connected to '" + WiFi.SSID() + "'. Disconnect before attempting a new connection.", true );
		return;
	}
	
	uint8_t i_retries = 0;
	sendMessage("Attempting connection to: " + ssid );
	WiFi.mode(WIFI_STA);
	
	if ( WiFi.begin( ssid.c_str(), password.c_str() ) == WL_CONNECT_FAILED )
	{
		sendMessage("Failed to begin Wifi connection (Invalid password or SSID?)", true );
		closeConnection();//Just in case
	}
	else 
	{
		while ( WiFi.status() != WL_CONNECTED && i_retries < i_timeoutLimit ) //We'll give it 10 seconds to try to connect?
		{
			/*station_status_t status = wifi_station_get_connect_status();
			if ( status == STATION_WRONG_PASSWORD )
			{
				sendMessage("Invalid password for " + ssid, true );
				return;
			}*/
			delay(1000); 
			i_retries++;
		}
		if ( WiFi.isConnected() )
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
}

void ESPComm::sendMessage( String str, bool force )
{
	if ( !b_verboseMode && !force ) //verbose mode disabled, and we're not forcing the message to send.
		return;
		
	Serial.println( str );
}

void ESPComm::printDiag()
{
	String stat; 
	switch( WiFi.status() )
	{
		case WL_CONNECTED:
			stat = "Connected";
			break;
		case WL_DISCONNECTED:
			stat = "Disconnected";
			break;
		case WL_CONNECTION_LOST:
			stat = "Connection lost";
			break;
		case WL_IDLE_STATUS:
			stat = "Idle";
			break;
		default:
			stat = String( WiFi.status() );
	}
	sendMessage( "Status: " + stat, true );
	sendMessage( "IP: " + WiFi.localIP().toString(), true );
	sendMessage( "Gateway: " + WiFi.gatewayIP().toString(), true );
	sendMessage( "Subnet: " + WiFi.subnetMask().toString(), true );
	sendMessage( "MAC: " + WiFi.macAddress(), true );
	
	sendMessage( "Available system memory: " + String(system_get_free_heap_size()) + " bytes.", true );
}