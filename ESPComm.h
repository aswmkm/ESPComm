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
	vector<String> parseArgs( int &pos, char buffer[] );
	bool parseAccessPoint( vector<String> args );
	void parseConnect( vector<String> args );
	//
	
	void Process(); //This basically functions as our loop function
	void setup(); //Setup functions to be called when device inits.
	void setupServer(); //Links specific web server address to corresponding page generation functions.
	void beginConnection( const char *, const char *);
	void closeConnection();
	
	//These functions handle the generation of HTML pages t be transmitted to users
	void HandleIndex();
	void HandleConfig();
	void HandleLogin();
	//

private:
	bool b_attemptingConnection;
	vector <DataField *> p_dataFields;
	wl_status_t currentStatus;
	ESP8266WebServer *p_server;
};


long parseInt( String str );

#endif /* ESPCOMM_H_ */