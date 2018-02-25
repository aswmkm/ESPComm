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
  
/*

"<h1>ESP8266 Web Form Demo</h1>" //Custom section names later?
"<FORM action=\"/\" method=\"post\">"
"<P>"
"LED<br>"
"<INPUT type=\"radio\" name=\"LED\" value=\"1\">On<BR>"
"<INPUT type=\"radio\" name=\"LED\" value=\"0\">Off<BR>"
"<INPUT type=\"submit\" value=\"Send\"> <INPUT type=\"reset\">"
"</P>"
"</FORM>";
*/

void ESPComm::HandleIndex() //Generate the HTML for our main page.
{	  
	String HTML = String(HTML_HEADER);
	HTML += "<title>Device ID Here:</title>"; //Page title
	
	HTML += "<FORM action=\"/\" method=\"post\">";
	HTML += "<P>";
	for ( uint8_t x = 0; x < p_dataFields.size(); x++ )
		HTML += p_dataFields[x]->GenerateHTML(); //Add each datafield to the HTML body
		
	HTML += "</P>";
	HTML += "</FORM>";
	HTML += String(HTML_FOOTER); //Add the footer stuff.
	p_server->send(200, "text/html", HTML ); //And we're off.
}
