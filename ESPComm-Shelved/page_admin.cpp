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
	DataTable *deviceTable = new DataTable( F("Device Specific Settings") );
	DataTable *networkTable = new DataTable( F("Wifi Network Settings") );
	DataTable *sqlTable = new DataTable( F("SQL Configuration") );
	DataTable *timeTable = new DataTable( F("System Time Settings") );
	DataTable *verboseTable = new DataTable( F("Verbose Settings") );
	DataTable *saveTable = new DataTable();
	
	//DataField *resourceField = new DataField( 2, TYPE_INPUT_CHECKBOX, "Verbose Mode (Serial)", "verbose_serial", "checked" );
	
	//Device specific setings
	deviceTable->AddElement( new STRING_Datafield( &s_uniqueID, 1, FIELD_TYPE::TEXT, F("Device Unique ID"), F("devid") ) ); 
	deviceTable->AddElement( new BOOL_Datafield( &b_enableAP, 2, FIELD_TYPE::CHECKBOX, F("Enable Access Point Mode"), F("devap") ) );
	deviceTable->AddElement( new DataField(3, FIELD_TYPE::TEXT, F("Access Point SSID"), F("devapssid") ) );
	deviceTable->AddElement( new DataField(4, FIELD_TYPE::PASSWORD, F("Access Point Password"), F("devappwd") ) );
	//
	
	//Network Table Stuff
	networkTable->AddElement( new SSID_Datafield(5, F("Wifi Network SSID"), F("netssid"), false ) );
	networkTable->AddElement( new DataField(6, FIELD_TYPE::PASSWORD, F("Wifi Password"), F("netpwd") ) );
	networkTable->AddElement( new UINT8_Datafield( &i_timeoutLimit, 7, FIELD_TYPE::TEXT, F("Wifi Connection Retry Limit"), F("netretrylimit") ) );
	networkTable->AddElement( new DataField(8, FIELD_TYPE::CHECKBOX, F("Auto Retry On Disconnect"), F("netautoretry") ) );
	//
	
	//SQL Table Stuff
	sqlTable->AddElement( new DataField(9, FIELD_TYPE::TEXT, F("SQL Server Hostname"), F("sqlhostname"), "server.test" ) );
	sqlTable->AddElement( new DataField(10, FIELD_TYPE::TEXT, F("SQL Server Port"), F("sqlport"), "" ) );
	sqlTable->AddElement( new DataField(11, FIELD_TYPE::PASSWORD, F("SQL Database Password"), F("sqlpwd") ) );
	sqlTable->AddElement( new DataField(12, FIELD_TYPE::TEXT, F("SQL Database Name"), F("sqldbname") ) );
	sqlTable->AddElement( new DataField(13, FIELD_TYPE::CHECKBOX, F("Auto Log to SQL Server"), F("sqlautolog"), "on" ) );
	sqlTable->AddElement( new DataField(14, FIELD_TYPE::TEXT, F("SQL Log Interval"), F("sqlloginterval"), "5" ) );
	//
	
	//Time table stuff
	timeTable->AddElement( new BOOL_Datafield( &b_enableNIST, 15, FIELD_TYPE::CHECKBOX, F("Enable NIST Time Updating"), F("nistupd") ) );
	timeTable->AddElement( new STRING_Datafield( &s_NISTServer, 16, FIELD_TYPE::TEXT, F("NIST Time Update Server"), F("nistserv") ) );
	timeTable->AddElement( new UINT_Datafield( &i_NISTPort, 17, FIELD_TYPE::TEXT, F("Time Server Port"), F("nistport") ) );
	timeTable->AddElement( new UINT_Datafield( &i_NISTupdateFreq, 18, FIELD_TYPE::TEXT, F("NIST Time Update Frequency"), "nistfreq" ) );
	//
	
	//Verbose table stuff
	verboseTable->AddElement( new UINT8_Datafield( &i_verboseMode, 19, FIELD_TYPE::TEXT, F("Verbose Mode (Serial)"), F("verbmode") ) );
	//	
	
	//Save Table Stuff
	saveTable->AddElement( new DataField(20, FIELD_TYPE::CHECKBOX, F("Write Config To EEPROM"), F("eepromwrite"), "", false ) );
	saveTable->AddElement( new DataField(21, FIELD_TYPE::SUBMIT, "", "", F("Apply Settings") ) );
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
	String AlertHTML;
	if ( p_server->args() ) //Do we have some args to input? Apply settings if so (before generating the rest of the HTML
		ProcessDeviceSettings( AlertHTML );
	
	String HTML = String(HTML_HEADER);
	HTML += "<title>Device: " + s_uniqueID + " Admin Page</title>"; //Page title
	HTML += AlertHTML; //Any alerts? MAY REWORK THIS LATER
	
	HTML += F("<FORM action=\"./admin\" method=\"post\">");
	
	for ( uint8_t x = 0; x < p_configDataTables.size(); x++ )
		HTML += p_configDataTables[x]->GenerateTableHTML(); //Add each datafield table to the HTML body
	
	HTML += F("</FORM>");
	HTML += String(HTML_FOOTER); //Add the footer stuff.
	p_server->send(200, "text/html", HTML );
}

void ESPComm::ProcessDeviceSettings( String &HTML )
{
	//This bit of code handles all of the Datafield value updating, depending on the args that were received from the POST method.
	for ( uint8_t i = 0; i < p_configDataTables.size(); i++ ) //Go through each setting.
	{
		DataField *tempField; //Init pointer here
		//HACKHACK - We need to set all checkboxes to "off", they'll be set to on later if applicable. This is due to a limit of the POST method
		for ( uint8_t y = 0; y < p_configDataTables[i]->GetFields().size(); y++ )
		{
			tempField = p_configDataTables[i]->GetFields()[y]; //Get the pointer
			
			if ( tempField->GetType() == FIELD_TYPE::CHECKBOX ) //Arg doesn't apply
				tempField->SetFieldValue("");
		}
		//
		
		for ( uint8_t x = 0; x < p_server->args(); x++ ) // For each arg...
		{
			tempField = p_configDataTables[i]->GetElementByName( p_server->argName(x) );
			if ( tempField ) //Found an element with this name?
			{
				if ( tempField->GetFieldValue() == p_server->arg(x) ) //Is the arg the same as the existing setting? //REWORK THIS A BIT
					continue; //Skip if so
				
				if ( tempField->SetFieldValue( p_server->arg(x) ) )
					HTML += tempField->GetFieldLabel() + " set to: " + p_server->arg(x) + "<br>"; //Inform the admin
				else 
					HTML += "Update of '" + tempField->GetFieldLabel() + "' failed. <br>";
			}
		}
	}
}