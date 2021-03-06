/*
 * page_index.cpp
 *
 * Created: 2/12/2018 4:04:16 PM
 *  Author: Andrew
  * The purpose of this file is to house the HTML generator for the index page and related functions.
  * the important thing to note about the index vs. the other pages is that the index is dynamically generated using
  * DataField objects. These objects are created via serial commands (and possibly from the admin page later).
  * Typically this page is used as a means of monitoring (reading) information, and not for inputting. 
  */

#include "ESPComm.h"

void ESPComm::HandleIndex() //Generate the HTML for our main page.
{	  
	String HTML = String(HTML_HEADER);
	HTML += "<title>Device: " + s_uniqueID +"</title>"; //Page title
	
	if ( p_server->args() ) //Got some args?
	{
		for ( uint8_t x = 0; x < p_server->args(); x++ )
		{
			if ( p_server->argName( x ) == "1" )
			{
				p_server->send(200, "text/html", p_server->arg(x) );
			}
			else
				p_server->send(200, "text/html", "wrong arg" );
		}
	}
	
	HTML += "<FORM action=\"/\" method=\"post\">";
	HTML += "<P>";
	for ( uint8_t x = 0; x < p_dataFields.size(); x++ )
		HTML += p_dataFields[x]->GenerateHTML(); //Add each datafield to the HTML body
		
	HTML += "</P>";
	HTML += "</FORM>";
	HTML += String(HTML_FOOTER); //Add the footer stuff.
	p_server->send(200, "text/html", HTML ); //And we're off.
}
