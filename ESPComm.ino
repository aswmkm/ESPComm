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
#include "time.h"

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
	//Init our objects/configs here.
	setupServer(); //Set up the web hosting directories.
	//Time object initialization
	p_currentTime = new Time;
	p_nextNISTUpdateTime = new Time;
	//
	
	//Default stuff for now, until EEPROM loading is implemented
	i_verboseMode = PRIORITY_LOW; //Do this by default for now, will probably have an eeprom setting for this later.
	i_timeoutLimit = 15; //20 second default, will probably be an eeprom config later
	b_enableNIST = true;
	b_enableAP = false;
	i_NISTupdateFreq = 5; //default to 5 minutes for now.
	i_NISTUpdateUnit = TIME_MINUTE; //default for now
	s_NISTServer = "time.nist.gov"; //Default for now.
	s_uniqueID = "DEFID"; //Default for now
	i_NISTPort = 13; //default
	//
	
	CreateAdminFields(); //Do this last - creates the data fields/tables for the ADMIN page.
}

void ESPComm::Process()
{
	parseSerialData(); //parse all incoming serial data.
	
	if ( WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() ) //Only do this stuff if we're connected to a network, or a client has connected to the AP
	{
		p_server->handleClient(); //Process stuff for clients that have connected.
	}
	
	updateClock(); //Update our stored system clock values;
}


void ESPComm::HandleLogin()
{
	p_server->send( 200, "text/html", "This is the login page.");
}

//THis function creates the access point if a network is unavailable for us to connect to. 
bool ESPComm::setupAccessPoint( const String &ssid, const String &password )
{
	/*if ( WiFi.isConnected() )
	{
		sendMessage( "Connected to '" + WiFi.SSID() + "'. Disconnect before establishing an access point.", PRIORITY_HIGH );
		return false;
	}*/
	
	WiFi.mode(WIFI_AP_STA);
	IPAddress Ip(192, 168, 1, 1); 
	IPAddress NMask(255, 255, 255, 0); 
	WiFi.softAPConfig(Ip, Ip, NMask);
	if ( WiFi.softAP(ssid.c_str(), password.c_str() ) ) //Set up the access point with our password and SSID name.
	{
		sendMessage( PSTR("Opening access point with SSID: ")+ ssid + " using password: " + password );
		IPAddress myIP = WiFi.softAPIP();
		sendMessage( PSTR("IP address: ") + myIP.toString(), PRIORITY_HIGH );
		p_server->begin();//Start up page server.
		return true;
	}
	return false;
}

void ESPComm::closeConnection( bool msg )
{
	if ( msg ) //Sometimes this function will be called from within the code and not explicitly by the user.
	{
		if ( !WiFi.isConnected() )
		{
			sendMessage( F("Wifi is not currently connected.") );
			return; //end here
		}
		else
		{
			sendMessage( PSTR("Closing connection to ") + WiFi.SSID() );
		}
	}
	WiFi.setAutoReconnect( false ); 
	WiFi.softAPdisconnect( true ); //Close the AP, if open.
	p_server->close(); //Stop the web server.
	WiFi.disconnect(); //Disconnect the wifi
	WiFi.mode(WIFI_OFF);
}

void ESPComm::setupServer()
{
	p_server = new ESP8266WebServer(80); //Open on port 80 (http)
	
	//These set up our page triggers, linking them to specific functions.
	p_server->on("/config", std::bind(&ESPComm::HandleConfig, this) );
	p_server->on("/", std::bind(&ESPComm::HandleIndex, this) );
	p_server->on("/login", std::bind(&ESPComm::HandleLogin, this) );
	p_server->on("/admin", std::bind(&ESPComm::HandleAdmin, this) );
	//
};


void ESPComm::scanNetworks()
{
	if ( WiFi.isConnected() )
	{
		sendMessage( "Connected to '" + WiFi.SSID() + "'. Disconnect before scanning for networks.", true );
		return;
	}
		
	WiFi.mode(WIFI_AP_STA);
	closeConnection( false ); //Just in case
	delay(100);
	int n = WiFi.scanNetworks(); //More than 255 networks in an area? Possible I suppose.
	if (!n)
		sendMessage( F("No networks found" ));
	else
	{
		sendMessage( String(n) + F(" networks found.") );
		for (int i = 0; i < n; ++i)
		{
			// Print SSID and RSSI for each network found
			sendMessage( String( i ) + ": " + WiFi.SSID(i) + " (" +  WiFi.RSSI(i) + ")" + ( ( WiFi.encryptionType(i) == ENC_TYPE_NONE )?" ":"*" ), true );
			delay(10);
		}
	}
	WiFi.mode(WIFI_OFF);
}

void ESPComm::beginConnection( const String &ssid, const String &password )
{
	if ( WiFi.isConnected() )
	{
		sendMessage( "Connected to '" + WiFi.SSID() + "'. Disconnect before attempting a new connection.", PRIORITY_HIGH );
		return;
	}
	
	sendMessage( PSTR("Attempting connection to: ") + ssid );
	WiFi.mode(WIFI_AP_STA);
	
	if ( WiFi.begin( ssid.c_str(), password.c_str() ) == WL_CONNECT_FAILED )
	{
		sendMessage( PSTR("Failed to begin Wifi connection (Invalid password or SSID?)") , true );
		closeConnection();//Just in case
	}
	else 
	{
		uint8_t i_retries = 0;
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
			sendMessage("Connection to " + ssid + " timed out.", PRIORITY_HIGH ); 
			closeConnection( false );
		}
	}
}

void ESPComm::sendMessage( const String &str, uint8_t priority )
{
	if ( i_verboseMode >= priority ) 
		Serial.println( str );
	else 
		return;
}

void ESPComm::printDiag()
{
	String stat; 
	switch( WiFi.status() )
	{
		case WL_CONNECTED:
			stat = F("Connected");
			break;
		case WL_DISCONNECTED:
			stat = F("Disconnected");
			break;
		case WL_CONNECTION_LOST:
			stat = F("Connection lost");
			break;
		case WL_IDLE_STATUS:
			stat = F("Idle");
			break;
		default:
			stat = String( WiFi.status() );
	}
	sendMessage( PSTR("Network Status: ") + stat, PRIORITY_HIGH );
	sendMessage( PSTR("IP: ") + WiFi.localIP().toString(), PRIORITY_HIGH );
	sendMessage( PSTR("Gateway: ") + WiFi.gatewayIP().toString(), PRIORITY_HIGH );
	sendMessage( PSTR("Subnet: ") + WiFi.subnetMask().toString(), PRIORITY_HIGH );
	sendMessage( PSTR("MAC: ") + WiFi.macAddress(), PRIORITY_HIGH );
	
	sendMessage( PSTR("Time Server Address: ") + s_NISTServer, PRIORITY_HIGH );
	sendMessage( PSTR("Time Update Interval (mins): ") + String(i_NISTupdateFreq) );
	sendMessage( PSTR("NIST Time Mode: ") + String(b_enableNIST) );
	
	sendMessage( PSTR("Available system memory: ") + String(system_get_free_heap_size()) + " bytes.", PRIORITY_HIGH );
}

void ESPComm::updateClock()
{
	unsigned long totalSeconds = millis()/1000;
	uint8_t tempSeconds = totalSeconds%60;
	
	if ( i_lastUpdateSecond == tempSeconds )
		return; //Same second, do not update yet.
		
	i_lastUpdateSecond = tempSeconds; //Save off our update second, since that's the fastest we update.	

	if ( UpdateNIST() ) //if we've updated the time, advance it next round.
		return;
		
	//System clock increase below.
	p_currentTime->IncrementTime( 1, TIME_SECOND );//1 second at a time.
}

bool ESPComm::UpdateNIST( bool force ) //TODO -- NTP?
{
	if ( WiFi.status() == WL_CONNECTED && b_enableNIST && s_NISTServer.length() ) //Must be on a network before attempting to connect to NIST server
	{
		if ( !i_NISTupdateFreq && !force ) //must be a non-zero value
			return false;
		
		if ( p_currentTime->IsBehind( p_nextNISTUpdateTime ) && !force ) //too soon for an update?
			return false;
		
		WiFiClient NISTclient;
		
		uint8_t retries = 0;
		while( !NISTclient.connect(s_NISTServer, i_NISTPort) )
		{
			if ( retries >= 5 )
			{
				sendMessage("Connection to NIST server: '" + s_NISTServer + "' failed.", PRIORITY_HIGH );
				b_enableNIST = false;
				return false;
			}
			retries++;
			delay(100); //small delay to allow the server to respond
		}
		sendMessage( F("Updating time." ) );
		
		delay(100); //small delay to allow the server to respond
			
		while( NISTclient.available() )
		{
			String line = NISTclient.readStringUntil('\r'); //DAYTIME protocol - meh, it works.
			if ( line.length() < 24 )
				return false; //to be safe
					
			//Break the string down into its components, then save.
			if ( !p_currentTime->SetTime( line.substring(7, 9).toInt(), line.substring(10, 12).toInt(), line.substring(13, 15).toInt(),
			 line.substring(16, 18).toInt(), line.substring(19, 21).toInt(), line.substring(22, 24).toInt() ) )
				return false;
				
			p_nextNISTUpdateTime->SetTime( p_currentTime ); //Replace with current time
		}
		
		p_nextNISTUpdateTime->IncrementTime( i_NISTupdateFreq, i_NISTUpdateUnit );
		
		return true; //End here, we'll let the system clock carry on on the next second.
	}
	return false; //default path
}