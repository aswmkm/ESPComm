
#include "Wifi.h" //Project specific header
#include "arduino.h"

		   
//Global stuff
WiFiClient wifi_ServerConnection; 
WiFiEventHandler disconnectedEventHandler;
//
void setup()
{
	/* add setup code here */
	EEPROM.begin(512); //default size for now. More than we need anyway
	Serial.begin(SERIAL_BAUD);
	
	loadDefaultConnection();
	
	WiFi.mode( WIFI_STA );//Station mode only, do not enable access point mode.
	WiFi.enableAP( false ); //Forcefully disable access point mode, just in case.
	WiFi.setAutoReconnect( true ); //Default to auto-reconnect
	
	disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
	{
		Serial.println(F("Network connection lost."));
		closeTCPConnection(); //Kill the TCP connection if applicable
	});
}

void loop()
{
	if ( wifi_ServerConnection.connected() && wifi_ServerConnection.available() ) //incoming from TCP port?
	{
		char tcpBuffer[MAX_BUFFERSIZE];
		wifi_ServerConnection.readBytesUntil('/r', tcpBuffer, MAX_BUFFERSIZE - 1 );
		Serial.print(tcpBuffer); //Forward to the serial port.
	}
	
	parseSerialData(); //Check for serial data input, and handle it.
}

void saveDefaultConnection()
{
	//Starting positions for each var
	uint8_t i_passWD = WIFI_SSID_LENGTH; // (0 + SSID )
	uint8_t i_TCPIP = i_passWD + WIFI_PASSWORD_LENGTH; // (SSID + PASSWORD )
	uint8_t i_Port = i_TCPIP + TCP_SERVERIP_LENGTH;// (IP + IP_LENGTH)
	//
	
	//if ( WiFi.isConnected() )
	//{
		char testPW[] = { "ASSHole Test" };
		for ( uint8_t x = 0; x < WIFI_SSID_LENGTH; x++ )
		{
			if ( x <= strlen(WiFi.SSID().c_str()))
				EEPROM.write(x, WiFi.SSID().c_str()[x] );
			else //Write blanks otherwise.
				EEPROM.write(x, NULL_CHAR );
		}
			
		for ( uint8_t x = 0; x < WIFI_PASSWORD_LENGTH; x++ )
		{
			if ( x <= strlen(testPW) )
				EEPROM.write(x + i_passWD, testPW[x] );
			else //Write blanks otherwise
				EEPROM.write(x + i_passWD, NULL_CHAR );
		}
		
		
		if ( wifi_ServerConnection.connected() ) //save TCP related stuff?
		{
			String ipStr = wifi_ServerConnection.localIP().toString();
			String portStr = String( wifi_ServerConnection.localPort() );
			
			for ( uint8_t x = 0; x < TCP_SERVERIP_LENGTH; x++ )
			{
				if ( x <= ipStr.length() )
					EEPROM.write(x + i_TCPIP, ipStr.c_str()[x] );
				else
					EEPROM.write(x + i_TCPIP, NULL_CHAR );
			}
			
			for ( uint8_t x = 0; x < TCP_PORT_LENGTH; x++ )
			{
				if ( x <= portStr.length() )
					EEPROM.write(x + i_Port, portStr.c_str()[x] );
				else
					EEPROM.write(x + i_Port, NULL_CHAR );
			}
		}
	//}
	EEPROM.commit(); //Write the data
}

void loadDefaultConnection()
{
	//Starting positions for each var
	uint8_t i_passWD = WIFI_SSID_LENGTH; // (0 + SSID )
	uint8_t i_TCPIP = i_passWD + WIFI_PASSWORD_LENGTH; // (SSID + PASSWORD )
	uint8_t i_Port = i_TCPIP + TCP_SERVERIP_LENGTH;// (IP + IP_LENGTH)
	//
	
	char c_tempIP[TCP_SERVERIP_LENGTH];
	char c_tempPort[TCP_PORT_LENGTH];
	char c_WiFi_SSID[WIFI_SSID_LENGTH];
	char c_WiFi_Password[WIFI_PASSWORD_LENGTH];
	
	for ( uint8_t x = 0; x < WIFI_SSID_LENGTH; x++ )
		c_WiFi_SSID[x] = char(EEPROM.read(x)); //This starts at x = 0
	
	for ( uint8_t x = 0; x < WIFI_PASSWORD_LENGTH; x++ )
		c_WiFi_Password[x] = char(EEPROM.read(x + i_passWD));
	
	for ( uint8_t x = 0; x < TCP_SERVERIP_LENGTH; x++ )
		c_tempIP[x] = char(EEPROM.read(x + i_TCPIP));
	
	for ( uint8_t x = 0; x < TCP_PORT_LENGTH; x++ )
		c_tempPort[x] = char(EEPROM.read(x + i_Port));
	
	//beginConnection( String(c_WiFi_SSID), String(c_WiFi_Password) );
	//if ( WiFi.isConnected() ) //Connection successful?
	//	beginTCPConnection( String(c_tempIP), parseInt( String(c_tempPort) ) );
	
	Serial.println(c_WiFi_SSID);
	Serial.println(c_WiFi_Password);
	Serial.println(c_tempIP);
	Serial.println(c_tempPort);
}


void parseSerialData()
{
	char buffer[MAX_BUFFERSIZE] = { NULL_CHAR };
	bool b_commandFound = false;
	
	if ( Serial.available() ) //Have something in the serial buffer?
	{
		Serial.readBytesUntil( NULL_CHAR, buffer, MAX_BUFFERSIZE - 1 ); //read into the buffer.
		Serial.println(String("Echo: ") + String(buffer) );
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
						b_skip= true;
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
					case CMD_PROGRAM:
						saveDefaultConnection();
						b_commandFound = true;
						break;
					default:
						continue; //Nothing here? just skip it.
				}	
			}
		}
		
		if ( !b_commandFound && WiFi.isConnected() && wifi_ServerConnection.connected() ) //if we haven't parsed any commands, just forward the buffer to the TCP port.
			wifi_ServerConnection.print( buffer );
	} 
}

vector<String> parseArgs( int &pos, const uint8_t &len, const char buffer[] ) //Here is where we split our buffer up into a series of strings, we'll read until the next CMD_PREFIX char.
{
	vector<String> args;
	bool quoteBegin = false;
	String tempStr;
	
	for ( pos, pos < len; pos++; )
	{
		bool b_stop = true;
		for ( uint8_t x = 0; x < strlen(CMD_PREFIX); x++ )
		{
			if ( buffer[pos + x] != CMD_PREFIX[x] )
			b_stop= false;
		}

		if ( b_stop || buffer[pos] == NULL_CHAR || pos >= len  ) //Start of a new command or end of the buffer.
		{
			if ( b_stop )
				pos -= strlen(CMD_PREFIX); //Jump back a step so that other command can be interpreted in parseSerialData
			
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
				args.push_back( tempStr ); //We've reached the splitter char for multiple data streams.
				tempStr = ""; //Clear the string.
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
	//<SSID>:<Password>:<special mode>
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
		Serial.println( F("Not enough arguments to connect to network.") );
}

void parseTCPConnect( const vector<String> &args )
{
	if ( args.size() >= 2 ) //Min args
	{
		beginTCPConnection( args[0], parseInt(args[1]) );
	}
	else
		Serial.println( F("Not enough args to connect to data server.") );
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
	Serial.println( PSTR("IP: ") + WiFi.localIP().toString() );
	Serial.println( PSTR("Gateway: ") + WiFi.gatewayIP().toString() );
	Serial.println( PSTR("Subnet: ") + WiFi.subnetMask().toString() );
	Serial.println( PSTR("MAC: ") + WiFi.macAddress() );
	
	//Serial.println( PSTR("Available ESP system memory: ") + String(system_get_free_heap_size()) + " bytes." );
}

void beginConnection( const String &ssid, const String &password )
{
	if ( WiFi.isConnected() )
	{
		Serial.println( "Connected to '" + WiFi.SSID() + "'. Disconnect before attempting a new connection." );
		return;
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
		Serial.println(F("Must be connected to network before establishing TCP connection."));
		
	closeTCPConnection(); //Force it to stop if already open.
		
	Serial.print(String("Connection to server at ") + addr + String(" at port ") + String(port) );
	if ( wifi_ServerConnection.connect( addr, port ) )
		Serial.println(" successful.");
	else 
		Serial.println(" failed.");
}

void closeTCPConnection()
{
	if ( wifi_ServerConnection.connected() ) //Already connected?
	{
		wifi_ServerConnection.stop(); //Then force it to stop
		Serial.println("Closing connection to server.");
	}
}

void scanNetworks()
{
	/*if ( WiFi.isConnected() )
	{
		sendMessage( "Connected to '" + WiFi.SSID() + "'. Disconnect before scanning for networks.", true );
		return;
	}*/
		
	//WiFi.mode(WIFI_AP_STA);
	//closeConnection( false ); //Just in case
	//delay(100);
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
	//WiFi.mode(WIFI_OFF);
}

void closeConnection( )
{
	if ( !WiFi.isConnected() )
	{
		Serial.println( F("Wifi is not currently connected.") );
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
	//WiFi.mode(WIFI_OFF);
}

