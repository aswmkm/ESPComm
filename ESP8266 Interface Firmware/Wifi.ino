
#include "Wifi.h" //Project specific header
#include "arduino.h"

		   
//Global stuff
WiFiClient wifi_ServerConnection; 
WiFiEventHandler disconnectedEventHandler, connectedEventHandler;
//
void setup()
{
	i_NumConnectRetries = 0; //init to 0
	/* add setup code here */
	Serial.begin(SERIAL_BAUD);
	
	WiFi.mode( WiFiMode_t::WIFI_STA ); //Enable station mode by default
	WiFi.enableAP( false ); //Forcefully disable access point mode, just in case.
	WiFi.setAutoReconnect( false ); //Default to auto-reconnect - off
	
	loadDefaultConnection(); //load stored server connections from eeprom
	
	disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
	{
		if ( i_NumConnectRetries >= 10 ) //This event is called even during auto reconnect attempts, attempt reconnection a maximum of 10 times before stopping.
		{
			WiFi.setAutoReconnect( false );
			Serial.println(F("Network connection lost."));
		}
		i_NumConnectRetries++; //Increment
		closeTCPConnection(); //Kill the TCP connection if applicable.
	});
	connectedEventHandler = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected& event)
	{
		i_NumConnectRetries = 0; //reset this upon successful connection
	});
	
}

void loop()
{
	if ( wifi_ServerConnection.connected() && wifi_ServerConnection.available() ) //incoming from TCP port?
	{
		char tcpBuffer[MAX_BUFFERSIZE] = { NULL_CHAR };
		wifi_ServerConnection.readBytesUntil(CR_CHAR, tcpBuffer, MAX_BUFFERSIZE - 1 );
		Serial.println(tcpBuffer); //Forward to the serial port.
	}
	
	parseSerialData(); //Check for serial data input, and handle it.
}
void saveDefaultConnection()
{
	EEPROM.begin(MAX_EEPROM_BYTES); //default size for now. More than we need anyway
	
	//Starting positions for each var
	EEPROM_Range StartingPos;
	//
	
	if ( WiFi.isConnected() )
	{
		struct station_config conf;
		wifi_station_get_config(&conf); //Fetch the current wifi configuration
		
		Serial.println(F("Saving WiFi credentials..."));
		for ( uint8_t x = 0; x < WIFI_SSID_LENGTH - 1; x++ )
			EEPROM.write(x + StartingPos.SSID, conf.ssid[x]);
			
		for ( uint8_t x = 0; x < WIFI_PASSWORD_LENGTH - 1; x++ )
			EEPROM.write(x + StartingPos.PASSWORD, conf.password[x] );
		
		
		if ( wifi_ServerConnection.connected() ) //save TCP related stuff?
		{
			String ipStr = wifi_ServerConnection.remoteIP().toString();
			String portStr = String( wifi_ServerConnection.remotePort() );
			Serial.println(F("Saving TCP server credentials..."));
			for ( uint8_t x = 0; x < TCP_SERVERIP_LENGTH - 1; x++ )
			{
				if ( x <= ipStr.length() )
					EEPROM.write(x + StartingPos.TCP_SERVERIP, ipStr.c_str()[x] );
				else
					EEPROM.write(x + StartingPos.TCP_SERVERIP, NULL_CHAR );
			}
			
			for ( uint8_t x = 0; x < TCP_PORT_LENGTH - 1; x++ )
			{
				if ( x <= portStr.length() )
					EEPROM.write(x + StartingPos.TCP_PORT, portStr.c_str()[x] );
				else
					EEPROM.write(x + StartingPos.TCP_PORT, NULL_CHAR );
			}
		}
	}
	else //Just clear the entire eeprom if there's nothing going on
	{
		Serial.println(F("No Connections Established. Clearing EEPROM settings."));
		for ( unsigned int x = 0; x < StartingPos.HOSTNAME - 1; x++ ) //We're NOT clearing the hostname area
			EEPROM.write(x, NULL_CHAR); 
	}
	EEPROM.end(); //commit changes, and end instance (free ram)
}

void loadDefaultConnection()
{
	EEPROM.begin(MAX_EEPROM_BYTES); //Begin EEPROM instance
	
	//Starting positions for each var
	EEPROM_Range StartingPos;
	//
	
	char c_TCPAddr[TCP_SERVERIP_LENGTH] = { NULL_CHAR };
	char c_TCPPort[TCP_PORT_LENGTH] = { NULL_CHAR };
	char c_WiFi_SSID[WIFI_SSID_LENGTH] = { NULL_CHAR };
	char c_WiFi_Password[WIFI_PASSWORD_LENGTH] = { NULL_CHAR };
	char c_WiFi_Hostname[WIFI_HOSTNAME_LENGTH] = { NULL_CHAR };
	
	for ( uint8_t x = 0; x < WIFI_SSID_LENGTH - 1; x++ )
	{
		char e_c = EEPROM.read( x + StartingPos.SSID );
		if ( e_c == NULL_CHAR )
			break;
			
		c_WiFi_SSID[x] = e_c; //This starts at x = 0
	}
	
	for ( uint8_t x = 0; x < WIFI_PASSWORD_LENGTH - 1; x++ )
	{
		char e_c = EEPROM.read( x + StartingPos.PASSWORD );
		if ( e_c == NULL_CHAR )
			break;
		
		c_WiFi_Password[x] = e_c;
	}
	
	for ( uint8_t x = 0; x < TCP_SERVERIP_LENGTH - 1; x++ )
	{
		char e_c = EEPROM.read( x + StartingPos.TCP_SERVERIP );
		if ( e_c == NULL_CHAR )
			break;
			
		c_TCPAddr[x] = e_c;
	}
	
	for ( uint8_t x = 0; x < TCP_PORT_LENGTH - 1; x++ )
	{
		char e_c = EEPROM.read(x + StartingPos.TCP_PORT );
		if ( e_c == NULL_CHAR )
			break;
			
		c_TCPPort[x] = e_c;
	}
	
	for ( uint8_t x = 0; x < WIFI_HOSTNAME_LENGTH - 1; x++ )
	{
		char e_c = EEPROM.read( x + StartingPos.HOSTNAME );
		if ( e_c == NULL_CHAR )
			break;
		
		c_WiFi_Hostname[x] = e_c;
	}
	
	if ( strlen(c_WiFi_Hostname) ) //set device hostname before we do anything else, if there's one to set
		wifi_station_set_hostname( c_WiFi_Hostname );
	
	if ( strlen(c_WiFi_SSID) ) //must at least have an SSID to connect to.
	{
		beginConnection( String(c_WiFi_SSID), String(c_WiFi_Password) );
		
		if ( WiFi.isConnected() && strlen(c_TCPPort) && strlen(c_TCPAddr) )
			beginTCPConnection( String(c_TCPAddr), parseInt( String(c_TCPPort) ) );
	}
	
	EEPROM.end(); //End our EEPROM instance here.
}


void parseSerialData()
{
	char buffer[MAX_BUFFERSIZE] = { NULL_CHAR };
	bool b_commandFound = false;
	
	if ( Serial.available() ) //Have something in the serial buffer?
	{
		Serial.readBytesUntil( NULL_CHAR, buffer, MAX_BUFFERSIZE - 1 ); //read into the buffer.
		uint8_t length = strlen(buffer);
		for ( int pos = 0; pos < length ; pos++ )
		{
			if ( buffer[pos] == NULL_CHAR )
				break; //Assume that a null char is the end of the buffer.
				
			if ( length >= (pos + (strlen(CMD_PREFIX) - 1)) ) //Found our command indicator in the buffer
			{
				bool b_skip = false;
				for ( uint8_t x = 0; x < strlen(CMD_PREFIX); x++ )
				{
					if ( buffer[pos + x] != CMD_PREFIX[x] )
					{
						b_skip= true;
						break;
					}
				}
				if ( b_skip )
					continue;
					
				pos += strlen(CMD_PREFIX); //increment our pos to skip over the CMD_PREFIX
				if ( pos >= length || buffer[pos] == NULL_CHAR )
					break; //Safety first. End here
				
				switch ( buffer[pos] )
				{
					case CMD_CONNECT: //For connecting to a wifi router ** Should be able to connect by SSID index as well.
						parseConnect( parseArgs( pos, length, buffer ) );
						b_commandFound = true;
						break;
					case CMD_DISCONNECT: //Disconnect from current wifi network
						closeConnection();
						b_commandFound = true;
						break;
					case CMD_NETWORKS: //For listing all available networks
						scanNetworks();
						b_commandFound = true;
						break;
					case CMD_NETINFO: //Status
						printDiag();
						b_commandFound = true;
						break;
					case CMD_SERVERINFO: //TCP server info <IP>:<Port>
						parseTCPConnect( parseArgs( pos, length, buffer ) );
						b_commandFound = true;
						break;
					case CMD_HOSTNAME:
						parseHostname( parseArgs( pos, length, buffer ) );
						b_commandFound = true;
						break;
					case CMD_PROGRAM:
						saveDefaultConnection();
						b_commandFound = true;
						break;
					default:
						continue; //Nothing here? just skip it.
				}	
			}
		}
		
		if ( !b_commandFound )
		{
			if ( WiFi.isConnected() && wifi_ServerConnection.connected() ) //if we haven't parsed any commands, just forward the buffer to the TCP port.
				wifi_ServerConnection.print( buffer );
			else
				Serial.println("Unknown Command: " + String(buffer) );
		}
	} 
}

vector<String> parseArgs( int &pos, const uint8_t &len, const char buffer[] ) //Here is where we split our buffer up into a series of strings, we'll read until the next CMD_PREFIX char.
{
	vector<String> args;
	bool quoteBegin = false;
	String tempStr;
	
	for ( pos, pos < len; pos++; ) //We're working through, one char at a time.
	{
		bool b_stop = false;
		if ( !quoteBegin ) //Don't look for command prefixes if we're within quotes
		{
			bool b_test = true; //If our next two chars in the buffer are a command prefix, this will remain true.
			for ( uint8_t x = 0; x < strlen(CMD_PREFIX); x++ )
			{
				if ( buffer[pos + x] != CMD_PREFIX[x] ) 
				{
					b_test = false; //mismatched char, set to false this pass.
					break; //if any chars don't match, it's a fail, so stop here.
				}
			}
			b_stop = b_test; //Save.
		}

		if ( b_stop || buffer[pos] == NULL_CHAR || pos >= len  ) //Start of a new command or end of the buffer.
		{
			if ( b_stop )
				pos -= strlen(CMD_PREFIX); //Jump back a step so that other command can be interpreted in parseSerialData
			
			if ( tempStr.length() ) //only push_back if there's valid data inside
				args.push_back( tempStr );
			return args; //Exit the function, and push our arguments.
		}
		
		if ( buffer[pos] == '"' ) //Check for the beginning of quotes to prohibit further filtering.
		{
			quoteBegin = !quoteBegin; //Toggle
			continue; //Skip this char and move on
		}
		
		if ( !quoteBegin ) //Assuming we're not in quotes here.
		{
			if ( ( buffer[pos] <= 32 || buffer[pos] > 126 ) ) //Filter all non-printable ascii chars and whitespace if we're not between the quotes
				continue; //Skip
			
			if ( buffer[pos] == DATA_SPLIT  ) //DATA_SPLIT can be a valid char if in quotes.
			{
				if ( tempStr.length() ) //only push_back if there's valid data inside
				{
					args.push_back( tempStr ); //We've reached the splitter char for multiple data streams.
					tempStr = ""; //Clear the string.
				}
				continue; //Skip this char
			}
		}

		tempStr.concat( buffer[pos] ); //Add the char to our string if we've come this far.
	}
	
	return args;
}


//For connecting to a wifi network, must be currently disconnected before connecting to a new network
void parseConnect( const vector<String> &args )
{
	//<SSID>:<Password>:<special mode(Optional)>
	if ( args.size() >= 2 ) //Minimum args.
	{
		if ( args.size() >= 3 ) //More than 2 args?
		{
			if ( args[2] == "#" )//Connect by index option. This only works if a scan has already been performed.
			{
				unsigned int ssidIndex = parseInt(args[0]);
				int8_t indexes = WiFi.scanComplete();
				if ( !ssidIndex || indexes <= 0 || ssidIndex > indexes )
				{
					Serial.println( "Invalid SSID index: " + String(ssidIndex) );
					return;
				}
				beginConnection( WiFi.SSID(ssidIndex), args[1] );
				return;
			}
			else if ( args[2] == "*" ) //Wild-card connection option.
			{
				WiFi.scanNetworks(); //Build a list of all available networks first.
				int8_t numResults = WiFi.scanComplete(); //We need to build an index for all available nearby networks first
				if ( numResults > 0 ) //It's possible to have negative numbers here (error codes)
				{
					for ( uint8_t i = 0; i < numResults; i++ )
					{
						if ( !strncmp( args[0].c_str(), WiFi.SSID(i).c_str(), args[0].length() ) ) //Returns 0 if strings are equal
						{
							beginConnection( WiFi.SSID(i), args[1] );
							return;
						}
					}
					Serial.println( F("No valid SSID match found using wild-card.") );
					return;
				}
				else
					Serial.println( F("Scan returned no available networks, cannot connect via wild-card.") );
				
				return; //if we've made it this far, we're probably going nowhere. just end.
			}
		}
		
		beginConnection( args[0], args[1] ); //Normal connection method
	}
	else
		Serial.println( F("Usage: <SSID>:PASSWORD> - See documentation for additional information.") );
}

void parseTCPConnect( const vector<String> &args )
{
	if ( args.size() >= 2 ) //Min args <Address>:<Port>
	{
		unsigned int port = parseInt( args[1] );
		
		if ( port > MAX_TCP_PORT )
			Serial.println("Invalid Port Number. Valid Range: 0-" + String(MAX_TCP_PORT));
		else
			beginTCPConnection( args[0], port );
	}
	else
		Serial.println( F("Usage: <SERVER ADDRESS>:<PORT>" ) );
}

void parseHostname( const vector<String> &args )
{
	if ( args.size() >= 1 ) //must have at least one arg
	{
		if ( WiFi.isConnected() )
			Serial.println(F("Disconnect from WiFi before setting a new device hostname."));
		else
		{
			char hostname[WIFI_HOSTNAME_LENGTH] = { NULL_CHAR };
			EEPROM_Range StartingPos;
			args[0].toCharArray( hostname, WIFI_HOSTNAME_LENGTH - 1 );//Store into char array
				
			if ( wifi_station_set_hostname( hostname ) )
			{
				EEPROM.begin(MAX_EEPROM_BYTES);
				Serial.println("Setting hostname to: " + args[0] );
			
				//Also program the hostname into the EEPROM for future uses.
				for ( uint8_t x = 0; x < WIFI_HOSTNAME_LENGTH - 1; x++ )
				{
					if ( x < strlen(hostname) )
						EEPROM.write( x + StartingPos.HOSTNAME, hostname[x] );
					else
						EEPROM.write( x + StartingPos.HOSTNAME, NULL_CHAR );	
				}
				EEPROM.end(); //commit changes, and end instance (free ram)
			}
			else
				Serial.println("Failed to set hostname to: " + args[0] );
		}	
	}
	else
		Serial.println( F("Usage: <HOSTNAME>") );
}

long parseInt( const String &str )
{
	String tempstr;
	for ( int x = 0; x < str.length(); x++ )
	{
		char tempChar = str.charAt(x);
		
		if ( tempChar > 47 && tempChar < 58 || tempChar == 45 )//only add number chars to the string, also the negative sign
		tempstr.concat( tempChar );
	}
	
	return tempstr.toInt(); //Will return 0 if buffer does not contain data. (safe)
}

void printDiag()
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
	
	Serial.println( PSTR("Network Status: ") + stat );
	if ( WiFi.isConnected() )
	{
		Serial.println( PSTR("SSID: ") + WiFi.SSID() );
		Serial.println( PSTR("IP: ") + WiFi.localIP().toString() );
		Serial.println( PSTR("Gateway: ") + WiFi.gatewayIP().toString() );
		Serial.println( PSTR("Subnet: ") + WiFi.subnetMask().toString() );
		Serial.println( "Hostname: " + String(wifi_station_get_hostname()) );
	}
	Serial.println( PSTR("MAC: ") + WiFi.macAddress() );
	if ( wifi_ServerConnection.connected() )
	{
		Serial.println( String("TCP/IP Server: ") + wifi_ServerConnection.remoteIP().toString() );
		Serial.println( String("TCP/IP Port: ") + wifi_ServerConnection.remotePort() );
	}
	Serial.println( "Available system RAM: " + String(system_get_free_heap_size()) + " bytes.");
	
	//Serial.println( PSTR("Available ESP system memory: ") + String(system_get_free_heap_size()) + " bytes." );
}

void beginConnection( const String &ssid, const String &password )
{
	if ( WiFi.isConnected() )
	{
		Serial.println( "Connected to '" + WiFi.SSID() + "'. Disconnect before attempting a new connection." );
		return;
	}
	
	if( WiFi.getMode() == WiFiMode_t::WIFI_OFF )
	{
		if ( !WiFi.mode( WIFI_STA ) ) //enable station mode only, do not enable access point mode.
		{
			Serial.println( F("Fatal Error: Failed to enable WiFi device.") );
			return;
		}
	}
		
	Serial.println( "Attempting connection to: " + ssid );

	if ( WiFi.begin( ssid.c_str(), password.c_str() ) == WL_CONNECT_FAILED )
	{
		Serial.println( PSTR("Failed to begin Wifi connection (Invalid password or SSID?)") );
		closeConnection();//Just in case
	}
	else
	{
		uint8_t i_retries = 0;
		while ( WiFi.status() != WL_CONNECTED && i_retries < CONNECT_RETRIES ) //We'll give it 10 seconds to try to connect?
		{
			delay(1000); //Every second
			i_retries++;
		}
		if ( WiFi.isConnected() )
		{
			Serial.println("Connected to " + ssid + " with local IP: " + WiFi.localIP().toString() );
			WiFi.setAutoReconnect( true );
		}
		else
		{
			Serial.println("Connection to " + ssid + " timed out." );
			closeConnection(); //Just to be safe
		}
	}
}

void beginTCPConnection( const String &addr, unsigned int port )
{
	if ( !WiFi.isConnected() )
	{
		Serial.println(F("Must be connected to network before establishing TCP connection."));
		return;
	}
		
	closeTCPConnection(); //Force it to stop if already open.
		
	Serial.print(String("Connection to server at ") + addr + String(" at port ") + String(port) );
	if ( wifi_ServerConnection.connect( addr, port ) )
	{
		Serial.println(" successful.");
		wifi_ServerConnection.keepAlive(); //keep the connection active
	}
	else 
		Serial.println(" failed.");
}

void closeTCPConnection()
{
	if ( wifi_ServerConnection.connected() ) //Already connected?
	{
		wifi_ServerConnection.disableKeepAlive();
		wifi_ServerConnection.stop(); //Then force it to stop
		Serial.println(F("Closing connection to server."));
	}
}

void scanNetworks()
{
	int n = WiFi.scanNetworks(); //More than 255 networks in an area? Possible I suppose.
	if (!n)
		Serial.println( F("No networks found" ) );
	else
	{
		Serial.println( String(n) + F(" networks found.") );
		for (int i = 0; i < n; ++i)
		{
			// Print SSID and RSSI for each network found
			Serial.println( String( i ) + ": " + WiFi.SSID(i) + " (" +  WiFi.RSSI(i) + ")" + ( ( WiFi.encryptionType(i) == ENC_TYPE_NONE )?" ":"*" ) );
			delay(10);
		}
	}
}

void closeConnection( )
{
	if ( !WiFi.isConnected() )
	{
		Serial.println( F("WiFi is not currently connected.") );
		return; //end here
	}
	else
	{
		Serial.println( PSTR("Closing connection to ") + WiFi.SSID() );
	}
	
	closeTCPConnection(); //End the TCP connection if it exists.
	WiFi.setAutoReconnect( false );
	//WiFi.softAPdisconnect( true ); //Close the AP, if open.
	WiFi.disconnect(); //Disconnect the wifi
	//WiFi.mode(WiFiMode_t::WIFI_OFF);
}

