/*
* page_config.cpp
*
* Created: 2/12/2018 4:04:33 PM
*  Author: Andrew
* The purpose of this file is to house the HTML generator for the device configuration page and related functions.
*/

#include "ESPComm.h"

  
void ESPComm::HandleConfig()
{
	String HTML = String(HTML_HEADER);
	HTML += PSTR("<title>Device: ") + s_uniqueID + PSTR(" User Config Page</title>"); //Page title
	
	if ( p_server->args() ) //Do we have some args to input? Apply settings if so.
		ProcessDeviceSettings( HTML );
	
	HTML += F("<FORM action=\"./config\" method=\"post\">");
	
	for ( uint8_t x = 0; x < p_configDataTables.size(); x++ )
		HTML += p_configDataTables[x]->GenerateTableHTML(); //Add each datafield table to the HTML body
	
	HTML += F("</FORM>");
	HTML += String(HTML_FOOTER); //Add the footer stuff.
	p_server->send(200, "text/html", HTML );
}
  