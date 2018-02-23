/*
 * page_admin.cpp
 *
 * Created: 2/12/2018 4:04:47 PM
 *  Author: Andrew
 * The purpose of this file is to house the HTML generator for the admin page and related functions.
 */

 #include "ESPComm.h" 
 
 DataField *resourceField,
		   *htmlRefreshField,
		   *sqlHostname,
		   *sqlPort,
		   *autoSQLLogFIeld,
		   *autoSQLLogIntervalField; 
 
void ESPComm::CreateAdminFields()
{
	DataField resourceField( 2, TYPE_INPUT_CHECKBOX, "Verbose Mode (Serial)", "verbose_serial", "checked" );
	DataField htmlRefreshField( 3, TYPE_INPUT_TEXT, "Page Refresh Interval (Seconds)", "refresh", "3" ); //Number of seconds between automatic page refresh 0 = disabled
	DataField autoSQLLogField( 4, TYPE_INPUT_CHECKBOX, "Auto Log to SQL Server", "autolog", "checked" );
	DataField autoSQLLogIntervalField( 5, TYPE_INPUT_TEXT, "SQL Log Interval", "loginterval", "5" );
} 

void ESPComm::HandleAdmin()
{
	p_server->send(200, "text/html", "This is the config page." );
}

void ESPComm::ProcessAdminSettings()
{
	
}