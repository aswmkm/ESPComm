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

QStringList BackendServer::parseConsoleArgs( const QString &msg )
{
    QStringList args;
    //args = msg.split("-");
    //So first we need to get the command being used, IE: send, clients, etc. Then we need to get the args, which are
    //annotated with the - char. Those args can have their own sets of args, which immediately follow.
    //So we need to read to the first space char or - to determine the command, then use the command to parse accordingly.
    //args[0].remove(" ");
    QString tempStr;
    bool inQuotes = false;

    for ( int x = 0; x <= msg.length(); x++ ) //find the command first
    {
        if ( msg[x] == "\"" )
        {
            inQuotes = !inQuotes; //flip
            continue; //skip this char
        }

        if ( ( ( msg[x] == " " || msg[x] == "-" ) && !inQuotes ) || x == msg.length() )
        {
            if ( tempStr.length() )
            {
                args.append( tempStr ); //save the stored string
                tempStr.clear(); //clear the temp or re-use
            }
        }
        else
            tempStr += msg[x]; //append the char
    }

    return args;
}

void BackendServer::parseConsoleMessage(const QString &msg)
{
    QStringList args = parseConsoleArgs( msg );
    //"close -tcp args -sql args2" -> "close " "tcp args" "sql args2"

    if ( !args.size() ) //must have at east one arg
        return;

    args[0].toLower(); //convert commands to lower case

    if ( args[0] == CMD_BEGIN )
    {
        if ( args.size() < 2 )
            printToConsole("Usage: -(all/tcp/sql) -args(optional, see documentation)");
        else
            handleBeginServers( args );
    }
    else if ( args[0] == CMD_CLIENTS )
    {
        if( !p_Server->isListening() )
            printToConsole("TCP is not running.", VERBOSE_PRIORITY::PRIORITY_HIGH);

        else if ( !p_Sockets.size() )
            printToConsole("No clients connected.", VERBOSE_PRIORITY::PRIORITY_HIGH );

        else
        {
            printToConsole("Connected clients:", VERBOSE_PRIORITY::PRIORITY_HIGH );
            for ( int x = 0; x < p_Sockets.size(); x++ )
                printToConsole( QString().number(x) + ": " + p_Sockets[x]->getTcpSocket()->localAddress().toString(), VERBOSE_PRIORITY::PRIORITY_HIGH );
        }
    }
    else if ( args[0] == CMD_CLOSE ) // CLOSING SERVICES
    {
        if ( args.size() < 2 ) //no args?
            printToConsole("Usage: close -(all/tcp/sql)");
        else
            handleCloseServers( args );
    }
    else if ( args[0] == CMD_CONFIG )
    {

    }
    else if ( args[0] == CMD_SEND )
    {
        if ( args.size() < 3 ) //must have at least 3 args available (CMD_SEND)/client/data
            printToConsole("Usage: -(client index #) (data)");
        else
            handleSendTCPData( args );
    }
    else
        printToConsole( QString("Unknown command: ") + args[0] );
}



void BackendServer::handleBeginServers(const QStringList &args)
{
    for ( int x = 1; x < args.size(); x++ )
    {
        QStringList values = args[x].split(" ");
        if ( values[0] == "tcp" )
        {
            if ( values.size() < 2 )
            {
                if ( !beginTCPServer( settingsMap->value(CONFIG_TCP_PORT).toUInt() ) )
                    printToConsole( QString("Failed to open TCP server on port: ") + settingsMap->value(CONFIG_TCP_PORT) );
            }
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
    //ClientSocket *connection = (ClientSocket *)p_Server->nextPendingConnection();
    connection->setParent( this );

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
