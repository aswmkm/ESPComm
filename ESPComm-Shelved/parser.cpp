/*
 * parser.cpp
 *
 * Created: 2/1/2018 2:30:31 PM
 *  Author: Andrew
 * TODO: Reduce usage of String objects in favor of C-Style char arrays (memory safety).
 */ 

#include "arduino.h" //Used for serial.
#include "ESPComm.h"

void ESPComm::parseSerialData()
{	
	char buffer[MAX_BUFFERSIZE] = { NULL_CHAR };
		
	if ( Serial.available() ) //Have something in the serial buffer?
	{
		Serial.readBytesUntil( NULL_CHAR, buffer, MAX_BUFFERSIZE - 1 ); //read into the buffer.
		uint8_t length = strlen(buffer);
		for ( int pos = 0; pos < length ; pos++ )
		{
			if ( buffer[pos] == NULL_CHAR )
				break; //Assume that a null char is the end of the buffer.
				
			if ( buffer[pos] == CMD_PREFIX ) //Found our command indicator in the buffer
			{
				pos++; //increment our pos +1
				if ( pos >= length || buffer[pos] == NULL_CHAR )
					break; //Safety first. End here
					
				switch ( buffer[pos] )
				{
					case CMD_CONNECT: //For connecting to a wifi router ** Should be able to connect by SSID index as well. 
						parseConnect( parseArgs( pos, length, buffer ) );
						break;
					case CMD_LOGIN: //For logging into a SQL server
						Serial.println("Called login command (NOT IMPLEMENTED)");
						break;
					case CMD_PROGRAM:
						parseEEPROMCfg( parseArgs( pos, length, buffer ) );
						break;
					case CMD_DISCONNECT: //Disconnect from current wifi network
						closeConnection();
						break;
					case CMD_UPDATE: //For updating any necessary data fields that are displayed in the HTML pages
						parseUpdate( parseArgs( pos, length, buffer ) );
						break;
					case CMD_CREATEFIELD: //For assigning a new data field, as well as a corresponding address number and type. 
						parseDataFields( parseArgs( pos, length, buffer ) );
						break;
					case CMD_NETWORKS: //For listing all available networks
						scanNetworks();
						break;
					case CMD_AP: //Telling the ESP to become an access point "name":"password"
						parseAccessPoint( parseArgs( pos, length, buffer ) );
						break;
					case CMD_VERBOSE: //Enable/disable verbose mode
						parseVerbose( parseArgs( pos, length, buffer ) );
						break;
					case CMD_NETINFO: //Status
						printDiag();
						break;
					case CMD_TIME:
						parseTime( parseArgs( pos, length, buffer ) );
						break;
					default:
						continue; //Nothing here? just skip it.
				}
				
			}
		}
	}
}

vector<String> ESPComm::parseArgs( int &pos, const uint8_t &len, const char buffer[] ) //Here is where we split our buffer up into a series of strings, we'll read until the next CMD_PREFIX char.
{
	vector<String> args;
	bool quoteBegin = false;
	String tempStr;
	
	for ( pos, pos < len; pos++; )
	{
		if ( buffer[pos] == CMD_PREFIX || buffer[pos] == NULL_CHAR || pos >= len ) //Start of a new command or end of the buffer.
		{
			if ( buffer[pos] == CMD_PREFIX )
				pos--; //Jump back a step so that other command can be interpreted in parseSerialData
				
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

bool ESPComm::parseAccessPoint( const vector<String> &args )
{
	//<SSID>:<Password>
	if ( args.size() >= 2 ) //Make sure they exist, prevent crashing
		return setupAccessPoint( args[0], args[1] ); //Any other args will be discarded (not used)
	else
		sendMessage( PSTR("Not enough arguments to initialize access point."), PRIORITY_HIGH );
		return false;
}

//For connecting to a wifi network, must be currently disconnected before connecting to a new network
void ESPComm::parseConnect( const vector<String> &args )
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
					sendMessage( "Invalid SSID index: " + String(ssidIndex), PRIORITY_HIGH );
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
					sendMessage( F("No valid SSID match found using wild-card."), PRIORITY_HIGH );
					return;
				}
				else
					sendMessage( F("Scan returned no available networks, cannot connect via wild-card."), PRIORITY_HIGH  );
				
				return; //if we've made it this far, we're probably going nowhere. just end.
			}	
		}
		 
		beginConnection( args[0], args[1] ); //Normal connection method
	}
	else
		sendMessage( F("Not enough arguments to connect to network."), PRIORITY_HIGH );
}

void ESPComm::parseVerbose( const vector<String> &args )
{
	if ( args.size() ) //Make sure we've got some data.
	{ 
		uint8_t value = parseInt( args[0] ); //See if we have a numeric value 
		if ( value )
		{
			if ( value > VERBOSE_MAX )
				value = VERBOSE_MAX; //cap
				
			i_verboseMode = value;
		}
		else if ( args[0] == "on" )// specified "on" - just default to show all messages
			i_verboseMode = PRIORITY_LOW;
		else if ( !value || args[0] == "off" )
			i_verboseMode = 0;
	}
}

void ESPComm::parseDataFields( const vector<String> &args ) // /f<address><type_num><Label Text*><method name*><default value text*>
{	
	if ( args.size() >= 2 )	//Must have at least 2 fields.
	{
		int address = parseInt( args[0] );
		int8_t type = parseInt( args[1] ); //can also be negative*
		
		if ( address ) //Address must be a non 0 value.
		{
			for ( uint8_t x = 0; x < p_dataFields.size(); x++ )
			{
				if ( p_dataFields[x]->GetAddress() == address ) //Address exists?
				{
					if ( type == FIELD_TYPE::REMOVE ) //Looking to remove it?
					{
						delete p_dataFields[x];//delete
						p_dataFields.erase( p_dataFields.begin() + x );
						sendMessage("Field: " + String(address) + " removed." );
					}
					else
						sendMessage( F("Address detected in existing data field."), PRIORITY_HIGH ); //duplicate address being created
						
					return; //Die here.
				}
			}
			
			if ( type < FIELD_TYPE::NONE ) //Could be a remove command or something.
				return;
			
			DataField *newField;
			switch ( args.size() ) //varying number of inputs is possible.
			{
				case 2: //Min fields necessary.
					newField = new DataField( address, type );
					break;
				case 3:
					newField = new DataField( address, type, args[2] );
					break;
				case 4:
					newField = new DataField( address, type, args[2], args[3] );
					break;
				case 5://MAX number of fields for now
					newField = new DataField( address, type, args[2], args[3], args[4] );
					break;
			}
			p_dataFields.push_back( newField ); //Add it to the vector.
		}
	}
	else if ( args.size() && args[0] == "*" && p_dataFields.size() ) //Print some diag info.
	{
		String tempStr = F("Data Fields at Indexes: ");
		for ( uint8_t x = 0; x < p_dataFields.size(); x++ )
			tempStr += String( p_dataFields[x]->GetAddress() ) + ",";
		sendMessage( tempStr );
	}
	else
		sendMessage( F("Not enough valid arguments to create new data field." ) );
}

void ESPComm::parseUpdate( const vector<String> &args ) // /u<Index><Data>
{
	uint8_t totalArgs = args.size();
	if ( totalArgs >= 2 ) //For now, other args are discarded.
	{
		for (uint8_t x = 0; x < totalArgs; x++ )
		{
			int address = parseInt( args[x] );
			if ( address && totalArgs >= (x+1) )
			{
				x++;
				for ( uint8_t i = 0; i < p_dataFields.size(); i++ )
				{
					if ( p_dataFields[i]->GetAddress() == address )
					{
						p_dataFields[i]->SetFieldValue( args[x] ); //Set the new data
						return; //End here.
					}
				}
				sendMessage( "Failed to make any update to field with index: " + String(address) );
			}
		}
	}
	sendMessage( F("Not enough valid arguments for update." ));
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


void ESPComm::parseEEPROMCfg( vector<String> args )
{
	
}

void ESPComm::parseTime( const vector<String> &args )
{
	uint8_t totalArgs = args.size();
		
	if ( totalArgs )
	{
		for ( uint8_t x = 0; x < totalArgs; x++ )
		{
			if ( args[x] == "n" && totalArgs >= (x+1) ) //enabledisable
			{
				x++; //Move to next element.
				uint8_t value = parseInt( args[x] );
				if ( value )
					b_enableNIST = true;
				else
					b_enableNIST = false;
			}
			else if ( args[x] == "u" ) //forced update
			{
				if ( !UpdateNIST(true) )
					sendMessage(F("Failed to update NIST time."));
			}
			else if ( args[x] == "f" && totalArgs >= (x+1) ) //frequency
			{
				x++; //Move to next element.
				i_NISTupdateFreq = parseInt( args[x] );
			}
			else if ( args[x] == "s" && totalArgs >= (x+1) ) //nist server name/ip
			{
				x++; //Move to next element.
				s_NISTServer = args[x];
			}
			else if ( args[x] == "t" && totalArgs >= (x+6) ) //manual time entry (requires year, month, day, hour, min, and sec)
			{
				p_currentTime->SetTime(parseInt(args[x + 1]), parseInt(args[x + 2]), parseInt(args[x + 3]), parseInt(args[x + 4]), parseInt(args[x + 5]), parseInt(args[x + 6]) );
				x += 6; //advance
			}
			else if ( args[x] == "z" && totalArgs >= (x+1) ) //zone
			{
				x++;
				p_currentTime->SetTimeZone(parseInt( args[x] ));
			}
			else
				sendMessage( p_currentTime->GetTimeStr(), PRIORITY_HIGH );
		}
	}	
}

