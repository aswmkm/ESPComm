#include <server.h> //class header

bool BackendServer::beginTCPServer( const uint port )
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

void BackendServer::closeTCPServer()
{
    for ( int x = 0; x < p_Sockets.size(); x++ ) //force all connected clients to close
    {
        p_Sockets[x]->sendMessage("TCP Server Closing"); //Inform the client
        p_Sockets[x]->getTcpSocket()->close(); //force it to close.
        p_Sockets[x]->deleteLater();
    }
    p_Sockets.clear(); //empty the clients list.
    p_Server->close(); //close the TCP server and don't allow new connections
}

void BackendServer::handleSendTCPData(const QStringList &args )
{
    int clientIndex = args[1].toUInt() - 1;
    if ( ( clientIndex >= p_Sockets.size() || clientIndex < 0 ) && args[1] != CMD_ALL )
    {
        printToConsole("Invalid client at index: " + QString().number(clientIndex + 1) );
        return; //end here
    }

    if ( args[1] == CMD_ALL )
    {
        for ( int x = 0; x < p_Sockets.size(); x++ )
        {
            TRANSMISSION_CONDITION trans = p_Sockets[x]->sendMessage(args[2]);
            if ( trans == TRANSMISSION_CONDITION::ERROR_GENERIC )
                printToConsole("Failed to send message to client at index: " + QString::number(x + 1) + "(" + p_Sockets[clientIndex]->getDeviceID() + ")" );
            else if ( trans == TRANSMISSION_CONDITION::ERROR_MSGLEN )
            {
                printToConsole("Message too long. Maximum characters is limited to: " + QString().number(MAX_MSG_LENGTH - 1) );
                return; //if the message is too long for one client, it's too long for the other. end the loop here here
            }
        }
    }
    else
    {
        TRANSMISSION_CONDITION trans = p_Sockets[clientIndex]->sendMessage(args[2]);
        if ( trans == TRANSMISSION_CONDITION::ERROR_GENERIC )
            printToConsole("Failed to send message to client at index: " + QString::number(clientIndex + 1) + "(" + p_Sockets[clientIndex]->getDeviceID() + ")" );
        else if ( trans == TRANSMISSION_CONDITION::ERROR_MSGLEN )
            printToConsole("Message too long. Maximum characters is limited to: " + QString().number(MAX_MSG_LENGTH - 1) );
    }
}

void BackendServer::handleTCPClients(const QStringList & args)
{
    if( !p_Server->isListening() )
        printToConsole("TCP server is not running.", VERBOSE_PRIORITY::PRIORITY_HIGH);

    else if ( !p_Sockets.size() )
        printToConsole("No clients connected.", VERBOSE_PRIORITY::PRIORITY_HIGH );

    else
    {
        if ( args.size() > 1 )
        {
            if ( args[1] == CMD_CLOSE ) //forcing a connection to close based on client index/IP/deviceID?
            {
                if( args.size() > 2 )
                {
                    int socketIndex = -1; //default to -1, this will change if we've found a valid client to close.
                    int tempIndex = args[2].toInt();
                    if ( tempIndex ) //Found a client index to try?
                    {
                        if ( tempIndex <= p_Sockets.size() )
                            socketIndex = tempIndex - 1;
                    }
                    else //We're searching by device ID or IP address?
                    {
                        for( int x = 0; x < p_Sockets.size(); x++ )
                        {
                            if ( args[2] == p_Sockets[x]->getDeviceID() || args[2] == p_Sockets[x]->getAddress() )
                            {
                                socketIndex = x;
                                break; //end here
                            }
                        }
                    }
                    if ( socketIndex > -1 )
                    {
                        p_Sockets[socketIndex]->sendMessage("Server closed the connection.");
                        p_Sockets[socketIndex]->getTcpSocket()->close(); //Close the socket -- OnClientDisconnected will handle the rest.
                    }
                    else
                        printToConsole("Invalid client at: " + args[2] );

                }
                else
                    printToConsole("Usage: -(close) (Client Index #/Device ID/IP)", VERBOSE_PRIORITY::PRIORITY_HIGH );
            }
        }
        else //simply print a lst of the connected clients.
        {
            printToConsole("Connected clients:", VERBOSE_PRIORITY::PRIORITY_HIGH );
            for ( int x = 0; x < p_Sockets.size(); x++ )
                printToConsole( QString().number(x + 1) + ": " + p_Sockets[x]->getAddress() + "(" + p_Sockets[x]->getDeviceID() + ")", VERBOSE_PRIORITY::PRIORITY_HIGH );
        }
    }
}

