#include <server.h>

bool BackendServer::beginSQLConnection( const uint port )
{
    p_Database->addDatabase( settingsMap->value(CONFIG_SQL_DBTYPE).getString() );
    p_Database->setHostName( settingsMap->value(CONFIG_SQL_HOSTNAME).getString() );
    p_Database->setDatabaseName( settingsMap->value(CONFIG_SQL_DBNAME).getString() );
    p_Database->setUserName( settingsMap->value( CONFIG_SQL_DBUSER).getString() );
    p_Database->setPassword( settingsMap->value( CONFIG_SQL_DBPASS).getString() );
    p_Database->setPort( port );

    return p_Database->open(); //Attempt to open the connection with the args that were passed.
}

bool BackendServer::createDBTables( const QStringList &args )
{
    if ( !p_Database->isOpen() )
    {
        printToConsole("SQL Database not open.");
        return false;
    }

    QSqlQuery query;
    //query.
    return true;
}
