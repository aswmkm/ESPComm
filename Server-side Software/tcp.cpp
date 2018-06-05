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
    QByteArray msg = "TCP Server Closing";
    int len = msg.length();

    for ( int x = 0; x < p_Sockets.size(); x++ ) //force all connected client to close
    {
        int i = 0;
        do
        {
            i += p_Sockets[x]->getTcpSocket()->write(msg); //Inform the client
        }while ( i < len );

        p_Sockets[x]->getTcpSocket()->close(); //force it to close.
    }
    p_Sockets.clear(); //empty the clients list.
    p_Server->close(); //don't allow new connections
}

void BackendServer::handleSendTCPData(const QStringList &args )
{
    int clientIndex = args[1].toUInt();

    if ( clientIndex >= p_Sockets.size() )
    {
        printToConsole("Invalid client at index: " + QString().number(clientIndex) );
        return; //end here
    }
    //for ( int x = 2; x < args.size(); x++ ) //everything else is the message
    if ( args[2].length() > MAX_MSG_LENGTH - 1 )
        printToConsole("Message too long. Maximum characters is limited to: " + QString().number(MAX_MSG_LENGTH - 1) );
    else
    {
        QByteArray block;
        //block << QString(args[2]).toLocal8Bit();// << CHAR_CR;
        block.append(args[2]);
        block.append(CHAR_CR);
        int len = block.length();
        int i = 0;
        do
        {
            int j = p_Sockets[clientIndex]->getTcpSocket()->write( block );
            if ( !j )
            {
                printToConsole("Failed to send message to client: " + QString::number(clientIndex) );
                break;
            }
            else
                i += j;
        }while ( i < len );

        //p_Sockets[clientIndex]->getTcpSocket()->flush();
    }
}
