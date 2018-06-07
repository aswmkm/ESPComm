/*
 * Wifi.h
 *
 * Created: 5/18/2018 7:32:52 AM
 *  Author: Andrew Ward
 * This file contains the function definitions for the wifi adapter firmware, as well as a basic summary of their functionality.
 */ 


#ifndef WIFI_H_
#define WIFI_H_

#include <vector> //Vectors for the win
#include <WString.h> //String stuff
//Wifi device related stuff
#include <WiFiUdp.h>
#include <WiFiServerSecure.h>
#include <WiFiServer.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ESP8266WiFiType.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFi.h>
#include <user_interface.h> //access to some lower level functionality
//

#include <EEPROM.h> //EEPROM stuff.

using namespace std;

#define MAX_BUFFERSIZE 64 //limited by default max serial buffer size on arduino
#define NULL_CHAR '\0'
#define CR_CHAR '\r'
#define SERIAL_BAUD 115200 //This seems adequate.
#define MAX_TCP_PORT UINT16_MAX //highest number of addressable ports available for TCP/IP communication, basically a 16 bit int.
#define CONNECT_RETRIES 10

//EEPROM settings -- DO NOT MODIFY THESE UNLESS YOU KNOW WHAT YOU'RE DOING.
#define MAX_EEPROM_BYTES 255 //Maximum addressable by uint8_t - could be expanded further with different type

#define WIFI_SSID_LENGTH 32
#define WIFI_PASSWORD_LENGTH 64
#define WIFI_HOSTNAME_LENGTH 32
#define TCP_SERVERIP_LENGTH 64 //Maximum length for string that stores IP address (also host names).
#define TCP_PORT_LENGTH 5 //max chars needed for port number

struct EEPROM_Range
{
	EEPROM_Range() //Set up the defaults here. REMEMBER, THESE ARE THE STARTING POSITIONS FOR EACH SETTING
	{
		SSID = 0; 
		PASSWORD = SSID + WIFI_SSID_LENGTH; //After SSID
		TCP_SERVERIP = PASSWORD + WIFI_PASSWORD_LENGTH; //After PASSWORD
		TCP_PORT = TCP_SERVERIP + TCP_SERVERIP_LENGTH; //so on
		HOSTNAME = TCP_PORT + TCP_PORT_LENGTH; //Hostname should be last, until I can come up with a more intuitive way of handling this data.
	}
	uint8_t SSID,
			PASSWORD,
			TCP_SERVERIP,
			TCP_PORT,
			HOSTNAME;
}; 
// -- END OF EEPROM SETTINGS

const char CMD_PREFIX[] =  {"AT"}, // -- Attention
		DATA_SPLIT = ':'; //Char used to split multiple strings of data in a serial commamnd stream

const char CMD_DISCONNECT = 'D',
		CMD_CONNECT = 'C', //"SSID":"Password"
		CMD_NETWORKS = 'F', //find networks
		CMD_PROGRAM = 'P', //Used to program the default SSID, password, TCP Server IP, and TCP Port -- In that order. No args = program current.
		CMD_SERVERINFO = 'S', //TCP server info <IP>:<PORT>
		CMD_HOSTNAME = 'H', //Used for saving the device hostname (networking related) 
		CMD_IGNORE = 'X', //Command that tells the parser to exit early, and forward the buffer to the TCP port (optimization)
		CMD_NETINFO = 'I'; //request current network info <if connected>
		
uint8_t i_NumConnectRetries;

//Function definitions below:
//Descr: This function checks to see if there is data available to read from the hardware serial, and parses/distributes it accordingly. 
//Output: returns false if no valid command was found, otherwise true.
void parseSerialData();

//Descr: This function does the actual parsing of arguments after the serial data is read, before distributing them to the proper functions for execution.
//Inputs: Position from which to read in the char buffer, 
vector<String> parseArgs( int &, const uint8_t &, const char buffer[] );

//Descr: This function handles the arguments for connecting to a wireless network that is within range of the transceiver.
//Inputs: Vector of parsed arguments. --These should inclose the SSID and network password.
void parseConnect( const vector<String> & );

//Descr: This function handles the arguments for connecting to the TCP server that will store the data it receives via the ESP (Through the wifi)
//Inputs: vector of parsed arguments. -- These should include the IP of the server, as well as the port.
void parseTCPConnect( const vector<String> & );

//Descr: This function handles the arguments for setting the device's hostname. This is typically used to describe the device on the DHCP table in a network router.
//Inputs: vector of parsed arguments. 
void parseHostname( const vector<String> & );

//Descr: This function actually handles the connection to the wireless network.
//Inputs: netowrk SSID, network password
void beginConnection( const String &, const String & );

//Descr: This function closes the wifi connection to the local network. (Also closes the TCP connection if needed)
void closeConnection();

//Descr: This function basically parses a string for a number, then returns that number.
//Inputs: String to be parsed.
long parseInt( const String & );

//Descr: This function prints a list of the network details for the current connection (if applicable)
void printDiag();

//Descr: This function is responsible for setting the information needed to establish the TCP connection to our data server.
//Inputs: Server IP address, Server Port
void beginTCPConnection( const String &, unsigned int );

//Descr: This function is responsible for closting the connection to the TCP server.
void closeTCPConnection();

//Descr: This function loads the default connection information for both the wireless network and the TCP server (From the eeprom), then attempts to establish the connections.
void loadDefaultConnection();

//Descr: This function saves off our current (functioning) network connection into the EEPROM, which will allow for auto-connect upon device boot.
//At a bare minimum, the Wifi network connection must be established before any data will be written, if the TCP connection is made, it will also be stored.
void saveDefaultConnection();

#endif /* WIFI_H_ */