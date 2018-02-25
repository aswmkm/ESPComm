/*
 * page_admin.cpp
 *
 * Created: 2/12/2018 4:04:47 PM
 *  Author: Andrew
 * The purpose of this file is to house the HTML generator for the admin page and related functions.
 */

 #include "ESPComm.h" 
 
void ESPComm::CreateAdminFields() //Should never be called more than once
{
	DataTable *deviceTable = new DataTable( F("Device Specific Settings"), F("devconf") );
	DataTable *networkTable = new DataTable( F("Wifi Network Settings"), F("devconf") );
	DataTable *sqlTable = new DataTable( F("SQL Configuration"), F("sqlconf") );
	DataTable *timeTable = new DataTable( F("System Time Settings"), F("timeconf") );
	DataTable *verboseTable = new DataTable( F("Verbose Settings"), F("verboseconf") );
	DataTable *saveTable = new DataTable();
	
	//DataField *resourceField = new DataField( 2, TYPE_INPUT_CHECKBOX, "Verbose Mode (Serial)", "verbose_serial", "checked" );
	
	//Device specific setings
	deviceTable->AddElement( new DataField( 1, TYPE_INPUT_TEXT, F("Device Unique ID"), F("devid"), s_uniqueID ) ); 
	deviceTable->AddElement( new DataField( 2, TYPE_INPUT_CHECKBOX, F("Enable Access Point Mode"), F("devap"), (b_enableAP ? "on" : "") ) );
	deviceTable->AddElement( new DataField( 3, TYPE_INPUT_TEXT, F("Access Point SSID"), F("devapssid") ) );
	deviceTable->AddElement( new DataField( 4, TYPE_INPUT_PASSWORD, F("Access Point Password"), F("devappwd") ) );
	//
	
	//Network Table Stuff
	networkTable->AddElement( new DataField(1, TYPE_INPUT_TEXT, F("Wifi Network SSID"), F("netssid") ) );
	networkTable->AddElement( new DataField(2, TYPE_INPUT_PASSWORD, F("Wifi Password"), F("netpwd") ) );
	networkTable->AddElement( new DataField(3, TYPE_INPUT_TEXT, F("Wifi Connection Retry Limit"), F("netretrylimit"), String(i_timeoutLimit) ) );
	networkTable->AddElement( new DataField(4, TYPE_INPUT_CHECKBOX, F("Auto Retry On Disconnect"), F("netautoretry") ) );
	//
	
	//SQL Table Stuff
	sqlTable->AddElement( new DataField(1, TYPE_INPUT_TEXT, F("SQL Server Hostname"), F("sqlhostname"), "server.test" ) );
	sqlTable->AddElement( new DataField(2, TYPE_INPUT_TEXT, F("SQL Server Port"), F("sqlport"), "" ) );
	sqlTable->AddElement( new DataField(3, TYPE_INPUT_PASSWORD, F("SQL Database Password"), F("sqlpwd") ) );
	sqlTable->AddElement( new DataField(4, TYPE_INPUT_TEXT, F("SQL Database Name"), F("sqldbname") ) );
	sqlTable->AddElement( new DataField(5, TYPE_INPUT_CHECKBOX, F("Auto Log to SQL Server"), F("sqlautolog"), "on" ) );
	sqlTable->AddElement( new DataField(6, TYPE_INPUT_TEXT, F("SQL Log Interval"), F("sqlloginterval"), "5" ) );
	//
	
	//Time table stuff
	timeTable->AddElement( new DataField( 1, TYPE_INPUT_CHECKBOX, F("Enable NIST Time Updating"), F("nistupd"), (b_enableNIST ? "on" : "") ) );
	timeTable->AddElement( new DataField( 2, TYPE_INPUT_TEXT, F("NIST Time Update Server"), F("nistserv"), s_NISTServer ) );
	timeTable->AddElement( new DataField( 3, TYPE_INPUT_TEXT, F("Time Server Port"), F("nistport"), String(i_NISTPort) ) );
	timeTable->AddElement( new DataField( 4, TYPE_INPUT_TEXT, F("NIST Time Update Frequency"), "nistfreq", String(i_NISTupdateFreq) ) );
	//
	
	//Verbose table stuff
	verboseTable->AddElement( new DataField(1, TYPE_INPUT_TEXT, F("Verbose Mode (Serial)"), F("verbmode"), String(i_verboseMode) ) );
	//	
	
	//Save Table Stuff
	saveTable->AddElement( new DataField(1, TYPE_INPUT_CHECKBOX, F("Write Config To EEPROM"), F("eepromwrite"), "", false ) );
	saveTable->AddElement( new DataField(2, TYPE_INPUT_SUBMIT, "", "", F("Apply Settings") ) );
	//
	
	//Add to our list of config tables for the ESPComm object
	p_configDataTables.push_back( deviceTable );
	p_configDataTables.push_back( networkTable );
	p_configDataTables.push_back( sqlTable );
	p_configDataTables.push_back( timeTable );
	p_configDataTables.push_back( verboseTable );
	p_configDataTables.push_back( saveTable );
	//
} 

void ESPComm::HandleAdmin()
{
	String HTML = String(HTML_HEADER);
	HTML += PSTR("<title>Device: ") + s_uniqueID + PSTR(" Admin Page</title>"); //Page title
	
	if ( p_server->args() ) //Do we have some args to input? Apply settings if so.
		ProcessDeviceSettings();
	
	HTML += F("<FORM action=\"./admin\" method=\"post\">");
	
	for ( uint8_t x = 0; x < p_configDataTables.size(); x++ )
		HTML += p_configDataTables[x]->GenerateTableHTML(); //Add each datafield table to the HTML body
	
	HTML += F("</FORM>");
	HTML += String(HTML_FOOTER); //Add the footer stuff.
	p_server->send(200, "text/html", HTML );
}

void ESPComm::ProcessDeviceSettings()
{
	for ( uint8_t i = 0; i < p_configDataTables.size(); i++ ) //Go through each setting.
	{
		DataField *tempField; //Init pointer here
		//HACKHACK - We need to set all checkboxes to "off", they'll be set to on later if applicable.
		for ( uint8_t y = 0; y < p_configDataTables[i]->GetFields().size(); y++ )
		{
			tempField = p_configDataTables[i]->GetFields()[y]; //Get the pointer
			
			if ( tempField->GetType() == TYPE_INPUT_CHECKBOX ) //Arg doesn't apply
			tempField->SetFieldValue(""); //HACKHACK - We need to set all checkboxes to "off", they'll be set to on later if applicable.
		}
		//
		
		for ( uint8_t x = 0; x < p_server->args(); x++ ) // For each arg...
		{
			tempField = p_configDataTables[i]->GetElementByName( p_server->argName(x) );
			if ( tempField ) //Found an element with this name?
			{
				if ( tempField->GetFieldValue() == p_server->arg(x) ) //Is the arg the same as the existing setting?
				continue; //Skip if so
				
				tempField->SetFieldValue( p_server->arg(x) );
				//HTML += tempField->GetFieldLabel() + " set to: " + p_server->arg(x); //Inform the admin
				//HTML += "<br>";
			}
		}
	}
}