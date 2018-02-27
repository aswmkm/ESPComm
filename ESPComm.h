/*
 * EspComm.h
 *
 * Created: 2/4/2018 3:43:16 PM
 *  Author: Andrew
 * This header contains the base functions for the project.
 */ 

#include "common.h"
#include "data_fields.h" 
#include "time.h"
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
	}
	
	void scanNetworks(); //Used to output to serial that gives detains on various available networks.
	bool setupAccessPoint( const String &, const String & ); //This function creates an access point based on inputted data

	//Parser stuff
	void parseSerialData();
	vector<String> parseArgs( int &, const uint8_t &, const char buffer[] ); //Used as the first interpreter, to divide up all arguments to their respective functions.
	bool parseAccessPoint( const vector<String> & ); //Used to set the device as a wireless access point.
	void parseConnect( const vector<String> & ); //Used to parse values used to connect to existing wireless networks.
	void parseVerbose( const vector<String> & ); //Used to enable/disable verbose mode.
	void parseDataFields( const vector<String> & ); //Used in the creation of data fields to be displayed via HTML (dynamically)
	void parseUpdate( const vector<String> & ); //Used for the updating of data field values.
	void parseLogin( vector<String> ); //Used to log into (and load) user data stored remotely
	void parseTime( const vector<String> & ); //Used to change certain system time related settings. No args returns current time.
	void parseEEPROMCfg( vector<String> ); //Used to program specific values into the EEPROM for non-volatile storage (default wifi connection, so on)
	//
	
	void Process(); //This basically functions as our loop function
	void updateClock(); //Updates device timer settings
	void setup(); //Setup functions to be called when device inits.
	void setupServer(); //Links specific web server address to corresponding page generation functions.
	void beginConnection( const String &, const String & ); //Used to connect to an existing wireless network.
	void closeConnection( bool = true ); //Used to close all connections to the ESP device.
	void sendMessage( const String &, uint8_t = PRIORITY_LOW ); //This prepares the inputted string for serial transmission, if applicable. 
	void printDiag(); //Used to display current connection information for the wifi device.
	
	//These functions handle the generation of HTML pages t be transmitted to users
	void HandleIndex(); //Index that displays the data fields.
	void HandleConfig(); //Device config (possibly user specific configs loaded from SQL (if available ?) ) 
	void HandleLogin(); //User (student) login page.
	void HandleAdmin(); //Administration page (instructor page)
	
	void CreateAdminFields(); //Create static data fields for page.
	
	void ProcessDeviceSettings( String & ); //Located in page_admin.cpp, shared with page_config, Appends setting change notifications (if applicable) 
	//
	
	bool CheckUpdateNIST(); //Used to determine if a NIST server check should be performed.
	bool UpdateNIST( bool = false ); //Used to perform the NIST update.

private:
	vector <DataField *> p_dataFields; //These are the data fields to be displayed on the index.
	vector <DataTable *> p_configDataTables; //Organizatioal datafield tables for the config/admin pages
	
	DataField *p_EEPROMSubmit; //Checkbox that enables/disables the saving of configs to the system EEPROM
	DataField *p_configSubmit; //Submit button for configs 
	
	ESP8266WebServer *p_server;
	
	uint8_t i_timeoutLimit;
	uint8_t i_verboseMode; //Used to determine what messages should be sent over the serial channel
	
	String s_uniqueID; //Unique ID string for device. 
	String s_apSSID; //SSID for AP mode  broadcast.
	String s_apPWD; //Password to connect to AP
	
	//System Clock objects
	Time *p_currentTime;
	Time *p_nextNISTUpdateTime; //Used to store the time for next NIST update.
	
	String s_NISTServer;
	unsigned int i_NISTPort;
	uint8_t i_lastUpdateSecond;
	//
	
	//NIST time variables
	unsigned int i_NISTupdateFreq; //frequency of NIST time update
	uint8 i_NISTUpdateUnit; //Unit of time for frequency between updates.
	//
	
	bool b_enableNIST; //Enable time server update mode?
	bool b_enableAP; //Enable access point mode?
};

long parseInt( const String &str );


#endif /* ESPCOMM_H_ */