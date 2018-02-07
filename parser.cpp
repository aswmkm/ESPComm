/*
 * parser.cpp
 *
 * Created: 2/1/2018 2:30:31 PM
 *  Author: Andrew
 * TODO: Reduce usage of String objects in favor of C-Style char arrays (memory safety).
 */ 

#include "common.h"
#include "arduino.h" //Used for serial.
#include <vector>
#include "ESPComm.h"

void ESPComm::parseSerialData()
{	
	char buffer[MAX_BUFFERSIZE] = { NULL_CHAR };
		
	if ( Serial.available() ) //Have something in the serial buffer.
	{
		Serial.readBytesUntil( END_TRANS, buffer, MAX_BUFFERSIZE - 1 ); //read into the buffer.
		Serial.println( buffer );
		
		for ( int pos = 0; pos < MAX_BUFFERSIZE - 1; pos++ )
		{
			if ( buffer[pos] == NULL_CHAR )
				break; //Assume that a null char is the end of the buffer.
				
			if ( buffer[pos] == CMD_PREFIX ) //Found our command indicator in the buffer
			{
				pos++; //increment our pos +1
				Serial.println("Found a cmd");
				if ( pos > MAX_BUFFERSIZE - 1 || buffer[pos] == NULL_CHAR )
					break; //Safety first. End here
					
				switch ( buffer[pos] )
				{
					case CMD_CONNECT: //For connecting to a wifi router ** Should be able to connect by SSID index as well. 
						parseConnect( parseArgs( pos, buffer ) );
						Serial.println("Called connect command");
						break;
					case CMD_LOGIN: //For logging into a SQL server
						Serial.println("Called login command");
						break;
					case CMD_DISCONNECT: //Disconnect from current wifi network
						Serial.println("Disconnecting/Closing Wifi connection.");
						WiFi.enableAP(false);
						p_server->close();
						WiFi.disconnect();
						break;
					case CMD_UPDATE: //For updating any necessary data fields that are displayed in the HTML pages
						//parseUpdateField( pos, buffer );
						Serial.println("Called update command");
						break;
					case CMD_CREATEFIELD: //For assigning a new data field, as well as a corresponding address number and type. 
						//parseCreateField( pos, buffer );
						Serial.println("Called CreateField command");
						break;
					case CMD_NETWORKS: //For listing all available networks
						scanNetworks();
						break;
					case CMD_AP: //Telling the ESP to become an access point "name":"password"
						Serial.println("Called AP command"); //Debug
						if( !parseAccessPoint( parseArgs( pos, buffer ) ) )
							Serial.println("Failed to create access point.");
						break;
				}
				
			}
		}
	}
}

vector<String> ESPComm::parseArgs( int &pos, char buffer[] ) //Here is where we split our buffer up into a series of strings, we'll read until the next CMD_PREFIX char.
{
	vector<String> args;
	bool quoteBegin = false;
	String tempStr;
	
	for ( pos, pos < MAX_BUFFERSIZE - 1; pos++; )
	{
		if ( buffer[pos] == CMD_PREFIX || buffer[pos] == NULL_CHAR ) //Start of a new command or end of the buffer.
		{
			if ( buffer[pos] == CMD_PREFIX )
				pos--; //Jump back a step so that other command can be interpreted in parseSerialData
				
			args.push_back( tempStr ); //TODO -- Make sure that the tempstr has some valid data inside, before adding it to the vector.
			return args; //Exit the function, and push our arguments.
		}
				
		if ( buffer[pos] == '"' ) //Check for the beginning of quotes to prohibit further filtering.
		{
			quoteBegin = !quoteBegin; //Toggle
			continue; //Skip this char and move on.
		}
		if ( buffer[pos] == ' ' && !quoteBegin) //Filter whitespaces if we're not between the quotes
			continue; //Skip
						
		if ( buffer[pos] == DATA_SPLIT ) ///*&& ( ( !quoteBegin && inQuotes ) || !inQuotes ) */)
		{
			args.push_back( tempStr ); //We've reached the splitter char for multiple data streams. 
			tempStr = ""; //Clear the string.
			quoteBegin = false; //Reset just in case
			continue; //Skip this char
		}	
		else 
			tempStr.concat( buffer[pos] ); //Add the char to our string.
	}
	
	return args;
}

bool ESPComm::parseAccessPoint( vector<String> args )
{
	for ( uint8_t x = 0; x < args.size(); x++ ) //Cycle through the parsed arguments for this command.
	{
		Serial.print(x);
		Serial.println( ": " + args[x] );//debug stuff
	}

	//<SSID>:<Password>
	if ( args.size() >= 2 ) //Make sure they exist, prevent crashing
		return setupAccessPoint( args[0], args[1] ); //Any other args will be discarded (not used)
	else 
	{
		Serial.println( "Not enough arguments to start AP.");
		return false;
	}
}

//For connecting to a wifi network, must be currently disconnected before connecting to a new network
void ESPComm::parseConnect( vector<String> args )
{
	//<SSID>:<Password>

	if ( args.size() >= 2 )
	{
		
		if ( args[0].c_str()[0] == '#' )//If we're using the number sign for the SSID input, we're indicating
		{
			unsigned int ssidIndex = parseInt(args[0]);
			if ( !ssidIndex || !WiFi.SSID( ssidIndex ).length() )
			{
				Serial.println("Invalid index ID - Cannot connect to network.");
				return;
			}
			beginConnection( WiFi.SSID(ssidIndex).c_str(), args[1].c_str() );
		}
		else
		{
			
			WiFi.begin(  );
			beginConnection( args[0].c_str(), args[1].c_str() );
		}
	}
	else
		Serial.println("Not enough arguments to connect to network.");
}

long parseInt( String str )
{
	String tempstr;
	for ( int x = 0; x < str.length(); x++ )
	{
		char tempChar = str.charAt(x);
		
		if ( tempChar > 47 && tempChar < 58 )//only add number chars to the string
			tempstr.concat( tempChar );
	}
	
	return tempstr.toInt(); //Will return 0 if buffer does not contain data. (safe)
}