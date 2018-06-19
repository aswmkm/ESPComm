#include "server.h"
#include "common.h"
#include "socket.h"


BackendServer::BackendServer( QObject *parent ) //default configuration data
{
    setParent( parent ); //memory management
    //Create the settings map here.
    settingsMap = new QMap<QString, ConfigValue>();
    settingsMap->insert(CONFIG_SQL_AUTOSTART, ConfigValue(0, true) );
    settingsMap->insert(CONFIG_SQL_HOSTNAME, ConfigValue("localhost", MAX_HOSTNAME_LENGTH) );
    settingsMap->insert(CONFIG_SQL_PORT, ConfigValue(DEFAULT_SQL_PORT, UINT16_MAX) );
    settingsMap->insert(CONFIG_SQL_DBTYPE, ConfigValue("QMYSQL", 24) ); //QT does not ship with any database drivers with a longer name length than 24. (Actually much less)
    settingsMap->insert(CONFIG_SQL_DBNAME, ConfigValue("", MAX_SQL_DBNAME) );
    settingsMap->insert(CONFIG_SQL_DBUSER, ConfigValue("", MAX_SQL_USERNAME) );
    settingsMap->insert(CONFIG_SQL_DBPASS, ConfigValue("", MAX_SQL_PASSWORD) );
    settingsMap->insert(CONFIG_TCP_AUTOSTART, ConfigValue(0, true) );
    settingsMap->insert(CONFIG_TCP_PORT, ConfigValue(DEFAULT_TCP_PORT, UINT16_MAX) );
    settingsMap->insert(CONFIG_TCP_MAXCONNECTIONS, ConfigValue(DEFAULT_TCP_MAXCONNECTIONS, MAX_TCP_CONNECTIONS) );
    settingsMap->insert(CONFIG_TCP_DUPLICATECONNECTIONS, ConfigValue(0, true) );
    settingsMap->insert(CONFIG_VERBOSITY, ConfigValue(VERBOSE_PRIORITY::PRIORITY_LOW, VERBOSE_PRIORITY::PRIORITY_LOW ) );
    //


    p_Server = new QTcpServer( this );
    p_Database = new QSqlDatabase();

    connect(p_Server, SIGNAL(newConnection()), this, SLOT(onClientConnected()));//Set up the slot that handles new socket connections.
}
BackendServer::~BackendServer()
{
    closeTCPServer(); //just in case
    p_Database->close(); //close the database connection.

    delete settingsMap;
    delete p_Server;
    delete p_Database;
}
bool BackendServer::LoadConfigFromFile(const QString &filename )
{
    QFile configFile(filename);

    if ( !configFile.exists() ) //Should we create a default config file?
    {
        if ( saveConfigFile( filename ) )
        {
            printToConsole(QString("Successfully created new config file: ") + CONFIG_FILE);
            return true;
        }
        else
            return false;
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
        if ( s_line[0] == CHAR_COMMENT_1 || s_line[0] == CHAR_COMMENT_2) //skip commented lines
            continue;

        QStringList cfgLine = s_line.split(CHAR_CONFIG_SPLIT);

        if (cfgLine.size() > 1) //cfgLine[0] = command, cfgLine[1] = value
        {
            cfgLine[0].remove(CHAR_WHITESPACE); //remove whitespaces first, just in case
            if ( settingsMap->contains(cfgLine[0]) ) //make sure the key already exists in the map.
            {
                ConfigValue *config = &settingsMap->operator [](cfgLine[0]);
                if ( !config ) //just in case
                    continue;

                if ( config->getString() == cfgLine[1] )
                    continue; //move to the next line, we're not changing this setting.

                if( config->setValue(cfgLine[1]) != CONFIG_SETVALUE::SUCCESS)
                {
                    if ( config->getConfigType() == CONFIGURATION_TYPE::TYPE_STRING )
                        printToConsole("The number of characters for " + cfgLine[0] + " must be between: " + QString().number(config->getMinValue())
                           + "~" + QString().number(config->getMaxValue()), VERBOSE_PRIORITY::PRIORITY_HIGH);
                    else
                        printToConsole("The value for " + cfgLine[0] + " must be between: " + QString().number(config->getMinValue())
                           + "~" + QString().number(config->getMaxValue()), VERBOSE_PRIORITY::PRIORITY_HIGH);

                }
            }
            else
                printToConsole(QString("Unknown setting: ") + cfgLine[0], VERBOSE_PRIORITY::PRIORITY_MEDIUM );

        }
    }
    configFile.close();
    printToConsole( filename + " loaded succesfully.", VERBOSE_PRIORITY::PRIORITY_MEDIUM );
    return true;
}

bool BackendServer::saveConfigFile(const QString &filename )
{
    QFile configFile(filename);

    if ( !configFile.open( QIODevice::ReadWrite | QIODevice::Text ) ) //can't open or create it for some reason?
    {
        printToConsole( QString("Failed to create or open config file: ").append(filename), VERBOSE_PRIORITY::PRIORITY_HIGH );
        return false;
    }

    QMapIterator <QString, ConfigValue> i(*settingsMap);
    QVector<QMapIterator<QString, ConfigValue> *> parsedIterators; //pointers to the iterators that have been parsed and replaced in existing text
    QString inputBuffer = configFile.readAll(), //read all chars into an input buffer for parsing.
            outputBuffer; //buffer to store the final output for writing.

    QStringList inputLines = inputBuffer.split(CHAR_NL); //split on newlines

    for ( int x = 0; x < inputLines.size(); x++ ) //go through the lines
    {
        QStringList lineKeyValue = inputLines[x].split(CHAR_CONFIG_SPLIT);

        if ( inputLines[x][0] == CHAR_COMMENT_1 || inputLines[x][0] == CHAR_COMMENT_2 || lineKeyValue.size() < 2 )
        {
            outputBuffer.append(inputLines[x] + CHAR_NL); //All the line to our output buffer and move on
            continue;
        }

        lineKeyValue[0].remove(CHAR_WHITESPACE); //just to be sure, before we compare

        i.toFront();
        while ( i.hasNext() ) //we're going through all iterations of the settings map
        {
            i.next(); //advance
            if ( lineKeyValue[0] == i.key() )//the config key in the line read from the buffer line matches the key from the iterated setting
            {
                parsedIterators.push_back( &i ); //make sure we don't do anything with this iterator later, since it exists already
                if ( i.value() == lineKeyValue[1] ) //same value already exists in the file for this key, just move on.
                {
                    outputBuffer.append(inputLines[x] + CHAR_NL); //nothing changed, just append the entire existing line.
                    break;
                }
                lineKeyValue[1] = i.value().getString(); //set the new value
                outputBuffer.append(lineKeyValue[0] + CHAR_CONFIG_SPLIT + lineKeyValue[1] + CHAR_NL);
                break; //we've done what we needed to do. end this loop
            }
        }

    }

    i.toFront(); //reset iterator progression.
    while ( i.hasNext() ) //now we're going through the keys of our settings map that were not previously addressed (non-existant or new config file?)
    {
        i.next(); //advance
        if ( !parsedIterators.contains( &i ) ) //write values that haven't been overwritten or previously read (non existant?)
            outputBuffer.append( i.key() + CHAR_CONFIG_SPLIT + i.value().getString() + CHAR_NL );
    }

    //At the end of itall, we write the entire output buffer to our file.
    configFile.resize(filename, outputBuffer.length() );
    configFile.seek(0); //to go beginning of file.
    configFile.write(outputBuffer.toLocal8Bit(), outputBuffer.length()); //write the output

    configFile.close();
    return true;
}

void BackendServer::printToConsole( const QString &msg, VERBOSE_PRIORITY verbose )
{
    if ( settingsMap->value(CONFIG_VERBOSITY).toInt() < verbose )// setting is less then passed in value? HIGHER INPUT VALUE = LOWER PRIORITY MESSAGE
        return; //just end here
    //Any other stuff here. if needed?
    emit printMessage( msg ); //send the message off
}

QStringList BackendServer::parseConsoleArgs( const QString &msg )
{
    QStringList args;
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

    if ( !args.size() ) //must have at least one arg
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
        handleTCPClients( args );
    else if ( args[0] == CMD_CLOSE ) // CLOSING SERVICES
    {
        if ( args.size() < 2 ) //no args?
            printToConsole("Usage: close -(all/tcp/sql)");
        else
            handleCloseServers( args );
    }
    else if ( args[0] == CMD_CONFIG )
    {
        if ( args.size() < 2 )
        {
            printToConsole("Usage: <config> <value>");
            printToConsole("Available configuration settings and current settings: <setting>=<value>");
            QMapIterator <QString, ConfigValue> i(*settingsMap); //needed to cleanly go through the entire settings map
            while ( i.hasNext() )
            {
                i.next(); //advance the iterator each pass
                printToConsole( i.key() + CHAR_CONFIG_SPLIT + i.value().getString() + "    " + QString().number(i.value().getMinValue())
                                + "~" + QString().number( i.value().getMaxValue() )
                                + (i.value().getConfigType() == CONFIGURATION_TYPE::TYPE_STRING ? " characters" : ""), VERBOSE_PRIORITY::PRIORITY_HIGH );
            }
        }
        else
            handleSetConfig(args);
    }
    else if ( args[0] == CMD_SEND )
    {
        if ( args.size() < 3 ) //must have at least 3 args available (CMD_SEND)/client/data
            printToConsole("Usage: -(Client Index #) (data)");
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
        if ( args[x] == CMD_TCP )
        {
            uint port = 0;
            if ( x + 1 < args.size() )
                port = args[x+1].toUInt(); //attempt to convert to int. return 0 if invalid

            if ( !port ) //no port number assigned, use default from config
            {
                if ( !beginTCPServer( settingsMap->value(CONFIG_TCP_PORT).toInt() ) )
                    printToConsole( QString("Failed to open TCP server on port: ") + settingsMap->value(CONFIG_TCP_PORT).getString() );
            }
            else
            {
                if ( !beginTCPServer( port ) )
                {
                    printToConsole( QString("Failed to open TCP server on port: ") + QString().number(port) );
                    x++; //found a port, skip one iterator
                }
            }
        }
        else if ( args[x] == CMD_SQL )
        {
            uint port = 0;
            if ( x + 1 < args.size() )
                port = args[x+1].toUInt();
            if ( !port )
            {
                if ( !beginSQLConnection( settingsMap->value(CONFIG_SQL_PORT).toInt() ))
                    printToConsole( QString("Failed to open SQL connection on port: ") + settingsMap->value(CONFIG_SQL_PORT).getString() );
            }
            else
            {
                if ( !beginSQLConnection( port ) )
                {
                    printToConsole( QString("Failed to open SQL connection on port: ") + QString().number(port) );
                    x++; //found a port, skip one iterator
                }
            }
        }
    }
}

void BackendServer::handleCloseServers(const QStringList &args)
{
    QStringList values = args[1].split(CHAR_WHITESPACE); //we only need to use the first arg here.
    //"tcp args" -> "tcp" "args"
    //"sql args2" -> "sql" "args2"
    if ( values[0] == CMD_TCP || values[0] == CMD_ALL )
    {
        if ( p_Server->isListening() ) //TCP server
        {
            printToConsole( "Closing TCP Server..." );
            closeTCPServer();
        }
        else
            printToConsole( "TCP Server is not running.");
    }
    if ( values[0] == CMD_SQL || values[0] == CMD_ALL )
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
    for ( int x = 1; x < args.size(); x++ )
    {
        if ( settingsMap->contains( args[x] ) )
        {
            if ( (x + 1) >= args.size() ) //make sure we've got an arg to server as a config value.
            {
                printToConsole("Missing value for config: " + args[x] );
                break; //end here
            }
            ConfigValue *config = &settingsMap->operator [](args[x]);
            if ( !config )
                continue;

            if ( config->getString() == args[x+1] )
                printToConsole("Value for " + args[x] + " unchanged.");
            else
            {
                switch( config->setValue(args[x+1]) )
                {
                    case CONFIG_SETVALUE::SUCCESS:
                        printToConsole("Setting value of " + args[x] + " to: " + args[x+1], VERBOSE_PRIORITY::PRIORITY_MEDIUM );
                        break;

                    default:
                        {
                            if ( config->getConfigType() == CONFIGURATION_TYPE::TYPE_STRING )
                                printToConsole("The number of characters for " + args[x] + " must be between: " + QString().number(config->getMinValue())
                                   + "~" + QString().number(config->getMaxValue()), VERBOSE_PRIORITY::PRIORITY_HIGH);
                            else
                                printToConsole("The value for " + args[x] + " must be between: " + QString().number(config->getMinValue())
                                   + "~" + QString().number(config->getMaxValue()), VERBOSE_PRIORITY::PRIORITY_HIGH);
                        }
                        break;
                }
            }
            x++; //advance past the value.
        }
        else if ( args[x] == CMD_SAVE )//save the current configuration to the
        {
            if ( saveConfigFile(CONFIG_FILE) )
                printToConsole(QString("Configuration successfully saved to: ") + CONFIG_FILE );
            else
                printToConsole(QString("Failed to save configuration to: ") + CONFIG_FILE );
        }
        else
            printToConsole("Unknown setting: " + args[x] );
    }
}

void BackendServer::onClientConnected()
{
    ClientSocket *connection = new ClientSocket(p_Server->nextPendingConnection());
    connection->setParent( this );

    connect( connection, SIGNAL(socketClosed( ClientSocket *)), this, SLOT( onClientDisconnected( ClientSocket *) ) );
    connect( connection, SIGNAL(forwardString(const QString &)), this, SLOT(onClientCommunication(const QString &))); //for handling incoming data

    QString clientAddress = connection->getAddress();
    if ( !settingsMap->value(CONFIG_TCP_DUPLICATECONNECTIONS).toInt() ) //are we allowing duplicate client connections to the TCP server?
    {
        for ( int x = 0; x < p_Sockets.size(); x++ )
        {
            if ( clientAddress == p_Sockets[x]->getAddress() ) //compare client addresses
            {
                printToConsole("Connection already exists.");
                connection->sendMessage("#ERR1"); //duplicate clients not allowed?
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
        printToConsole("Client at: " + socket->getAddress() + "(" + socket->getDeviceID() + ")" + " disconnected.");
        socket->deleteLater();
    }
}

void BackendServer::onClientCommunication( const QString &msg ) //this handles any data that is received over the TCP sockets.
{
    printToConsole( msg );
}
