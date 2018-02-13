/*
 * EspComm.h
 *
 * Created: 2/4/2018 3:43:16 PM
 *  Author: Andrew
 * This header contains the base functions for the project.
 */ 

#include "common.h"
#include "data_fields.h" 
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
//SQL Stuff
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
//


#define REFRESH_RATE 3 //Make this static for now, might do a config section for this.


#ifndef ESPCOMM_H_
#define ESPCOMM_H_

class ESPComm
{
public:
	ESPComm()
	{
		b_attemptingConnection = false;		
	}
	
	void scanNetworks(); //Used to output to serial that gives detains on various available networks.
	bool setupAccessPoint( String ssid, String password ); //This function creates an access point based on inputted data

	//Parser stuff
	void parseSerialData();
	vector<String> parseArgs( int &, const uint8_t &, const char buffer[] ); //Used as the first interpreter, to divide up all arguments to their respective functions.
	bool parseAccessPoint( vector<String>  ); //Used to set te device as a wireless access point.
	void parseConnect( vector<String>  ); //Used to parse values used to connect to existing wireless networks.
	void parseVerbose( vector<String> ); //Used to enable/disable verbose mode.
	void parseDataFields( vector<String> ); //Used in the creation of data fields to be displayed via HTML (dynamically)
	void parseUpdate( vector<String>  ); //Used for the updating of data field values.
	void parseConfig( vector<String> ); //Used to modify local device configurations.
	void parseLogin( vector<String> ); //Used to log into (and load) user data stored remotely
	void parseEEPROMCfg( vector<String> ); //Used to program specific values into the EEPROM for non-volatile storage (default wifi connection, so on)
	//
	
	void Process(); //This basically functions as our loop function
	void setup(); //Setup functions to be called when device inits.
	void setupServer(); //Links specific web server address to corresponding page generation functions.
	void beginConnection( String, String ); //Used to connect to an existing wireless network.
	void closeConnection( bool = true ); //Used to close all connections to the ESP device.
	void sendMessage( String, bool = false ); //This prepares 
	void printDiag(); //Used to display current connection information for the wifi device.
	
	//These functions handle the generation of HTML pages t be transmitted to users
	void HandleIndex(); //Index that displays the data fields.
	void HandleConfig(); //Device config (possibly user specific configs loaded from SQL (if available ?) ) 
	void HandleLogin(); //User (student) login page.
	void HandleAdmin(); //Administration page (instructor page)
	//

private:
	bool b_attemptingConnection;
	bool b_verboseMode; //Used to determine what messages should be sent over the serial channel
	vector <DataField *> p_dataFields;
	wl_status_t currentStatus;
	ESP8266WebServer *p_server;
	uint8_t i_timeoutLimit;
};


long parseInt( String str );

#endif /* ESPCOMM_H_ */