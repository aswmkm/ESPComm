#include "server.h"
#include "common.h"
#include "socket.h"


BackendServer::BackendServer() //default configuration data
{
    //Create the settings map here.
    settingsMap = new QMap<QString, QString>();
    settingsMap->insert(CONFIG_SQL_HOSTNAME, "localhost");
    settingsMap->insert(CONFIG_SQL_PORT, QString().number(DEFAULT_SQL_PORT) );
    settingsMap->insert(CONFIG_SQL_DBTYPE, "QMYSQL" );
    settingsMap->insert(CONFIG_SQL_DBNAME, "");
    settingsMap->insert(CONFIG_SQL_DBUSER, "");
    settingsMap->insert(CONFIG_SQL_DBPASS, "");
    settingsMap->insert(CONFIG_TCP_AUTOSTART, QString().number(0) );
    settingsMap->insert(CONFIG_TCP_PORT, QString().number(DEFAULT_TCP_PORT));
    settingsMap->insert(CONFIG_TCP_MAXCONNECTIONS, QString().number(DEFAULT_TCP_MAXCONNECTIONS) );
    settingsMap->insert(CONFIG_TCP_DUPLICATECONNECTIONS, QString().number(0) );
    settingsMap->insert(CONFIG_VERBOSITY, QString().number(VERBOSE_PRIORITY::PRIORITY_LOW) );
    //


    p_Server = new QTcpServer( this );
    p_Database = new QSqlDatabase();

    connect(p_Server, SIGNAL(newConnection()), this, SLOT(onClientConnected()));//Set up the slot that handles new socket connections.
}
BackendServer::~BackendServer()
{
    closeTCPServer(); //just in case
    p_Database->close();

    delete settingsMap;
    delete p_Server;
    delete p_Database;
}
bool BackendServer::LoadConfigFromFile(const QString &filename )
{
    QFile configFile(filename);

    if ( !configFile.exists() ) //Should we create a default config file?
    {
        if ( !configFile.open( QIODevice::ReadWrite | QIODevice::Text ) ) //still can't create it for some reason?
        {
            printToConsole( QString("Failed to create default config file: ").append(filename), VERBOSE_PRIORITY::PRIORITY_HIGH );
            return false;
        }

        else //create the default confg file here, then close it.
        {
            QTextStream fOut(&configFile);
            QMapIterator <QString, QString> i(*settingsMap); //needed to cleanly go through the entire settings map
            while ( i.hasNext() )
            {
                i.next(); //advance the iterator each pass
                fOut << i.key() << CHAR_CONFIG_SPLIT << i.value() << endl;
            }
            return true; //No need to continue here, since we just created the file from predefined settings.
        }
    }
    else if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text )) //it exists But it still won't open?
    {
        printToConsole( QString("Failed to open config file: ").append(filename), VERBOSE_PRIORITY::PRIORITY_HIGH );
        return false;
    }

    QTextStream fIn(&configFile); //Prepare to read lines from the file, now that it's open
    while (!fIn.atEnd())
    {
        QString s_line = fIn.readLine(); //Store the line we've just read
        if ( s_line[0] == '#' || s_line[0] == ';') //skip commented lines
            continue;

        QStringList cfgLine = s_line.split(CHAR_CONFIG_SPLIT);

        if (cfgLine.size() > 1) //cfgLine[0] = command, cfgLine[1] = value
        {
            cfgLine[0].remove(CHAR_WHITESPACE); //remove whitespaces first, just in case
            if ( settingsMap->contains(cfgLine[0]) ) //make sure the key already exists in the map.
            {
                if ( settingsMap->value( cfgLine[0]) == cfgLine[1] )
                    continue; //move to the next line, we're not changing this setting.

                 settingsMap->operator [](cfgLine[0]) = cfgLine[1]; //modify the value of this key
            }
            else
                printToConsole(QString("Unknown setting: ") + cfgLine[0], VERBOSE_PRIORITY::PRIORITY_MEDIUM );

        }
    }

    printToConsole(QString().append(filename).append(" loaded succesfully."), VERBOSE_PRIORITY::PRIORITY_MEDIUM );
    return true;
}

void BackendServer::printToConsole( const QString &msg, uint verbose )
{
    if ( settingsMap->value(CONFIG_VERBOSITY).toUInt() < verbose )// setting is less then passed in value? HIGHER INPUT VALUE = LOWER PRIORITY MESSAGE
        return; //just end here
    //Any other stuff here. if needed?
    emit printMessage( msg ); //send the message off
}

bool BackendServer::beginSQLConnection( uint port )
{
    p_Database->addDatabase( settingsMap->value(CONFIG_SQL_DBTYPE) );
    p_Database->setHostName( settingsMap->value(CONFIG_SQL_HOSTNAME) );
    p_Database->setDatabaseName( settingsMap->value(CONFIG_SQL_DBNAME) );
    p_Database->setUserName( settingsMap->value( CONFIG_SQL_DBUSER) );
    p_Database->setPassword( settingsMap->value( CONFIG_SQL_DBPASS) );
    p_Database->setPort( port );

    return p_Database->open(); //Attempt to open the connection with the args that were passed.
}

bool BackendServer::beginTCPServer( uint port )
{
    if ( p_Server->isListening() )
    {
        printToConsole( QString("Already listening on port: ").append(QString().number(p_Server->serverPort())));
        return false;
    }
    if ( p_Server->listen(QHostAddress::Any, port) )
    {
        p_Server->setMaxPendingConnections( settingsMap->value(CONFIG_TCP_MAXCONNECTIONS).toUInt() );
        printToConsole(QString("Listening for connections at address: ").append(p_Server->serverAddress().toString()).append(" on port: ").append(QString().number(p_Server->serverPort())));
        return true;
    }

    printToConsole( QString("Failed to start server on port: ").append(port) );
    return false;
}
void BackendServer::parseConsoleMessage(const QString &msg)
{
    QStringList commands = msg.split("-"); //cmd_split -cmd arg
    commands[0].remove(" "); //remove whitespaces for first command after splitting

    //"close -tcp args -sql args2" -> "close " "tcp args" "sql args2"

    if ( commands[0] == CMD_BEGIN )
    {
        if ( commands.size() < 2 )
            printToConsole("Usage: -(all/tcp/sql) -args(optional, see documentation)");
        else
            handleBeginServers( commands );
    }
    else if ( commands[0] == CMD_CLIENTS )
    {
        if( !p_Server->isListening() )
        {
            printToConsole("TCP is not running.", VERBOSE_PRIORITY::PRIORITY_HIGH);
            return;
        }

        if ( !p_Sockets.size() )
        {
            printToConsole("No clients connected.", VERBOSE_PRIORITY::PRIORITY_HIGH );
            return;
        }

        printToConsole("Connected clients:", VERBOSE_PRIORITY::PRIORITY_HIGH );
        for ( int x = 0; x < p_Sockets.size(); x++ )
            printToConsole( QString().number(x) + ": " + p_Sockets[x]->getTcpSocket()->localAddress().toString(), VERBOSE_PRIORITY::PRIORITY_HIGH );
    }
    else if ( commands[0] == CMD_CLOSE ) // CLOSING SERVICES
    {
        if ( commands.size() < 2 ) //no args?
        {
            printToConsole("Usage: close -(all/tcp/sql)");
            return;
        }
        else
            handleCloseServers( commands );
    }
    else if ( commands[0] == CMD_CONFIG )
    {

    }
    else
        printToConsole( QString("Unknown command: ") + commands[0] );
}

void BackendServer::handleBeginServers(const QStringList &args)
{
    for ( int x = 1; x < args.size(); x++ )
    {
        QStringList values = args[x].split(" ");
        if ( values[0] == "tcp" )
        {
            if ( values.size() < 2 && !beginTCPServer( settingsMap->value(CONFIG_TCP_PORT).toUInt() ) )
                printToConsole( QString("Failed to open TCP server on port: ") + settingsMap->value(CONFIG_TCP_PORT) );
            else if ( !beginTCPServer(values[1].toUInt() ) )
                printToConsole( QString("Failed to open TCP server on port: ") + values[1] );
        }
        else if ( values[0] == "sql ")
        {
            if ( values.size() < 2 && !beginSQLConnection( settingsMap->value(CONFIG_SQL_PORT).toUInt() ))
                printToConsole( QString("Failed to open SQL connection on port: ") + settingsMap->value(CONFIG_SQL_PORT) );
            else if ( beginSQLConnection( values[1].toUInt() ) )
                printToConsole( QString("Failed to open SQL connection on port: ") + values[1] );
        }
    }
}

void BackendServer::closeTCPServer()
{
    for ( int x = 0; x < p_Sockets.size(); x++ ) //force all connected client to close
    {
        p_Sockets[x]->getTcpSocket()->write("TCP Server Closing"); //Inform the client
        p_Sockets[x]->getTcpSocket()->close(); //force it to close.
    }
    p_Sockets.clear(); //empty the clients list.
    p_Server->close(); //don't allow new connections
}

void BackendServer::handleCloseServers(const QStringList &args)
{
    QStringList values = args[1].split(" "); //we only need to use the first arg here.
    //"tcp args" -> "tcp" "args"
    //"sql args2" -> "sql" "args2"
    if ( values[0] == "tcp" || values[0] == "all")
    {
        if ( p_Server->isListening() ) //TCP server
        {
            printToConsole( "Closing TCP Server..." );
            closeTCPServer();
        }
        else
            printToConsole( "TCP Server is not running.");
    }
    if ( values[0] == "sql" || values[0] == "all" )
    {
        if ( p_Database->isOpen() ) //SQL server
        {
            printToConsole( QString("Closing Database Connection...") );
            p_Database->close();
        }
        else
            printToConsole("SQL Server is not running.");
    }
}

void BackendServer::handleSetConfig(const QStringList &args)
{

}

void BackendServer::onClientConnected()
{
    ClientSocket *connection = new ClientSocket(p_Server->nextPendingConnection());
    connect( connection, SIGNAL(socketClosed( ClientSocket *)), this, SLOT( onClientDisconnected( ClientSocket *) ) );
    connect( connection, SIGNAL(forwardString(const QString &)), this, SLOT(onClientCommunication(QString))); //for handling incoming data

    QString clientAddress = connection->getTcpSocket()->localAddress().toString();

    if ( !settingsMap->value(CONFIG_TCP_DUPLICATECONNECTIONS).toUInt() ) //are we allowing duplicate client connections to the TCP server?
    {
        for ( int x = 0; x < p_Sockets.size(); x++ )
        {
            if ( clientAddress == p_Sockets[x]->getTcpSocket()->localAddress().toString() ) //compare client addresses
            {
                printToConsole("Connection already exists.");
                connection->getTcpSocket()->write("#ERR1"); //duplicate clients not allowed?
                connection->getTcpSocket()->close(); //force it to close.
                return; // end here
            }
        }
    }

    printToConsole(QString("New client connected at: ") + clientAddress );
    p_Sockets.push_back(connection);
}

void BackendServer::onClientDisconnected( ClientSocket *socket )
{
    if ( p_Sockets.removeOne( socket ) ) //attempt to remove the socket from our list of valid clients.
    {
        printToConsole("Client disconnected.");
        printToConsole( QString().number(p_Sockets.size()) );//debug
        socket->deleteLater();
        //delete socket;
    }
}

void BackendServer::onClientCommunication( QString msg ) //this handles any data that is received over the TCP sockets.
{
    printToConsole( msg );
}
