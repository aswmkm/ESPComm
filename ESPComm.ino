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
	setupServer(); //Set up the web hosting directories.
	i_verboseMode = PRIORITY_LOW; //Do this by default for now, will probably have an eeprom setting for this later.
	i_timeoutLimit = 15; //20 second default, will probably be an eeprom config later
	b_enableNIST = true;
	i_NISTupdateFreq = 1; //default to 1 minute for now.
	s_NISTServer = "time.nist.gov"; //Default for now.
}

void ESPComm::Process()
{
	parseSerialData(); //parse all incoming serial data.
	
	if ( WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() ) //Only do this stuff if we're connected to a network, or a client has connected
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
		sendMessage( "Opening access point with SSID: "+ ssid + " using password: " + password );
		IPAddress myIP = WiFi.softAPIP();
		sendMessage("IP address: " + myIP.toString(), PRIORITY_HIGH );
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
			sendMessage("Wifi is not currently connected.");
			return; //end here
		}
		else
		{
			sendMessage( "Closing connection to " + WiFi.SSID() );
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
	WiFi.mode(WIFI_OFF);
}

void ESPComm::beginConnection( const String &ssid, const String &password )
{
	if ( WiFi.isConnected() )
	{
		sendMessage( "Connected to '" + WiFi.SSID() + "'. Disconnect before attempting a new connection.", PRIORITY_HIGH );
		return;
	}
	
	sendMessage("Attempting connection to: " + ssid );
	WiFi.mode(WIFI_AP_STA);
	
	if ( WiFi.begin( ssid.c_str(), password.c_str() ) == WL_CONNECT_FAILED )
	{
		sendMessage("Failed to begin Wifi connection (Invalid password or SSID?)", true );
		closeConnection();//Just in case
	}
	else 
	{
		uint8_t i_retries = 0;
		unsigned long i_delayMS = ( millis() + 1000); //Current time + 1 second 
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
	sendMessage( "Network Status: " + stat, PRIORITY_HIGH );
	sendMessage( "IP: " + WiFi.localIP().toString(), PRIORITY_HIGH );
	sendMessage( "Gateway: " + WiFi.gatewayIP().toString(), PRIORITY_HIGH );
	sendMessage( "Subnet: " + WiFi.subnetMask().toString(), PRIORITY_HIGH );
	sendMessage( "MAC: " + WiFi.macAddress(), PRIORITY_HIGH );
	
	sendMessage( "Time Server Address: " + s_NISTServer, PRIORITY_HIGH );
	sendMessage( "Time Update Interval (mins): " + String(i_NISTupdateFreq) );
	sendMessage( "NIST Time Mode: " + String(b_enableNIST) );
	
	sendMessage( "Available system memory: " + String(system_get_free_heap_size()) + " bytes.", PRIORITY_HIGH );
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
	i_second++;
	if ( !(i_second%60) )
	{
		i_second = 0;
		i_minute++;
		if ( !(i_minute%60) )
		{
			i_minute = 0;
			i_hour++;
			if ( !(i_hour%24) )
			{
				i_hour = 0;
				i_day++;
				uint8_t mod = 0; //variable modulus
				switch( i_month )
				{
					case 4:
					case 6:
					case 9:
					case 11:
						mod = 30;
						break;
					case 2:
						mod = 28;
						break;
					default: //all others
						mod = 31;
						break;
				}
				if ( !(i_day%mod) )
				{
					i_day = 0;
					i_month++;
					if ( !(i_month%12) )
					{
						i_month = 0;
						i_year++; //We don't roll over years.
					}
				}
			}
		}
	}
}

bool ESPComm::UpdateNIST( bool force ) //TODO -- NTP?
{
	if ( WiFi.status() == WL_CONNECTED && b_enableNIST && s_NISTServer.length() ) //Must be on a network before attempting to connect to NIST server
	{
		if ( !i_NISTupdateFreq && !force ) //must be a non-zero value
			return false;
		
		unsigned long currentTime = i_minute; //we're basing this on minutes
		if ( i_hour )
			currentTime *= i_hour;
		if ( i_day )
			currentTime *= i_day;
		if ( i_month )
			currentTime *= i_month;
		if ( i_year )
			currentTime *= i_year;
		
		if ( currentTime < ( i_lastNISTupdate + i_NISTupdateFreq ) && !force ) //too soon for an update?
			return false;
		else 
			i_lastNISTupdate = currentTime;
		
		WiFiClient NISTclient;
		const uint8_t httpPort = 13;
		
		uint8_t retries = 0;
		while( !NISTclient.connect(s_NISTServer, httpPort) )
		{
			if ( retries >= 5 )
			{
				sendMessage("Connection to NIST server: '" + s_NISTServer + "' failed.", PRIORITY_HIGH );
				b_enableNIST = false;
				return false;
			}
			retries++;
		}
		sendMessage("Updating time.");
		delay(100); //small delay to allow the server to respond
			
		while( NISTclient.available() )
		{
			String line = NISTclient.readStringUntil('\r'); //DAYTIME protocol - meh, it works.
			if ( line.length() < 24 )
				return false; //to be safe
					
			//Break the string down into its components, then save.
			i_year = line.substring(7, 9).toInt();
			i_month = line.substring(10, 12).toInt();
			i_day= line.substring(13, 15).toInt();
			i_hour = line.substring(16, 18).toInt();
			i_minute = line.substring(19, 21).toInt();
			i_second = line.substring(22, 24).toInt();
		}
			
		return true; //End here, we'll let the system clock carry on on the next second.
	}
	return false; //default path
}