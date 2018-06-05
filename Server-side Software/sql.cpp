#include <server.h>

bool BackendServer::beginSQLConnection( const uint port )
{
    p_Database->addDatabase( settingsMap->value(CONFIG_SQL_DBTYPE) );
    p_Database->setHostName( settingsMap->value(CONFIG_SQL_HOSTNAME) );
    p_Database->setDatabaseName( settingsMap->value(CONFIG_SQL_DBNAME) );
    p_Database->setUserName( settingsMap->value( CONFIG_SQL_DBUSER) );
    p_Database->setPassword( settingsMap->value( CONFIG_SQL_DBPASS) );
    p_Database->setPort( port );

    return p_Database->open(); //Attempt to open the connection with the args that were passed.
}
